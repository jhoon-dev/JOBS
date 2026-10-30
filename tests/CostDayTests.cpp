#include "application/Document.h"
#include "calculation/Costs.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
const jobs::CostDay &day(const jobs::CostAnalysis &costs, const std::string &date) {
    const auto it = std::find_if(costs.days.begin(), costs.days.end(), [&](const auto &d) { return d.date == date; });
    if (it == costs.days.end())
        throw std::runtime_error("expected cost date missing");
    return *it;
}
jobs::CostEntry report(const std::string &id, int actual, int physical) {
    jobs::CostEntry entry;
    entry.taskId = id;
    entry.actualMinor = actual;
    entry.physicalBasisPoints = physical;
    entry.reporter = "PM";
    entry.reason = "Boundary observation";
    entry.recordedAt = "2026-10-30T00:00:00Z";
    return entry;
}
void lastMinute(jobs::JsonProjectStore &store, const std::string &date, const std::string &next) {
    jobs::Document doc(store);
    doc.create("Last minute of day", date, {"23:59 work"});
    doc.setDuration(0, 1);
    const auto id = doc.project()->tasks[0].id;
    const auto calendar = doc.putCalendar({"night", "Night", {1, 2, 3, 4, 5, 6, 7}, {{1439, 1440}}, {}});
    doc.setTaskCalendar(0, calendar);
    const auto resource = doc.putResource({{}, "R", "labor", 100, {}, {{date, 6000}}});
    doc.setAssignments(id, {{id, resource, 100}});
    doc.setTaskCost({id, 100, "uniform", "physical"});
    const auto baseline = doc.captureBaseline("B", "PM", "Approved", "2026-10-30T00:00:00Z");
    auto costs = jobs::analyzeCosts(*doc.project(), baseline);
    check(costs.evm.bac == 200 && day(costs, date).budget == 200 && day(costs, date).pv == 200 &&
              day(costs, next).budget == 0 && day(costs, next).pv == 200 && !day(costs, date).ac,
          "last minute fixed/resource cost missing from day-end PV without status");
    doc.setStatusAt(date + " 23:59");
    doc.recordCost(report(id, 7, 0));
    costs = jobs::analyzeCosts(*doc.project(), baseline);
    check(day(costs, date).budget == 200 && day(costs, date).pv == 0 && costs.evm.pv == 0 && !day(costs, next).ac,
          "status-day PV included work after the precise status minute");
    doc.setStatusAt(next + " 00:00");
    doc.recordCost(report(id, 200, 10000));
    costs = jobs::analyzeCosts(*doc.project(), baseline);
    check(day(costs, date).pv == 200 && day(costs, date).ev == 0 && day(costs, date).ac == 7 &&
              day(costs, date).currentActual == 7 && day(costs, next).pv == 200 && day(costs, next).ev == 200 &&
              day(costs, next).ac == 200 && costs.evm.pv == 200,
          "midnight report leaked into previous day or continuous last-minute PV was lost");
}
void midnightEvent(jobs::JsonProjectStore &store, const std::string &accrual, bool milestone) {
    jobs::Document doc(store);
    doc.create("Midnight event", "2026-10-26", {"Anchor", "Midnight"});
    doc.setDuration(0, 0);
    doc.setDuration(1, milestone ? 0 : 1);
    const auto id = doc.project()->tasks[1].id;
    const auto calendar = doc.putCalendar({"all", "All day", {1, 2, 3, 4, 5, 6, 7}, {{0, 1440}}, {}});
    doc.setTaskCalendar(1, calendar);
    doc.setTaskTiming(1, jobs::ConstraintType::SNET,
                      accrual == "start" || milestone ? "2026-10-27 00:00" : "2026-10-26 23:59", {});
    doc.setTaskCost({doc.project()->tasks[0].id, 0, "uniform", "physical"});
    doc.setTaskCost({id, 100, accrual, "physical"});
    const auto baseline = doc.captureBaseline("B", "PM", "Approved", "2026-10-30T00:00:00Z");
    auto costs = jobs::analyzeCosts(*doc.project(), baseline);
    check(day(costs, "2026-10-26").budget == 0 && day(costs, "2026-10-26").pv == 0 &&
              day(costs, "2026-10-27").budget == 100 && day(costs, "2026-10-27").pv == 100,
          "instantaneous midnight fixed cost accrued on previous day");
    doc.setStatusAt("2026-10-27 00:00");
    costs = jobs::analyzeCosts(*doc.project(), baseline);
    check(day(costs, "2026-10-26").pv == 0 && day(costs, "2026-10-27").pv == 100 && costs.evm.pv == 100,
          "precise midnight status did not include the event at that time");
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        jobs::JsonProjectStore store;
        for (const auto &[date, next] : {std::pair{"2026-10-26", "2026-10-27"},
                                         {"2026-01-31", "2026-02-01"},
                                         {"2026-02-28", "2026-03-01"},
                                         {"2028-02-28", "2028-02-29"},
                                         {"2028-02-29", "2028-03-01"},
                                         {"2026-12-31", "2027-01-01"}})
            lastMinute(store, date, next);
        midnightEvent(store, "start", false);
        midnightEvent(store, "finish", false);
        midnightEvent(store, "uniform", true);
        for (int profile : {0, 1, 2})
            for (bool late : {false, true}) {
                if (profile == 0 && late)
                    continue;
                jobs::Document last(store);
                last.create("Last supported date", "9999-12-31", {"A"});
                last.setDuration(0, late ? 1 : 60);
                const auto id = last.project()->tasks[0].id;
                std::string calendar;
                if (profile) {
                    calendar = last.putCalendar({"all", "All day", {1, 2, 3, 4, 5, 6, 7}, {{0, 1440}}, {}});
                    last.setTaskCalendar(0, calendar);
                    last.setTaskTiming(0, jobs::ConstraintType::SNET, late ? "9999-12-31 23:58" : "9999-12-31 09:00",
                                       {});
                }
                jobs::Resource resource{{}, "R", "labor", 100, {}, {{"9999-12-31", 6000}}};
                resource.calendarId = profile == 2 ? calendar : "";
                const auto rid = last.putResource(resource);
                last.setAssignments(id, {{id, rid, 100}});
                if (profile == 2)
                    last.setTaskEffort(0, jobs::TaskType::FixedWork, true, late ? 1 : 60, late ? 100 : 6000, {});
                last.setTaskCost({id, 100, "uniform", "physical"});
                const auto baseline = last.captureBaseline("B", "PM", "Approved", "2026-10-30T00:00:00Z");
                const auto total = late ? 200 : 6100;
                auto costs = jobs::analyzeCosts(*last.project(), baseline);
                check(costs.evm.bac == total && day(costs, "9999-12-31").plan == total &&
                          day(costs, "9999-12-31").budget == total && day(costs, "9999-12-31").pv == total,
                      "calendar/effort/resource day-end cost exceeded supported date range");
                last.setStatusAt(late ? "9999-12-31 23:58" : "9999-12-31 09:30");
                costs = jobs::analyzeCosts(*last.project(), baseline);
                check(costs.evm.pv == (late ? 0 : 3050) && day(costs, "9999-12-31").pv == costs.evm.pv,
                      "final-date partial status PV included future work");
                last.setStatusAt("9999-12-31 23:59");
                check(jobs::analyzeCosts(*last.project(), baseline).evm.pv == total,
                      "last supported minute lost cumulative work or resource cost");
            }
        std::cout << "PASS: last-minute uniform/resource cost, exclusive midnight events/reports, exact status, "
                     "month/year/leap-day and final supported date boundaries\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
