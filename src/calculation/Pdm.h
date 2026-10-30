#pragma once
#include "calculation/Scheduler.h"
#include "calculation/WorkCalendar.h"
namespace jobs {
// Calendar-local start offset satisfying one lower-bound endpoint relationship.
std::int64_t pdmRequiredStart(const WorkCalendar &successor, const Dependency &, const TaskSchedule &predecessor,
                              std::int64_t duration);
void pdmDates(const WorkCalendar &clock, const WorkCalendar &, TaskSchedule &, std::int64_t localStart,
              std::int64_t duration);
ScheduleResult calculatePdmPlan(const Project &, const std::vector<std::size_t> &order);
ScheduleResult calculatePdmForecast(const Project &, ScheduleResult plan);
std::int64_t pdmLatestStart(const WorkCalendar &, const std::string &finish, std::int64_t duration);
std::int64_t pdmFloat(const WorkCalendar &, std::int64_t from, std::int64_t to, std::int64_t duration);
} // namespace jobs
