#pragma once
#include "calculation/Scheduler.h"
namespace jobs {
struct ResourceDay {
    std::string resourceId, date;
    std::optional<int> capacity;
    std::int64_t peakUnits = 0, demandUnitMinutes = 0, overloadMinutes = 0, excessUnitMinutes = 0;
    std::vector<std::string> taskIds;
};
struct ResourceLoad {
    CalculationStatus status = CalculationStatus::Incomplete;
    std::vector<ResourceDay> days;
    std::vector<Diagnostic> diagnostics;
    std::size_t assignedTasks = 0, totalTasks = 0;
};
struct LevelingResult {
    ScheduleResult original, leveled;
    ResourceLoad before, after;
    std::string priority;
    int horizonWorkDays = 0;
    std::vector<Diagnostic> diagnostics;
};
// Forecast mode measures remaining work only; it never invents historical effort.
ResourceLoad resourceLoad(const Project&, const ScheduleResult&);
LevelingResult levelResources(const Project&, const std::string& priority, int horizonWorkDays);
Project scenarioProject(const LevelingScenario&);
}
