#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace jobs {
struct TaskSchedule {
    std::string taskId;
    std::int64_t earlyStartMinutes = 0, earlyFinishMinutes = 0;
    std::int64_t lateStartMinutes = 0, lateFinishMinutes = 0;
    std::int64_t totalFloatMinutes = 0, freeFloatMinutes = 0;
    bool critical = false;
    std::string start, finish;
    std::vector<std::string> drivingPredecessors;
    std::optional<std::string> remainingStart;
    bool completedActual = false;
    bool operator==(const TaskSchedule&) const = default;
};
}
