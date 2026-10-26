#pragma once
#include "calculation/Scheduler.h"
namespace jobs {
Baseline prepareBaseline(const Project& project, std::string name, std::string approvedBy, std::string reason, std::string createdAt);
void verifyBaseline(const Baseline& baseline);
struct TaskComparison {
    std::string taskId, name;
    std::optional<std::string> baselineStart, baselineFinish, planStart, planFinish, forecastStart, forecastFinish;
    std::optional<std::string> actualStart, actualFinish;
    std::optional<std::int64_t> actualMinutes, remainingMinutes, planFinishVariance, forecastFinishVariance;
    std::optional<double> durationProgressPercent;
    std::vector<std::string> changes;
    bool reported = false;
    std::string baselineName, baselineWbsPath, planWbsPath;
    std::optional<std::int64_t> baselineDuration, planDuration;
    std::vector<std::string> baselinePredecessors, planPredecessors;
};
struct ComparisonReport {
    std::string documentId, baselineId;
    std::uint64_t revision = 0;
    ScheduleResult plan, forecast;
    std::optional<std::string> baselineFinish;
    std::optional<std::int64_t> planFinishVariance, forecastFinishVariance;
    std::size_t reportedTasks = 0, totalTasks = 0;
    std::vector<TaskComparison> tasks;
    std::vector<std::string> contextChanges;
};
ComparisonReport compareToBaseline(const Project& project, const std::string& baselineId);
}
