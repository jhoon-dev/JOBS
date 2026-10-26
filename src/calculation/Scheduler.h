#pragma once
#include "domain/Project.h"

namespace jobs {
enum class CalculationStatus { Valid, Incomplete, Conflict, Stale };
struct Diagnostic {
    std::string code;
    std::string message;
    std::vector<std::string> taskIds;
};
struct ScheduleResult {
    std::string documentId;
    std::uint64_t inputRevision = 0;
    std::string engineVersion = "jobs-cpm-0.2";
    std::string profile = "native-fs0-workminutes-v1";
    CalculationStatus status = CalculationStatus::Incomplete;
    std::optional<std::string> projectFinish;
    std::vector<TaskSchedule> tasks;
    std::vector<Diagnostic> diagnostics;
    bool isForecast = false;
    std::optional<std::string> statusAt;
    std::string calculationPolicy = "plan-asap-v1";
};
// Pure, deterministic calculation on an immutable snapshot. No Qt, I/O or OS clock.
ScheduleResult calculate(const Project& snapshot);
ScheduleResult calculatePlan(const Project& snapshot);
struct WbsSummary {
    std::size_t taskCount = 0, missingDurations = 0;
    std::int64_t knownDurationMinutes = 0;
    std::optional<std::string> start, finish;
};
WbsSummary summarizeWbs(const Project& project, const ScheduleResult* result, const std::string& wbsId);
}
