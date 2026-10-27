#pragma once
#include "domain/Project.h"
#include <variant>

namespace jobs {
struct TaskEdit {
    std::string taskId;
    std::optional<std::string> name;
    std::optional<std::optional<std::int64_t>> duration;
    std::optional<std::vector<Dependency>> edges;
};
struct DurationEdit { std::string taskId; std::optional<std::int64_t> minutes; };
struct EffortEdit {
    std::string taskId, type;
    bool workInput = false;
    std::optional<std::int64_t> duration, work;
    std::vector<TaskBreak> breaks;
};
struct AssignmentEdit { std::string taskId; std::vector<Assignment> assignments; };
struct ProgressEdit { ProgressEntry entry; };
struct BatchTaskEdit { std::vector<TaskEdit> changes; };
struct GanttTaskEdit { std::string taskId; bool resize; std::string target; };
using EditCommand = std::variant<DurationEdit, EffortEdit, AssignmentEdit, ProgressEdit, BatchTaskEdit, GanttTaskEdit>;
enum class EditState { Ready, Applied, NoChange, Cancelled, Stale, Failed };
}
