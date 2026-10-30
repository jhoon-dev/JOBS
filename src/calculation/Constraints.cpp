#include "domain/Diagnostics.h"
#include "calculation/Constraints.h"
#include "formatting/PlanTokens.h"
#include "calculation/EnginePolicy.h"
#include <algorithm>
#include <map>
#include <unordered_map>
#include <stdexcept>
namespace jobs {
ConstraintWindow constraintWindow(const PlanInput &p, const Task &t, const WorkCalendar &cal, std::int64_t duration) {
    ConstraintWindow w;
    if (!t.constraintAt)
        return w;
    const auto &at = *t.constraintAt;
    const auto &type = t.constraintType;
    const bool start = type == ConstraintType::SNET || type == ConstraintType::SNLT || type == ConstraintType::MSO;
    const bool fixed = type == ConstraintType::MSO || type == ConstraintType::MFO;
    const bool upper = type == ConstraintType::SNLT || type == ConstraintType::FNLT;
    if (at < p.startDate + " 00:00") {
        if (fixed || upper)
            w.upper = -1;
        return w;
    }
    if (duration == 0 && !legacyMilestones()) {
        try {
            if (upper)
                w.upper = cal.pointEndAtMost(at);
            else {
                w.lower = cal.pointStartAtLeast(at);
                if (fixed) {
                    if (cal.pointBoundary(w.lower) != at)
                        throw AppError(ErrorCode::CONSTRAINT_BOUNDARY_INVALID, {at});
                    w.upper = w.lower;
                }
            }
        } catch (const AppError &error) {
            if (upper && error.code() == "CALENDAR_TIMESTAMP_RANGE")
                w.upper = cal.maxPointOffset();
            else
                throw;
        }
        return w;
    }
    std::int64_t value;
    try {
        value = cal.offset(at);
    } catch (const std::exception &) {
        if (upper) {
            w.upper = cal.maxOffset() - duration;
            return w;
        }
        throw;
    }
    if (fixed) {
        auto local = value - (start ? 0 : duration);
        if (local < 0 || cal.boundary(value, !start || duration == 0) != at)
            throw AppError(ErrorCode::CONSTRAINT_BOUNDARY_INVALID, {diagnosticArgument(at)});
        w.lower = local;
        w.upper = local;
    } else if (upper) {
        auto local = value - (start ? 0 : duration);
        if (local >= 0 && cal.boundary(start ? local : local + duration, !start || duration == 0) > at)
            --local;
        w.upper = local;
    } else {
        w.lower = start ? (duration ? cal.startAtLeast(at) : cal.finishAtLeast(at))
                        : std::max(std::int64_t{0}, cal.finishAtLeast(at) - duration);
    }
    return w;
}
std::optional<Diagnostic> timingViolation(const Task &t, const TaskSchedule &row) {
    if (!t.constraintAt)
        return {};
    const auto &type = t.constraintType;
    const bool start = type == ConstraintType::SNET || type == ConstraintType::SNLT || type == ConstraintType::MSO;
    const auto &actual = start ? row.start : row.finish;
    const auto &bound = *t.constraintAt;
    const bool bad = type == ConstraintType::MSO || type == ConstraintType::MFO     ? actual != bound
                     : type == ConstraintType::SNET || type == ConstraintType::FNET ? actual < bound
                                                                                    : actual > bound;
    if (!bad)
        return {};
    auto ids = row.drivingPredecessors;
    ids.insert(ids.begin(), t.id);
    return Diagnostic{"CONSTRAINT_CONFLICT", {t.name, enumToken(type), bound, actual}, std::move(ids)};
}
std::optional<std::int64_t> deadlineSlack(const PlanInput &p, const Task &t, const std::string &finish) {
    if (!t.deadlineAt)
        return {};
    try {
        WorkCalendar cal(p, t);
        const auto due = *t.deadlineAt < p.startDate + " 00:00" ? 0 : cal.offset(*t.deadlineAt);
        return due - cal.offset(finish);
    } catch (const std::exception &) {
        return {};
    }
}
std::vector<Diagnostic> deadlineDiagnostics(const PlanInput &p, const std::vector<TaskSchedule> &rows) {
    std::vector<Diagnostic> out;
    for (const auto &t : p.tasks)
        if (t.deadlineAt) {
            auto it = std::find_if(rows.begin(), rows.end(), [&](const auto &r) { return r.taskId == t.id; });
            if (it == rows.end() || it->finish <= *t.deadlineAt)
                continue;
            const auto slack = deadlineSlack(p, t, it->finish);
            out.push_back(
                {"DEADLINE_MISSED", {t.name, *t.deadlineAt, it->finish, slack ? std::to_string(*slack) : "?"}, {t.id}});
        }
    return out;
}
ScheduleResult calculateConstrainedPlan(const Project &p, const std::vector<std::size_t> &order) {
    ScheduleResult out;
    out.documentId = p.documentId;
    out.inputRevision = p.revision;
    out.profile = p.calculationProfile;
    out.engineVersion = calculationEngineFor(p);
    out.calculationPolicy = "constrained-asap-alap-v1";
    std::string currentTask;
    try {
        WorkCalendar clock(p);
        std::map<std::string, WorkCalendar> calendars;
        std::unordered_map<std::string, std::size_t> ids;
        for (std::size_t i = 0; i < p.tasks.size(); ++i) {
            ids[p.tasks[i].id] = i;
            if (!calendars.contains(p.tasks[i].calendarId))
                calendars.emplace(p.tasks[i].calendarId, WorkCalendar(p, p.tasks[i]));
        }
        const auto n = p.tasks.size();
        std::vector<TaskSchedule> rows(n);
        std::vector<ConstraintWindow> bounds(n);
        std::vector<std::int64_t> early(n), late(n);
        std::vector<std::vector<std::pair<std::size_t, Dependency>>> next(n);
        std::string finish = p.startDate + " 00:00";
        std::size_t completed = 0;
        for (auto i : order) {
            calculationProgress(CalculationStage::Forward, completed++, n);
            currentTask = p.tasks[i].id;
            const auto &t = p.tasks[i];
            const auto &cal = calendars.at(t.calendarId);
            auto &row = rows[i];
            row.taskId = t.id;
            try {
                bounds[i] = constraintWindow(p, t, cal, *t.durationMinutes);
            } catch (const std::exception &e) {
                auto issue = diagnosticFromException(e, "CONSTRAINT_CALENDAR", {t.id});
                issue.calendarIds = {t.calendarId};
                out.diagnostics.push_back(std::move(issue));
                continue;
            }
            early[i] = bounds[i].lower;
            for (const auto &edge : dependencies(t)) {
                const auto prev = ids.at(edge.predecessorId);
                if (rows[prev].start.empty())
                    continue;
                early[i] = std::max(early[i], pdmRequiredStart(cal, edge, rows[prev], *t.durationMinutes));
                next[prev].push_back({i, edge});
            }
            pdmDates(clock, cal, row, early[i], *t.durationMinutes);
            for (const auto &edge : dependencies(t))
                if (!rows[ids.at(edge.predecessorId)].start.empty() &&
                    pdmRequiredStart(cal, edge, rows[ids.at(edge.predecessorId)], *t.durationMinutes) == early[i])
                    row.drivingPredecessors.push_back(edge.predecessorId);
            if (auto error = timingViolation(t, row))
                out.diagnostics.push_back(*error);
            finish = std::max(finish, row.finish);
        }
        if (!out.diagnostics.empty()) {
            out.status = CalculationStatus::Conflict;
            return out;
        }
        auto latest = [&](std::size_t i, const std::vector<std::int64_t> &targets) {
            currentTask = p.tasks[i].id;
            const auto &t = p.tasks[i];
            const auto &cal = calendars.at(t.calendarId);
            auto cap = pdmLatestStart(cal, finish, *t.durationMinutes);
            if (bounds[i].upper)
                cap = std::min(cap, *bounds[i].upper);
            auto lo = early[i], hi = std::max(early[i], cap);
            while (lo < hi) {
                const auto mid = lo + (hi - lo + 1) / 2;
                TaskSchedule trial;
                pdmDates(clock, cal, trial, mid, *t.durationMinutes);
                bool fits = true;
                try {
                    for (const auto &[j, edge] : next[i])
                        if (pdmRequiredStart(calendars.at(p.tasks[j].calendarId), edge, trial,
                                             *p.tasks[j].durationMinutes) > targets[j]) {
                            fits = false;
                            break;
                        }
                } catch (const std::exception &) {
                    fits = false;
                }
                if (fits)
                    lo = mid;
                else
                    hi = mid - 1;
            }
            return lo;
        };
        // Reverse order places ALAP chains as late as their already placed successors permit.
        for (auto it = order.rbegin(); it != order.rend(); ++it)
            if (p.tasks[*it].constraintType == ConstraintType::ALAP) {
                const auto i = *it;
                early[i] = latest(i, early);
                pdmDates(clock, calendars.at(p.tasks[i].calendarId), rows[i], early[i], *p.tasks[i].durationMinutes);
            }
        for (auto it = order.rbegin(); it != order.rend(); ++it) {
            calculationProgress(CalculationStage::Backward, std::uint64_t(it - order.rbegin()), n);
            const auto i = *it;
            auto &row = rows[i];
            late[i] = latest(i, late);
            TaskSchedule trial;
            pdmDates(clock, calendars.at(p.tasks[i].calendarId), trial, late[i], *p.tasks[i].durationMinutes);
            row.lateStartMinutes = trial.earlyStartMinutes;
            row.lateFinishMinutes = trial.earlyFinishMinutes;
            const auto &cal = calendars.at(p.tasks[i].calendarId);
            row.totalFloatMinutes = pdmFloat(cal, early[i], late[i], *p.tasks[i].durationMinutes);
            row.freeFloatMinutes = pdmFloat(cal, early[i], latest(i, early), *p.tasks[i].durationMinutes);
            row.critical = row.totalFloatMinutes == 0;
            row.drivingPredecessors.clear();
            for (const auto &edge : dependencies(p.tasks[i]))
                if (pdmRequiredStart(calendars.at(p.tasks[i].calendarId), edge, rows[ids.at(edge.predecessorId)],
                                     *p.tasks[i].durationMinutes) == early[i])
                    row.drivingPredecessors.push_back(edge.predecessorId);
        }
        out.tasks = std::move(rows);
        out.projectFinish = finish;
        out.status = CalculationStatus::Valid;
        out.diagnostics = deadlineDiagnostics(p, out.tasks);
        out.diagnostics.push_back({"CONSTRAINT_POLICY", {}, {}, "CONSTRAINT_POLICY"});
    } catch (const std::exception &e) {
        out.status = CalculationStatus::Conflict;
        out.tasks.clear();
        out.projectFinish.reset();
        out.diagnostics.push_back(diagnosticFromException(e, "CONSTRAINT_HORIZON",
                                                          currentTask.empty() ? std::vector<std::string>{}
                                                                              : std::vector<std::string>{currentTask}));
    }
    return out;
}
} // namespace jobs
