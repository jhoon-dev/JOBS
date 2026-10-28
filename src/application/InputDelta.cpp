#include "application/InputDelta.h"

namespace jobs {
namespace {
std::size_t optionalBytes(const std::optional<std::string> &value) {
    return value ? value->size() : 0;
}
std::size_t valueBytes(const ValueDelta<std::string> &value) {
    return value.values ? value.values->first.size() + value.values->second.size() : 0;
}
} // namespace
std::size_t inputExtraBytes(const Task &t) {
    std::size_t bytes = t.id.size() + t.name.size() + t.wbsId.size() + t.calendarId.size() +
                        optionalBytes(t.constraintAt) + optionalBytes(t.deadlineAt);
    for (const auto &id : t.predecessors)
        bytes += sizeof(std::string) + id.size();
    for (const auto &edge : t.dependencyRules)
        bytes += sizeof(Dependency) + edge.predecessorId.size();
    for (const auto &pause : t.breaks)
        bytes += sizeof(TaskBreak) + pause.start.size() + pause.finish.size();
    return bytes;
}
std::size_t inputExtraBytes(const WbsNode &w) {
    return w.id.size() + w.name.size() + w.parentId.size();
}
std::size_t inputExtraBytes(const Resource &r) {
    std::size_t bytes = r.id.size() + r.name.size() + r.type.size() + r.calendarId.size();
    for (const auto &day : r.exceptions)
        bytes += sizeof(CapacityDay) + day.date.size();
    for (const auto &rate : r.rates)
        bytes += sizeof(Resource::Rate) + rate.effectiveDate.size();
    return bytes;
}
std::size_t inputExtraBytes(const Assignment &a) {
    return a.taskId.size() + a.resourceId.size();
}
std::size_t inputExtraBytes(const TaskCost &c) {
    return c.taskId.size() + c.accrual.size() + c.credit.size();
}
std::size_t inputExtraBytes(const TaskCalendar &c) {
    std::size_t bytes =
        c.id.size() + c.name.size() + c.weekdays.size() * sizeof(int) + c.periods.size() * sizeof(WorkPeriod);
    for (const auto &day : c.holidays)
        bytes += sizeof(std::string) + day.size();
    return bytes;
}
InputDelta::InputDelta(const Project &before, const Project &after)
    : name_(before.name, after.name), startDate_(before.startDate, after.startDate),
      calendar_(before.calendar, after.calendar),
      calculationProfile_(before.calculationProfile, after.calculationProfile),
      currency_(before.currency, after.currency), costProfile_(before.costProfile, after.costProfile),
      statusAt_(before.statusAt, after.statusAt), tasks_(before.tasks, after.tasks),
      holidays_(before.holidays, after.holidays), wbsNodes_(before.wbsNodes, after.wbsNodes),
      resources_(before.resources, after.resources), assignments_(before.assignments, after.assignments),
      taskCosts_(before.taskCosts, after.taskCosts), calendars_(before.calendars, after.calendars) {}

void InputDelta::apply(Project &p, bool forward) const {
    name_.apply(p.name, forward);
    startDate_.apply(p.startDate, forward);
    calendar_.apply(p.calendar, forward);
    calculationProfile_.apply(p.calculationProfile, forward);
    currency_.apply(p.currency, forward);
    costProfile_.apply(p.costProfile, forward);
    statusAt_.apply(p.statusAt, forward);
    tasks_.apply(p.tasks, forward);
    holidays_.apply(p.holidays, forward);
    wbsNodes_.apply(p.wbsNodes, forward);
    resources_.apply(p.resources, forward);
    assignments_.apply(p.assignments, forward);
    taskCosts_.apply(p.taskCosts, forward);
    calendars_.apply(p.calendars, forward);
}
std::size_t InputDelta::bytes() const {
    std::size_t bytes = sizeof(InputDelta) + valueBytes(name_) + valueBytes(startDate_) + valueBytes(calendar_) +
                        valueBytes(calculationProfile_) + valueBytes(currency_) + valueBytes(costProfile_);
    if (statusAt_.values)
        bytes += optionalBytes(statusAt_.values->first) + optionalBytes(statusAt_.values->second);
    return bytes + tasks_.bytes() + holidays_.bytes() + wbsNodes_.bytes() + resources_.bytes() + assignments_.bytes() +
           taskCosts_.bytes() + calendars_.bytes();
}
} // namespace jobs
