#pragma once
#include "domain/Project.h"

namespace jobs {
enum class CalculationStatus { Valid, Incomplete, Conflict, Stale };
struct Diagnostic {
    std::string code;
    std::string message;
    std::vector<std::string> taskIds;
};
struct TaskSchedule {
    std::string taskId;
    std::int64_t earlyStartMinutes = 0, earlyFinishMinutes = 0;
    std::int64_t lateStartMinutes = 0, lateFinishMinutes = 0;
    std::int64_t totalFloatMinutes = 0, freeFloatMinutes = 0;
    bool critical = false;
    std::string start, finish;
    std::vector<std::string> drivingPredecessors;
    bool operator==(const TaskSchedule&) const = default;
};
struct ScheduleResult {
    std::string documentId;
    std::uint64_t inputRevision = 0;
    std::string engineVersion = "jobs-cpm-0.1";
    std::string profile = "native-fs0-workminutes-v1";
    CalculationStatus status = CalculationStatus::Incomplete;
    std::optional<std::string> projectFinish;
    std::vector<TaskSchedule> tasks;
    std::vector<Diagnostic> diagnostics;
};
// Pure, deterministic calculation on an immutable snapshot. No Qt, I/O or OS clock.
ScheduleResult calculate(const Project& snapshot);
}
