#include "domain/Diagnostics.h"
#include "calculation/Effort.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jobs {
namespace {
void scaleAssignments(Project &p, std::size_t index, std::int64_t target, std::int64_t duration,
                      bool planOnly = false) {
    calculationCheckpoint();
    auto t = p.tasks.at(index);
    t.taskType = TaskType::FixedDuration;
    t.workDriven = false;
    t.workUnitMinutes.reset();
    t.durationMinutes = duration;
    Project trial;
    if (planOnly) {
        static_cast<PlanInput &>(trial) = p;
        trial.documentId = p.documentId;
        trial.revision = p.revision;
    } else
        trial = p;
    trial.tasks[index] = t;
    auto plan = calculatePlan(trial);
    std::int64_t local = 0;
    TaskExecution model(p, t);
    if (plan.status != CalculationStatus::Valid)
        throw AppError(ErrorCode::INVERSE_PLAN_REQUIRED, {});
    for (const auto &row : plan.tasks)
        if (row.taskId == t.id)
            local = model.calendar().offset(row.start);
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
