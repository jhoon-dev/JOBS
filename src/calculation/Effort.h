#pragma once
#include "calculation/WorkCalendar.h"
#include "calculation/Resources.h"
#include <unordered_map>
namespace jobs {
struct ExecutionInputs {
    std::unordered_map<std::string, std::vector<Assignment>> assignments;
    std::unordered_map<std::string, const Resource *> resources;
    explicit ExecutionInputs(const PlanInput &);
};
struct WorkSpan {
    std::string resourceId;
    std::int64_t start = 0, finish = 0;
    int units = 0;
};
// One execution model is shared by dates, resource demand, costs and reports.
// Civil minute intervals are intersected before work is integrated.
class TaskExecution {
    struct Weighted {
        std::int64_t start, finish, units, prefix;
    };
    Task task_;
    WorkCalendar clock_, calendar_;
    struct Grid {
        std::vector<WorkSpan> availability;
        std::vector<Weighted> weighted;
    };
    std::shared_ptr<const Grid> grid_;
    std::int64_t integrated(std::int64_t civil) const;

public:
    TaskExecution(const PlanInput &, const Task &, const ExecutionInputs *inputs = nullptr);
    const WorkCalendar &calendar() const {
        return calendar_;
    }
    TaskSchedule dates(std::int64_t localStart) const;
    std::int64_t required(const std::string &bound, bool targetFinish) const;
    std::vector<WorkSpan> spans(const TaskSchedule &) const;
    std::int64_t work(const TaskSchedule &) const;
    std::int64_t duration(const TaskSchedule &) const;
};
// Completed actual rows have no future demand. Work-driven reports require an
// explicit remaining work report; actual effort is never duration * units.
Task pendingTask(const Project &, const Task &);
std::vector<WorkSpan> executionSpans(const PlanInput &, const Task &, const TaskSchedule &);
ScheduleResult calculateEffort(const Project &, bool forecast = false, const std::string &levelingPriority = {},
                               int horizonDays = 260);
LevelingResult levelEffort(const Project &, const std::string &, int);
void changeDuration(Project &, std::size_t, std::optional<std::int64_t>, bool planOnly = false);
// Requested durations are already stored in the proposal's fixed-work tasks.
void changeFixedWorkDurations(Project &, const std::vector<std::size_t> &);
void changeWork(Project &, std::size_t, std::int64_t);
} // namespace jobs
