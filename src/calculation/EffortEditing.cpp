#include "domain/Diagnostics.h"
#include "calculation/Effort.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jobs {
namespace {
void scaleAssignments(Project &p, std::size_t index, std::int64_t target, std::int64_t duration, bool planOnly = false,
                      const TaskSchedule *source = nullptr) {
    calculationCheckpoint();
    auto t = p.tasks.at(index);
    t.taskType = TaskType::FixedDuration;
    t.workDriven = false;
    t.workUnitMinutes.reset();
    t.durationMinutes = duration;
    TaskSchedule scheduled;
    if (!source) {
        Project trial;
        if (planOnly) {
            static_cast<PlanInput &>(trial) = p;
            trial.documentId = p.documentId;
            trial.revision = p.revision;
        } else
            trial = p;
        trial.tasks[index] = t;
        const auto plan = calculatePlan(trial);
        if (plan.status != CalculationStatus::Valid)
            throw AppError(ErrorCode::INVERSE_PLAN_REQUIRED, {});
        const auto row =
            std::find_if(plan.tasks.begin(), plan.tasks.end(), [&](const auto &r) { return r.taskId == t.id; });
        if (row == plan.tasks.end())
            throw AppError(ErrorCode::INVERSE_PLAN_REQUIRED, {});
        scheduled = *row;
        source = &scheduled;
    }
    TaskExecution model(p, t);
    const auto local = model.calendar().offset(source->start);
    const auto work = model.work(model.dates(local));
    if (target && work == 0)
        throw AppError(ErrorCode::INVERSE_CAPACITY_REQUIRED, {});
    for (auto &a : p.assignments) {
        calculationCheckpoint();
        if (a.taskId == t.id) {
            const auto units = work ? std::llround(static_cast<long double>(a.units) * target / work) : 0;
            if (units > 100000)
                throw AppError(ErrorCode::INVERSE_UNITS_LIMIT, {});
            a.units = int(target ? std::max(1LL, units) : 0);
        }
    }
    std::erase_if(p.assignments, [&](const auto &a) { return a.taskId == t.id && !a.units; });
}
} // namespace
void changeFixedWorkDurations(Project &p, const std::vector<std::size_t> &indices) {
    if (indices.empty())
        return;
    auto current = indices.front();
    try {
        Project trial;
        static_cast<PlanInput &>(trial) = p;
        trial.documentId = p.documentId;
        trial.revision = p.revision;
        // Every requested duration participates in the same schedule, including
        // predecessors that appear below their successors in the task table.
        for (const auto index : indices) {
            calculationCheckpoint();
            current = index;
            auto &task = trial.tasks.at(index);
            if (task.taskType != TaskType::FixedWork || !task.durationMinutes ||
                (*task.durationMinutes == 0 && task.workUnitMinutes.value_or(0) > 0))
                throw AppError(ErrorCode::FIXED_WORK_DURATION_REQUIRED, {});
            task.taskType = TaskType::FixedDuration;
            task.workDriven = false;
            task.workUnitMinutes.reset();
        }
        current = indices.front();
        const auto plan = calculatePlan(trial);
        if (plan.status != CalculationStatus::Valid)
            throw AppError(ErrorCode::INVERSE_PLAN_REQUIRED, {});
        std::unordered_map<std::string, const TaskSchedule *> rows;
        for (const auto &row : plan.tasks)
            rows.emplace(row.taskId, &row);
        for (const auto index : indices) {
            current = index;
            const auto &task = p.tasks.at(index);
            scaleAssignments(p, index, task.workUnitMinutes.value(), task.durationMinutes.value(), true,
                             rows.at(task.id));
        }
    } catch (const std::exception &error) {
        const auto &task = p.tasks.at(current);
        auto diagnostic = diagnosticFromException(error, "TASK_EDIT_FAILED", {task.id});
        diagnostic.arguments = {task.name};
        throw AppError(std::move(diagnostic));
    }
}
void changeDuration(Project &p, std::size_t index, std::optional<std::int64_t> duration, bool planOnly) {
    auto &t = p.tasks.at(index);
    if (isEffort(p) && t.taskType == TaskType::FixedWork) {
        if (!duration || (*duration == 0 && t.workUnitMinutes.value_or(0) > 0))
            throw AppError(ErrorCode::FIXED_WORK_DURATION_REQUIRED, {});
        scaleAssignments(p, index, t.workUnitMinutes.value(), *duration, planOnly);
    } else if (t.taskType == TaskType::FixedUnits) {
        t.workDriven = false;
        t.workUnitMinutes.reset();
    }
    t.durationMinutes = duration;
}
void changeWork(Project &p, std::size_t index, std::int64_t work) {
    if (work < 0 || work > maxWork)
        throw AppError(ErrorCode::WORK_INPUT_LIMIT, {});
    p.calculationProfile = effortProfile;
    auto &t = p.tasks.at(index);
    if (t.taskType == TaskType::FixedDuration) {
        if (!t.durationMinutes)
            throw AppError(ErrorCode::FIXED_DURATION_REQUIRED, {});
        scaleAssignments(p, index, work, *t.durationMinutes);
    } else {
        t.workUnitMinutes = work;
        t.workDriven = true;
    }
}
} // namespace jobs
