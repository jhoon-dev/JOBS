#include "domain/Diagnostics.h"
#include "calculation/Effort.h"
#include "calculation/Constraints.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <numeric>
#include <tuple>
namespace jobs {
using Minute = std::int64_t;
ExecutionInputs::ExecutionInputs(const PlanInput &p) {
    for (const auto &a : p.assignments) {
        calculationCheckpoint();
        assignments[a.taskId].push_back(a);
    }
    for (const auto &r : p.resources)
        resources[r.id] = &r;
}
TaskExecution::TaskExecution(const PlanInput &p, const Task &t, const ExecutionInputs *inputs)
    : task_(t), clock_(p), calendar_(p, t) {
    EvaluationScope evaluation;
    calculationCheckpoint();
    std::vector<Assignment> assigned;
    if (inputs) {
        auto it = inputs->assignments.find(t.id);
        if (it != inputs->assignments.end())
            assigned = it->second;
    } else
        for (const auto &a : p.assignments)
            if (a.taskId == t.id)
                assigned.push_back(a);
    std::string key = "execution-grid/";
    auto field = [&](const std::string &value) { key += std::to_string(value.size()) + ":" + value; };
    field(calendar_.cacheKey());
    std::vector<WorkCalendar> resourceCalendars;
    for (const auto &a : assigned) {
        const Resource *resource = nullptr;
        if (inputs)
            resource = inputs->resources.at(a.resourceId);
        else
            for (const auto &r : p.resources)
                if (r.id == a.resourceId) {
                    resource = &r;
                    break;
                }
        if (!resource)
            throw AppError(ErrorCode::WORK_RESOURCE_REQUIRED, {});
        Task resourceTask;
        resourceTask.calendarId = resource->calendarId;
        resourceCalendars.emplace_back(p, resourceTask);
        field(a.resourceId);
        field(std::to_string(a.units));
        field(resourceCalendars.back().cacheKey());
    }
    if (auto cached = evaluationCached(key)) {
        grid_ = std::static_pointer_cast<const Grid>(cached);
        return;
    }
    auto grid = std::make_shared<Grid>();
    const auto taskTimes = calendar_.civilIntervals();
    std::map<Minute, Minute> changes;
    for (std::size_t assignment = 0; assignment < assigned.size(); ++assignment) {
        const auto &a = assigned[assignment];
        const auto resourceTimes = resourceCalendars[assignment].civilIntervals();
        std::size_t i = 0, j = 0;
        while (i < taskTimes.size() && j < resourceTimes.size()) {
            calculationCheckpoint();
            const auto s = std::max(taskTimes[i].first, resourceTimes[j].first),
                       e = std::min(taskTimes[i].second, resourceTimes[j].second);
            if (s < e) {
                grid->availability.push_back({a.resourceId, s, e, a.units});
                changes[s] += a.units;
                changes[e] -= a.units;
            }
            if (taskTimes[i].second < resourceTimes[j].second)
                ++i;
            else
                ++j;
        }
    }
    Minute units = 0, prefix = 0, last = 0;
    for (const auto &[at, delta] : changes) {
        calculationCheckpoint();
        if (units && at > last) {
            grid->weighted.push_back({last, at, units, prefix});
            prefix += (at - last) * units;
        }
        units += delta;
        last = at;
    }
    grid_ = std::move(grid);
    evaluationRemember(std::move(key), grid_);
}
Minute TaskExecution::integrated(Minute civil) const {
    const auto &weighted = grid_->weighted;
    auto it =
        std::upper_bound(weighted.begin(), weighted.end(), civil, [](Minute n, const auto &w) { return n < w.start; });
    if (it == weighted.begin())
        return 0;
    --it;
    return it->prefix + std::clamp(civil - it->start, Minute{0}, it->finish - it->start) * it->units;
}
TaskSchedule TaskExecution::dates(Minute local) const {
    calculationCheckpoint();
    const auto &weighted = grid_->weighted;
    TaskSchedule row;
    row.taskId = task_.id;
    const auto amount = usesWork(task_) ? task_.workUnitMinutes.value() : task_.durationMinutes.value();
    row.start = amount == 0 ? calendar_.pointBoundary(local) : calendar_.boundary(local, false);
    row.finish = row.start;
    if (amount > 0 && usesWork(task_)) {
        const auto first = clock_.offset(row.start);
        auto it = std::lower_bound(weighted.begin(), weighted.end(), first,
                                   [](const auto &w, Minute n) { return w.finish <= n; });
        if (it == weighted.end())
            throw AppError(ErrorCode::WORK_CALENDAR_NO_INTERSECTION, {});
        const auto begin = std::max(first, it->start), target = integrated(begin) + amount;
        row.start = clock_.boundary(begin, false);
        auto end = std::lower_bound(weighted.begin(), weighted.end(), target, [](const auto &w, Minute n) {
            return w.prefix + (w.finish - w.start) * w.units < n;
        });
        if (end == weighted.end())
            throw AppError(ErrorCode::WORK_HORIZON_EXHAUSTED, {});
        const auto finish = end->start + (target - end->prefix + end->units - 1) / end->units;
        row.finish = clock_.boundary(finish, true);
    } else if (amount > 0)
        row.finish = calendar_.boundary(local + amount, true);
    row.earlyStartMinutes = clock_.offset(row.start);
    row.earlyFinishMinutes = clock_.offset(row.finish);
    return row;
}
Minute TaskExecution::required(const std::string &bound, bool finish) const {
    if (usesWork(task_) ? task_.workUnitMinutes.value() == 0 : task_.durationMinutes.value() == 0)
        return calendar_.pointStartAtLeast(bound);
    if (!finish)
        return calendar_.startAtLeast(bound);
    // Finish-driven edges need an inverse of the work integral, not F - nominal D.
    Minute lo = 0, hi = calendar_.maxOffset();
    while (lo < hi) {
        auto mid = lo + (hi - lo) / 2;
        bool reached = true;
        try {
            reached = dates(mid).finish >= bound;
        } catch (const std::exception &) {
        }
        if (reached)
            hi = mid;
        else
            lo = mid + 1;
    }
    return lo;
}
Minute TaskExecution::offset(const std::string &stamp) const {
    const bool zero = usesWork(task_) ? task_.workUnitMinutes.value() == 0 : task_.durationMinutes.value() == 0;
    return zero ? calendar_.pointOffset(stamp) : calendar_.offset(stamp);
}
std::vector<WorkSpan> TaskExecution::spans(const TaskSchedule &row) const {
    if (row.completedActual)
        return {};
    const auto begin = clock_.offset(row.remainingStart.value_or(row.start)), end = clock_.offset(row.finish);
    std::vector<WorkSpan> out;
    for (const auto &a : grid_->availability)
        if (a.start < end && a.finish > begin)
            out.push_back({a.resourceId, std::max(begin, a.start), std::min(end, a.finish), a.units});
    if (usesWork(task_) && end > begin) {
        const auto target = *task_.workUnitMinutes;
        const auto before = integrated(end - 1) - integrated(begin), last = target - before,
                   total = integrated(end) - integrated(end - 1);
        if (last < 0 || last > total)
            throw AppError(ErrorCode::WORK_RESULT_MISMATCH, {});
        if (last < total) {
            std::vector<WorkSpan> kept, tail;
            Minute allocated = 0;
            for (const auto &a : out) {
                if (a.finish < end) {
                    kept.push_back(a);
                    continue;
                }
                if (a.start < end - 1)
                    kept.push_back({a.resourceId, a.start, end - 1, a.units});
                const int units = int(Minute(a.units) * last / total);
                allocated += units;
                tail.push_back({a.resourceId, end - 1, end, units});
            }
            std::sort(tail.begin(), tail.end(),
                      [](const auto &a, const auto &b) { return a.resourceId < b.resourceId; });
            for (auto &a : tail)
                if (allocated < last) {
                    ++a.units;
                    ++allocated;
                }
            for (const auto &a : tail)
                if (a.units)
                    kept.push_back(a);
            out = std::move(kept);
        }
    }
    std::sort(out.begin(), out.end(), [](const auto &a, const auto &b) {
        return std::tie(a.resourceId, a.start) < std::tie(b.resourceId, b.start);
    });
    return out;
}
Minute TaskExecution::work(const TaskSchedule &row) const {
    Minute total = 0;
    for (const auto &a : spans(row))
        total += (a.finish - a.start) * a.units;
    return total;
}
Minute TaskExecution::duration(const TaskSchedule &row) const {
    return calendar_.offset(row.finish) - calendar_.offset(row.remainingStart.value_or(row.start));
}
Task pendingTask(const Project &p, const Task &t) {
    auto pending = t;
    if (!p.statusAt)
        return pending;
    const auto *e = progressAsOf(p, t.id);
    if (!e)
        return pending;
    if (usesWork(t)) {
        if (e->actualFinish)
            pending.workUnitMinutes = 0;
        else if (e->remainingWorkUnitMinutes)
            pending.workUnitMinutes = e->remainingWorkUnitMinutes;
        else
            throw AppError(ErrorCode::REMAINING_WORK_REQUIRED, {diagnosticArgument(t.name)});
    } else {
        if (!e->actualFinish && e->remainingMinutes == 0 && e->remainingWorkUnitMinutes)
            throw AppError(ErrorCode::REMAINING_DURATION_REQUIRED, {diagnosticArgument(t.name)});
        pending.durationMinutes = e->remainingMinutes;
    }
    return pending;
}
std::vector<WorkSpan> executionSpans(const PlanInput &p, const Task &t, const TaskSchedule &row) {
    return TaskExecution(p, t).spans(row);
}
} // namespace jobs
