#include "calculation/Scheduler.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace jobs {
namespace {
using namespace std::chrono;
sys_days date(const std::string& s) {
    return sys_days{year{std::stoi(s.substr(0, 4))}/unsigned(std::stoi(s.substr(5, 2)))/unsigned(std::stoi(s.substr(8, 2)))};
}
std::string stamp(sys_days d, int minute) {
    auto ymd = year_month_day{d}; char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02u %02d:%02d", int(ymd.year()),
        unsigned(ymd.month()), unsigned(ymd.day()), minute / 60, minute % 60);
    return buffer;
}
class Calendar {
    std::vector<sys_days> days_;
public:
    explicit Calendar(const Project& p) {
        std::unordered_set<std::int64_t> holidays;
        for (const auto& h : p.holidays) holidays.insert(date(h).time_since_epoch().count());
        auto d = date(p.startDate);
        for (int i = 0; i < 366 * 20 && year_month_day{d}.year() <= year{9999}; ++i, d += days{1}) {
            const auto weekday = std::chrono::weekday{d}.iso_encoding();
            if (weekday <= 5 && !holidays.contains(d.time_since_epoch().count())) days_.push_back(d);
        }
    }
    std::string boundary(std::int64_t offset, bool finish) const {
        // A positive task starts after a non-working gap; a finish/milestone stays on the preceding endpoint.
        if (offset < 0) throw std::runtime_error("계산 offset이 음수입니다.");
        const auto index = finish && offset > 0 ? (offset - 1) / 480 : offset / 480;
        int inDay = int(finish && offset > 0 ? (offset - 1) % 480 + 1 : offset % 480);
        if (index >= std::int64_t(days_.size())) throw std::runtime_error("계획 시작부터 20년 이내에 근무시간이 부족합니다.");
        const auto minute = inDay < 180 || (finish && inDay == 180) ? 9 * 60 + inDay : 13 * 60 + inDay - 180;
        return stamp(days_[std::size_t(index)], minute);
    }
};
}
ScheduleResult calculate(const Project& p) {
    ScheduleResult result; result.documentId = p.documentId; result.inputRevision = p.revision; result.profile = p.calculationProfile;
    try { validate(p); }
    catch (const std::exception& e) {
        result.status = CalculationStatus::Conflict;
        result.diagnostics.push_back({"INVALID_INPUT", e.what(), {}}); return result;
    }
    if (p.tasks.empty()) { result.diagnostics.push_back({"NO_TASKS", "계산할 작업이 없습니다.", {}}); return result; }
    const auto n = p.tasks.size();
    std::unordered_map<std::string, std::size_t> ids;
    for (std::size_t i = 0; i < n; ++i) ids.emplace(p.tasks[i].id, i);
    std::vector<std::vector<std::size_t>> successors(n), predecessors(n);
    std::vector<std::size_t> degree(n), order;
    bool missing = false, invalid = false;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& task = p.tasks[i];
        if (!task.durationMinutes) {
            missing = true; result.diagnostics.push_back({"MISSING_DURATION", "작업 기간이 미정입니다: " + task.name, {task.id}});
        }
        for (const auto& id : task.predecessors) {
            const auto found = ids.find(id);
            if (found == ids.end() || found->second == i) {
                invalid = true; result.diagnostics.push_back({"INVALID_DEPENDENCY", "존재하지 않거나 자기 자신인 선행 작업입니다: " + task.name, {task.id, id}});
            } else {
                predecessors[i].push_back(found->second); successors[found->second].push_back(i); ++degree[i];
            }
        }
    }
    std::deque<std::size_t> ready;
    for (std::size_t i = 0; i < n; ++i) if (degree[i] == 0) ready.push_back(i);
    while (!ready.empty()) {
        auto i = ready.front(); ready.pop_front(); order.push_back(i);
        for (auto next : successors[i]) if (--degree[next] == 0) ready.push_back(next);
    }
    if (order.size() != n) {
        // Follow predecessors inside the unprocessed graph to return an actual cycle, not downstream nodes.
        std::size_t current = 0;
        while (degree[current] == 0) ++current;
        std::unordered_map<std::size_t, std::size_t> seen;
        std::vector<std::size_t> chain;
        while (!seen.contains(current)) {
            seen[current] = chain.size(); chain.push_back(current);
            for (auto prev : predecessors[current]) if (degree[prev] > 0) { current = prev; break; }
        }
        std::vector<std::string> cycle;
        for (std::size_t j = seen[current]; j < chain.size(); ++j) cycle.push_back(p.tasks[chain[j]].id);
        cycle.push_back(p.tasks[current].id); std::reverse(cycle.begin(), cycle.end());
        result.diagnostics.push_back({"DEPENDENCY_CYCLE", "선행관계 순환이 있습니다. 진단의 작업 ID 경로를 확인하세요.", std::move(cycle)});
        invalid = true;
    }
    if (invalid || missing) {
        result.status = invalid ? CalculationStatus::Conflict : CalculationStatus::Incomplete; return result;
    }
    std::vector<TaskSchedule> rows(n);
    std::int64_t finish = 0;
    for (auto i : order) {
        auto& row = rows[i]; row.taskId = p.tasks[i].id;
        for (auto prev : predecessors[i]) row.earlyStartMinutes = std::max(row.earlyStartMinutes, rows[prev].earlyFinishMinutes);
        row.earlyFinishMinutes = row.earlyStartMinutes + *p.tasks[i].durationMinutes;
        finish = std::max(finish, row.earlyFinishMinutes);
        for (auto prev : predecessors[i]) if (rows[prev].earlyFinishMinutes == row.earlyStartMinutes)
            row.drivingPredecessors.push_back(p.tasks[prev].id);
    }
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        auto i = *it; auto& row = rows[i]; row.lateFinishMinutes = finish;
        auto freeEnd = finish;
        for (auto next : successors[i]) {
            row.lateFinishMinutes = std::min(row.lateFinishMinutes, rows[next].lateStartMinutes);
            freeEnd = std::min(freeEnd, rows[next].earlyStartMinutes);
        }
        row.lateStartMinutes = row.lateFinishMinutes - *p.tasks[i].durationMinutes;
        row.totalFloatMinutes = row.lateStartMinutes - row.earlyStartMinutes;
        row.freeFloatMinutes = freeEnd - row.earlyFinishMinutes; row.critical = row.totalFloatMinutes == 0;
    }
    try {
        Calendar calendar(p);
        for (std::size_t i = 0; i < n; ++i) {
            rows[i].start = calendar.boundary(rows[i].earlyStartMinutes, *p.tasks[i].durationMinutes == 0);
            rows[i].finish = calendar.boundary(rows[i].earlyFinishMinutes, true);
        }
        result.projectFinish = calendar.boundary(finish, true);
    } catch (const std::exception& e) {
        result.status = CalculationStatus::Conflict;
        result.diagnostics.push_back({"NO_AVAILABILITY_IN_HORIZON", e.what(), {}}); return result;
    }
    result.tasks = std::move(rows); result.status = CalculationStatus::Valid;
    result.diagnostics.push_back({"RESOURCE_NOT_CHECKED", "기간·FS 논리 일정입니다. 자원 가용량은 검증하지 않았습니다.", {}});
    return result;
}
}
