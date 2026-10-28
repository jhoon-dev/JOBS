#include "calculation/Pdm.h"
#include "calculation/Constraints.h"
#include <algorithm>
#include <map>
#include <deque>
#include <unordered_map>
namespace jobs {
std::int64_t pdmRequiredStart(const WorkCalendar &cal, const Dependency &edge, const TaskSchedule &predecessor,
                              std::int64_t duration) {
    const auto source = dependencyFromFinish(edge.type) ? predecessor.finish : predecessor.start;
    const auto bound = cal.shift(source, edge.lagMinutes);
    if (duration == 0)
        return cal.finishAtLeast(bound);
    if (!dependencyToFinish(edge.type))
        return cal.startAtLeast(bound);
    return std::max(std::int64_t{0}, cal.finishAtLeast(bound) - duration);
}
void pdmDates(const WorkCalendar &clock, const WorkCalendar &cal, TaskSchedule &row, std::int64_t local,
              std::int64_t duration) {
    row.start = cal.boundary(local, duration == 0);
    row.finish = duration ? cal.boundary(local + duration, true) : row.start;
    row.earlyStartMinutes = clock.offset(row.start);
    row.earlyFinishMinutes = clock.offset(row.finish);
}
namespace {
struct Calendars {
    std::map<std::string, WorkCalendar> values;
    explicit Calendars(const PlanInput &p) {
        for (const auto &t : p.tasks)
            if (!values.contains(t.calendarId))
                values.emplace(t.calendarId, WorkCalendar(p, t));
    }
    const WorkCalendar &at(const Task &t) const {
        return values.at(t.calendarId);
    }
};
} // namespace
ScheduleResult calculatePdmPlan(const Project &p, const std::vector<std::size_t> &order) {
    ScheduleResult out;
    out.documentId = p.documentId;
    out.inputRevision = p.revision;
    out.profile = p.calculationProfile;
    out.engineVersion = "jobs-pdm-1.0";
    try {
        WorkCalendar clock(p);
        Calendars calendars(p);
        const auto n = p.tasks.size();
        std::unordered_map<std::string, std::size_t> ids;
        for (std::size_t i = 0; i < n; ++i)
            ids[p.tasks[i].id] = i;
        std::vector<TaskSchedule> rows(n);
        std::vector<std::int64_t> early(n), late(n);
        std::vector<std::vector<std::pair<std::size_t, Dependency>>> next(n);
        std::string finish = p.startDate + " 00:00";
        std::size_t completed = 0;
        for (auto i : order) {
            calculationProgress(CalculationStage::Forward, completed++, n);
            const auto &task = p.tasks[i];
            const auto &cal = calendars.at(task);
            const auto duration = *task.durationMinutes;
            auto &row = rows[i];
            row.taskId = task.id;
            auto &start = early[i];
            for (const auto &d : dependencies(task)) {
                const auto prev = ids.at(d.predecessorId);
                start = std::max(start, pdmRequiredStart(cal, d, rows[prev], duration));
                next[prev].push_back({i, d});
            }
            pdmDates(clock, cal, row, start, duration);
            finish = std::max(finish, row.finish);
            for (const auto &d : dependencies(task))
                if (pdmRequiredStart(cal, d, rows[ids.at(d.predecessorId)], duration) == start)
                    row.drivingPredecessors.push_back(d.predecessorId);
        }
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            calculationProgress(CalculationStage::Backward, std::uint64_t(it - order.rbegin()), n);
            const auto i = *it;
            const auto &task = p.tasks[i];
            const auto &cal = calendars.at(task);
            const auto duration = *task.durationMinutes;
            auto cap = std::max(early[i], cal.offset(finish) - duration);
            auto latest = [&](bool useLate) {
                auto lo = early[i], hi = cap;
                while (lo < hi) {
                    const auto mid = lo + (hi - lo + 1) / 2;
                    TaskSchedule trial;
                    pdmDates(clock, cal, trial, mid, duration);
                    bool fits = true;
                    try {
                        for (const auto &[j, edge] : next[i])
                            if (pdmRequiredStart(calendars.at(p.tasks[j]), edge, trial, *p.tasks[j].durationMinutes) >
                                (useLate ? late[j] : early[j])) {
                                fits = false;
                                break;
                            }
                    } catch (const std::exception &) {
                        fits = false;
                    } // A trial outside the horizon is infeasible, not a failed early schedule.
                    if (fits)
                        lo = mid;
                    else
                        hi = mid - 1;
                }
                return lo;
            };
            late[i] = latest(true);
            auto &row = rows[i];
            TaskSchedule dates;
            pdmDates(clock, cal, dates, late[i], duration);
            row.lateStartMinutes = dates.earlyStartMinutes;
            row.lateFinishMinutes = dates.earlyFinishMinutes;
            row.totalFloatMinutes = late[i] - early[i];
            row.freeFloatMinutes = latest(false) - early[i];
            row.critical = row.totalFloatMinutes == 0;
        }
        out.tasks = std::move(rows);
        out.projectFinish = finish;
        out.status = CalculationStatus::Valid;
        out.diagnostics.push_back({"PDM_POLICY",
                                   "FS·SS·FF·SF 하한 관계 · 지연/선행=후행 달력 근무분 · 시작일 이전 배치 없음 · "
                                   "여유=작업 달력 근무분 · 자원 미검증",
                                   {}});
    } catch (const std::exception &e) {
        out.status = CalculationStatus::Conflict;
        out.diagnostics.push_back({"NO_AVAILABILITY_IN_HORIZON", e.what(), {}});
    }
    return out;
}
ScheduleResult calculatePdmForecast(const Project &p, ScheduleResult out) {
    out.tasks.clear();
    out.projectFinish.reset();
    out.diagnostics.clear();
    out.isForecast = true;
    out.statusAt = p.statusAt;
    out.calculationPolicy =
        p.calculationProfile == constraintProfile ? "retained-remaining-constrained-v1" : "retained-remaining-pdm-v1";
    try {
        WorkCalendar clock(p);
        Calendars calendars(p);
        const auto n = p.tasks.size();
        std::unordered_map<std::string, std::size_t> ids;
        for (std::size_t i = 0; i < n; ++i)
            ids[p.tasks[i].id] = i;
        std::vector<std::vector<std::size_t>> next(n);
        std::vector<std::size_t> degree(n);
        std::deque<std::size_t> ready;
        for (std::size_t i = 0; i < n; ++i) {
            for (const auto &id : p.tasks[i].predecessors) {
                next[ids.at(id)].push_back(i);
                ++degree[i];
            }
            if (!degree[i])
                ready.push_back(i);
        }
        std::vector<TaskSchedule> rows(n);
        std::size_t unreported = 0;
        std::size_t completed = 0;
        for (; !ready.empty(); ready.pop_front()) {
            calculationProgress(CalculationStage::Forward, completed++, n);
            const auto i = ready.front();
            const auto &t = p.tasks[i];
            const auto &cal = calendars.at(t);
            auto &row = rows[i];
            row.taskId = t.id;
            const auto *e = progressAsOf(p, t.id);
            if (!e)
                ++unreported;
            if (e && e->actualFinish) {
                row.start = *e->actualStart;
                row.finish = *e->actualFinish;
                row.completedActual = true;
                row.earlyStartMinutes = clock.offset(row.start);
                row.earlyFinishMinutes = clock.offset(row.finish);
            } else {
                const auto duration = e ? e->remainingMinutes : *t.durationMinutes;
                auto local = duration ? cal.startAtLeast(*p.statusAt) : cal.finishAtLeast(*p.statusAt);
                if (p.calculationProfile == constraintProfile) {
                    auto remainingTask = t;
                    if (e && e->actualStart &&
                        (t.constraintType == ConstraintType::SNET || t.constraintType == ConstraintType::SNLT ||
                         t.constraintType == ConstraintType::MSO)) {
                        remainingTask.constraintType = ConstraintType::ASAP;
                        remainingTask.constraintAt.reset();
                    }
                    local = std::max(local, constraintWindow(p, remainingTask, cal, duration).lower);
                }
                for (const auto &d : dependencies(t))
                    local = std::max(local, pdmRequiredStart(cal, d, rows[ids.at(d.predecessorId)], duration));
                pdmDates(clock, cal, row, local, duration);
                row.remainingStart = row.start;
                if (e && e->actualStart) {
                    row.start = *e->actualStart;
                    row.earlyStartMinutes = clock.offset(row.start);
                }
            }
            if (e)
                for (const auto &d : dependencies(t)) {
                    const auto &prev = rows[ids.at(d.predecessorId)];
                    const auto target = !dependencyToFinish(d.type) ? e->actualStart : e->actualFinish;
                    if (target &&
                        *target < cal.shift(dependencyFromFinish(d.type) ? prev.finish : prev.start, d.lagMinutes))
                        out.diagnostics.push_back(
                            {"OUT_OF_SEQUENCE",
                             "실제 날짜가 선행관계 하한보다 빠릅니다. 실제는 보존하며 잔여에 관계를 적용합니다.",
                             {d.predecessorId, t.id}});
                }
            if (!out.projectFinish || row.finish > *out.projectFinish)
                out.projectFinish = row.finish;
            for (auto j : next[i])
                if (!--degree[j])
                    ready.push_back(j);
        }
        out.tasks = std::move(rows);
        out.status = CalculationStatus::Valid;
        if (p.calculationProfile == constraintProfile) {
            for (std::size_t i = 0; i < n; ++i)
                if (auto error = timingViolation(p.tasks[i], out.tasks[i])) {
                    out.diagnostics.push_back(*error);
                    out.status = CalculationStatus::Conflict;
                }
            auto deadlines = deadlineDiagnostics(p, out.tasks);
            out.diagnostics.insert(out.diagnostics.end(), deadlines.begin(), deadlines.end());
            if (out.status == CalculationStatus::Conflict)
                out.projectFinish.reset();
            out.calculationPolicy = "retained-remaining-constrained-v1";
            if (std::any_of(p.tasks.begin(), p.tasks.end(),
                            [](const auto &t) { return t.constraintType == ConstraintType::ALAP; }))
                out.diagnostics.push_back({"CONSTRAINT_POLICY",
                                           "잔여 예측은 상태일 이후 가장 이른 재개를 사용합니다. ALAP은 계획 모드에만 "
                                           "적용하며 예측의 늦은 날짜/여유는 미산정입니다.",
                                           {}});
        }
        if (unreported)
            out.diagnostics.push_back(
                {"UNREPORTED_REMAINING", "미보고 작업은 현재 계획 기간을 잔여로 사용합니다.", {}});
        out.diagnostics.push_back(
            {"FORECAST_POLICY",
             "실제 날짜 고정 · 상태일 이후 잔여에 PDM 관계/작업 달력 적용 · 예측 여유/임계 미산정",
             {}});
    } catch (const std::exception &e) {
        out.status = CalculationStatus::Conflict;
        out.tasks.clear();
        out.projectFinish.reset();
        out.diagnostics.push_back({"FORECAST_HORIZON", e.what(), {}});
    }
    return out;
}
} // namespace jobs
