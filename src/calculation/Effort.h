#pragma once
#include "calculation/WorkCalendar.h"
#include "calculation/Resources.h"
namespace jobs {
struct WorkSpan {
    std::string resourceId;
    std::int64_t start=0, finish=0;
    int units=0;
};
// One execution model is shared by dates, resource demand, costs and reports.
// Civil minute intervals are intersected before work is integrated.
class TaskExecution {
    struct Weighted { std::int64_t start,finish,units,prefix; };
    Task task_;
    WorkCalendar clock_,calendar_;
    std::vector<WorkSpan> availability_;
    std::vector<Weighted> weighted_;
    std::int64_t integrated(std::int64_t civil) const;
public:
    TaskExecution(const PlanInput&,const Task&);
    const WorkCalendar& calendar() const { return calendar_; }
    TaskSchedule dates(std::int64_t localStart) const;
    std::int64_t required(const std::string& bound,bool targetFinish) const;
    std::vector<WorkSpan> spans(const TaskSchedule&) const;
    std::int64_t work(const TaskSchedule&) const;
    std::int64_t duration(const TaskSchedule&) const;
};
// Completed actual rows have no future demand. Work-driven reports require an
// explicit remaining work report; actual effort is never duration * units.
Task pendingTask(const Project&,const Task&);
std::vector<WorkSpan> executionSpans(const PlanInput&,const Task&,const TaskSchedule&);
ScheduleResult calculateEffort(const Project&,bool forecast=false,const std::string& levelingPriority={},int horizonDays=260);
LevelingResult levelEffort(const Project&,const std::string&,int);
void changeDuration(Project&,std::size_t,std::optional<std::int64_t>);
void changeWork(Project&,std::size_t,std::int64_t);
}
