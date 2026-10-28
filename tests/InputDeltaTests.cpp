#include "application/InputDelta.h"
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void roundtrip(const jobs::Project &before, const jobs::Project &after) {
    jobs::InputDelta delta(before, after);
    auto actual = before;
    delta.apply(actual, true);
    check(actual == after, "forward delta lost an input field or order");
    delta.apply(actual, false);
    check(actual == before, "inverse delta lost an input field or order");
}
} // namespace
int main() {
    try {
        jobs::Project before;
        before.documentId = "document";
        before.name = "before";
        before.startDate = "2026-10-26";
        for (int i = 0; i < 10000; ++i)
            before.tasks.push_back({"T" + std::to_string(i), "Task", 60, {}});
        auto after = before;
        after.tasks[1].durationMinutes = 240;
        after.tasks[9998].name = "Distant edit";
        jobs::InputDelta sparse(before, after);
        check(sparse.bytes() < 10000, "distant edits retained intervening tasks");
        roundtrip(before, after);
        after = before;
        after.tasks.insert(after.tasks.begin() + 50, {{"new1", "First", 0, {}}, {"new2", "Second", 120, {}}});
        roundtrip(before, after);
        roundtrip(after, before);
        auto empty = before;
        empty.tasks.clear();
        roundtrip(empty, before);
        roundtrip(before, empty);
        roundtrip(before, before);
        after = before;
        after.name = "after";
        after.startDate = "2026-11-02";
        after.calendar = "other-calendar";
        after.calculationProfile = jobs::effortProfile;
        after.currency = "USD";
        after.costProfile = "other-cost-profile";
        after.statusAt = "2026-11-02 18:00";
        after.holidays = {"2026-12-25"};
        after.wbsNodes = {{"W", "WBS", ""}};
        after.resources = {{"R", "Resource", "labor", 200, {{"2026-11-02", 50}}, {{"2026-10-26", 1000}}, "C"}};
        after.assignments = {{"T1", "R", 50}};
        after.taskCosts = {{"T1", 100, "end", "duration"}};
        after.calendars = {{"C", "Calendar", {1, 3, 5}, {{540, 720}}, {"2026-12-25"}}};
        auto &task = after.tasks[1];
        task.name = "updated";
        task.predecessors = {"T0"};
        task.wbsId = "W";
        task.dependencyRules = {{"T0", "SS", -30}};
        task.calendarId = "C";
        task.constraintType = "SNET";
        task.constraintAt = "2026-11-02 09:00";
        task.deadlineAt = "2026-11-03 18:00";
        task.taskType = "fixed-work";
        task.workUnitMinutes = 6000;
        task.workDriven = true;
        task.breaks = {{"2026-11-02 10:00", "2026-11-02 11:00"}};
        roundtrip(before, after);
        roundtrip(after, before);
        // Existing rows modified alongside a size change must not be skipped as a common tail.
        after.tasks.erase(after.tasks.begin() + 60);
        after.tasks[999].durationMinutes.reset();
        roundtrip(before, after);
        std::cout << "PASS: sparse, inserted/deleted/empty vectors, all scalar/input fields and inverse order\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
