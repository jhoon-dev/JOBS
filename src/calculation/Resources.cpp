#include "calculation/Resources.h"
#include "calculation/WorkCalendar.h"
#include <algorithm>
#include <map>
#include <set>
#include <unordered_map>
#include <stdexcept>

namespace jobs {
namespace {
using Minute = std::int64_t;
using Events = std::map<Minute, Minute>;
struct Span { Minute start, finish; std::string taskId; int units; };
std::optional<int> capacity(const Resource& r, const std::string& date) {
    for (const auto& d : r.exceptions) if (d.date == date) return d.units;
    return r.maxUnits;
}
std::pair<Minute, Minute> span(const TaskSchedule& row, const ScheduleResult& result, const WorkCalendar& calendar) {
    if (result.isForecast && row.completedActual) return {0, 0};
    return {result.isForecast ? calendar.offset(row.remainingStart.value_or(row.start)) : row.earlyStartMinutes,
        row.earlyFinishMinutes};
}
void reserve(Events& events, Minute start, Minute finish, int units) {
    if (finish <= start) return;
    events[start] += units; events[finish] -= units;
}
// Visit every interval where either demand or daily capacity can change.
template<class F> void intervals(const Events& events, Minute start, Minute finish, F visit) {
    Minute used = 0;
    auto it = events.begin();
    while (it != events.end() && it->first <= start) { used += it->second; ++it; }
    for (auto at = start; at < finish;) {
        auto end = std::min(finish, (at / 480 + 1) * 480);
        if (it != events.end()) end = std::min(end, it->first);
        if (!visit(at, end, used)) return;
        at = end;
        while (it != events.end() && it->first <= at) { used += it->second; ++it; }
    }
}
}
ResourceLoad resourceLoad(const Project& p, const ScheduleResult& result) {
    ResourceLoad load; load.totalTasks = p.tasks.size();
    if (result.documentId != p.documentId || result.inputRevision != p.revision || result.statusAt != p.statusAt ||
        result.profile != p.calculationProfile || result.status != CalculationStatus::Valid) {
        load.diagnostics.push_back({"NO_CURRENT_SCHEDULE", "최신 유효한 일정이 없어 자원 수요를 계산할 수 없습니다.", {}}); return load;
    }
    WorkCalendar calendar(p);
    std::unordered_map<std::string, const TaskSchedule*> rows;
    for (const auto& r : result.tasks) rows.emplace(r.taskId, &r);
    std::set<std::string> assigned;
    std::unordered_map<std::string, std::vector<Span>> spans;
    for (const auto& a : p.assignments) {
        assigned.insert(a.taskId);
        const auto [s, e] = span(*rows.at(a.taskId), result, calendar);
        if (e > s) spans[a.resourceId].push_back({s, e, a.taskId, a.units});
    }
    load.assignedTasks = assigned.size(); load.status = CalculationStatus::Valid;
    for (const auto& r : p.resources) {
        Events events;
        for (const auto& s : spans[r.id]) reserve(events, s.start, s.finish, s.units);
        if (events.empty()) continue;
        std::map<Minute, ResourceDay> days;
        intervals(events, events.begin()->first, events.rbegin()->first, [&](Minute s, Minute e, Minute used) {
            if (!used) return true;
            auto day = s / 480;
            auto& d = days[day]; d.resourceId = r.id; d.date = calendar.boundary(day * 480, false).substr(0, 10);
            d.capacity = capacity(r, d.date); d.peakUnits = std::max(d.peakUnits, used); d.demandUnitMinutes += used * (e - s);
            if (!d.capacity) load.status = CalculationStatus::Incomplete;
            else if (used > *d.capacity) { d.overloadMinutes += e - s; d.excessUnitMinutes += (used - *d.capacity) * (e - s); }
            return true;
        });
        for (auto& [day, d] : days) {
            for (const auto& s : spans[r.id]) if (s.start < (day + 1) * 480 && s.finish > day * 480) d.taskIds.push_back(s.taskId);
            std::sort(d.taskIds.begin(), d.taskIds.end()); d.taskIds.erase(std::unique(d.taskIds.begin(), d.taskIds.end()), d.taskIds.end());
            load.days.push_back(std::move(d));
        }
    }
    const bool unknown = load.status == CalculationStatus::Incomplete;
    const bool overload = std::any_of(load.days.begin(), load.days.end(), [](const auto& d) { return d.overloadMinutes > 0; });
    if (overload && !unknown) load.status = CalculationStatus::Conflict;
    if (unknown) load.diagnostics.push_back({"UNKNOWN_CAPACITY", "배정 구간에 가용량 미정이 있습니다. 미정은 가용량 0과 구분합니다.", {}});
    if (overload) load.diagnostics.push_back({"RESOURCE_OVERLOAD", "동시간대 배정률이 가용량을 초과합니다. 날짜별 과부하 시간과 초과 공수를 확인하세요.", {}});
    if (assigned.size() < p.tasks.size()) load.diagnostics.push_back({"UNASSIGNED_TASKS", "미배정 작업은 자원 수요에 포함되지 않습니다. 배정된 자원에 한해 검증합니다.", {}});
    return load;
}
LevelingResult levelResources(const Project& p, const std::string& priority, int horizonWorkDays) {
    if ((priority != "task-order" && priority != "shortest-first" && priority != "longest-first") || horizonWorkDays < 1 || horizonWorkDays > 5200)
        throw std::runtime_error("지원하지 않는 우선순위 또는 탐색 범위입니다.");
    LevelingResult out; out.priority = priority; out.horizonWorkDays = horizonWorkDays;
    out.original = calculate(p); out.before = resourceLoad(p, out.original); out.leveled = out.original;
    out.leveled.calculationPolicy = "serial-no-split-v1/" + priority;
    if (out.original.status != CalculationStatus::Valid) return out;
    WorkCalendar calendar(p);
    const Minute horizon = Minute(horizonWorkDays) * 480;
    std::unordered_map<std::string, std::size_t> indexes;
    std::unordered_map<std::string, const Resource*> resources;
    std::unordered_map<std::string, std::vector<const Assignment*>> assignments;
    for (std::size_t i = 0; i < p.tasks.size(); ++i) indexes.emplace(p.tasks[i].id, i);
    for (const auto& r : p.resources) resources.emplace(r.id, &r);
    for (const auto& a : p.assignments) assignments[a.taskId].push_back(&a);
    std::unordered_map<std::string, Events> reserved;
    std::vector<bool> done(p.tasks.size(), false);
    std::vector<TaskSchedule> rows = out.original.tasks;
    // Engine output uses input order; look up IDs explicitly at this boundary.
    for (const auto& r : out.original.tasks) rows[indexes.at(r.taskId)] = r;
    std::size_t count = 0;
    while (count < p.tasks.size()) {
        std::optional<std::size_t> chosen;
        for (std::size_t i = 0; i < p.tasks.size(); ++i) {
            if (done[i]) continue;
            bool ready = true;
            for (const auto& pred : p.tasks[i].predecessors) if (!done[indexes.at(pred)]) ready = false;
            if (!ready) continue;
            auto length = [&](std::size_t n) { const auto [s, e] = span(rows[n], out.original, calendar); return e - s; };
            if (!chosen || (priority == "shortest-first" && length(i) < length(*chosen)) ||
                (priority == "longest-first" && length(i) > length(*chosen))) chosen = i;
        }
        if (!chosen) throw std::runtime_error("평준화 입력에 순환이 있습니다.");
        auto i = *chosen; auto& row = rows[i]; const auto& task = p.tasks[i];
        if (row.completedActual) { done[i] = true; ++count; continue; }
        auto [start, finish] = span(row, out.original, calendar); const Minute duration = finish - start, originalStart = start;
        std::vector<std::string> causes{task.id};
        for (const auto& pred : task.predecessors) {
            if (rows[indexes.at(pred)].earlyFinishMinutes > originalStart) causes.push_back(pred);
            start = std::max(start, rows[indexes.at(pred)].earlyFinishMinutes);
        }
        std::map<std::string, std::string> blocked;
        auto reason = [&] {
            std::string message;
            if (causes.size() > 1) message = " 선행 지연 ID:";
            for (std::size_t n = 1; n < causes.size(); ++n) message += " " + causes[n];
            for (const auto& [id, text] : blocked) { (void)id; message += " 자원 대기: " + text + ";"; }
            return message;
        };
        bool unknown = false, found = false;
        try {
            while (start + duration <= horizon) {
                calendar.boundary(start + duration, duration > 0);
                Minute next = start;
                for (const auto* a : assignments[task.id]) {
                    const auto& r = *resources.at(a->resourceId);
                    intervals(reserved[r.id], start, start + duration, [&](Minute s, Minute, Minute used) {
                        auto cap = capacity(r, calendar.boundary(s, false).substr(0, 10));
                        if (!cap) unknown = true;
                        if (!cap || used + a->units > *cap) {
                            if (!blocked.contains(r.id)) blocked.emplace(r.id, r.name + " [" + r.id + "] " + calendar.boundary(s, false) +
                                " · 수요 " + std::to_string(used + a->units) + "% / 가용 " + (cap ? std::to_string(*cap) + "%" : "미정"));
                            // The demand/capacity stays infeasible until its next real change,
                            // even when the trial task finishes earlier. Avoid one-minute retries.
                            Minute change = (s / 480 + 1) * 480;
                            if (cap && a->units <= *cap) {
                                auto event = reserved[r.id].upper_bound(s);
                                if (event != reserved[r.id].end()) change = std::min(change, event->first);
                            }
                            next = std::max(next, change); return false;
                        }
                        return true;
                    });
                }
                if (next == start) { found = true; break; }
                start = next;
            }
        } catch (const std::exception&) { found = false; }
        if (!found) {
            out.leveled.status = unknown ? CalculationStatus::Incomplete : CalculationStatus::Conflict;
            out.leveled.tasks.clear(); out.leveled.projectFinish.reset();
            out.diagnostics.push_back({unknown ? "LEVEL_UNKNOWN_CAPACITY" : "LEVEL_HORIZON_EXHAUSTED",
                "이 작업을 탐색 범위 안에서 배치하지 못했습니다. 가용량·배정률·탐색 범위·우선순위를 확인하세요. 부분 결과는 확정하지 않습니다." + reason(), causes});
            return out;
        }
        finish = start + duration;
        for (const auto* a : assignments[task.id]) reserve(reserved[a->resourceId], start, finish, a->units);
        if (start != originalStart) {
            if (!out.original.isForecast || !progressAsOf(p, task.id) || !progressAsOf(p, task.id)->actualStart) row.start = calendar.boundary(start, false);
            row.finish = calendar.boundary(finish, duration > 0);
            if (out.original.isForecast) row.remainingStart = calendar.boundary(start, false);
            out.diagnostics.push_back({"RESOURCE_DELAY", "잔여 시작을 " + std::to_string(start - originalStart) + " 근무분 지연했습니다." + reason(), causes});
        }
        row.earlyStartMinutes = out.original.isForecast && progressAsOf(p, task.id) && progressAsOf(p, task.id)->actualStart ? row.earlyStartMinutes : start;
        row.earlyFinishMinutes = finish;
        row.lateStartMinutes = row.lateFinishMinutes = row.totalFloatMinutes = row.freeFloatMinutes = 0; row.critical = false;
        row.drivingPredecessors.clear();
        done[i] = true; ++count;
    }
    out.leveled.tasks = std::move(rows);
    if (!out.leveled.tasks.empty()) {
        out.leveled.projectFinish = std::max_element(out.leveled.tasks.begin(), out.leveled.tasks.end(), [](const auto& a, const auto& b) { return a.finish < b.finish; })->finish;
    }
    out.after = resourceLoad(p, out.leveled);
    return out;
}
Project scenarioProject(const LevelingScenario& s) {
    Project p; static_cast<PlanInput&>(p) = s.input;
    p.documentId = s.sourceDocumentId; p.revision = s.inputRevision; p.statusAt = s.statusAt; p.progress = s.progress;
    validate(p); return p;
}
}
