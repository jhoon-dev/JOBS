#pragma once
#include "calculation/Pdm.h"
namespace jobs {
struct ConstraintWindow { std::int64_t lower=0; std::optional<std::int64_t> upper; };
ConstraintWindow constraintWindow(const PlanInput&,const Task&,const WorkCalendar&,std::int64_t duration);
std::optional<Diagnostic> timingViolation(const Task&,const TaskSchedule&);
std::vector<Diagnostic> deadlineDiagnostics(const PlanInput&,const std::vector<TaskSchedule>&);
std::optional<std::int64_t> deadlineSlack(const PlanInput&,const Task&,const std::string& finish);
ScheduleResult calculateConstrainedPlan(const Project&,const std::vector<std::size_t>& order);
}
