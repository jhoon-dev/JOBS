#include "calculation/SharedResources.h"
#include "formatting/DiagnosticText.h"
#include <algorithm>
#include <iostream>

namespace {
using namespace jobs;
void check(bool value, const char *reason) {
    if (!value)
        throw std::logic_error(reason);
}
Project sample(std::string id, std::string start = "2026-10-26", std::string at = "2026-10-26 09:00") {
    Project p;
    p.documentId = id;
    p.name = id;
    p.startDate = start;
    p.calculationProfile = constraintProfile;
    p.tasks = {{"T", "Task", 60, {}}};
    p.tasks[0].constraintType = ConstraintType::SNET;
    p.tasks[0].constraintAt = at;
    p.resources = {{"R", "Same display name", "labor", 100, {}}};
    p.assignments = {{"T", "R", 100}};
    return p;
}
Workspace config(const Project &a, const Project &b) {
    Workspace w;
    w.id = "W";
    w.projects = {{a.documentId, "", a.name}, {b.documentId, "", b.name}};
    w.resources = {{"pool", "Shared"}};
    w.bindings = {{a.documentId, "R", "pool"}, {b.documentId, "R", "pool"}};
    return w;
}
SharedResourceResult run(const Workspace &w, const Project &a, const Project &b, bool forecast = false) {
    return calculateSharedResources(w, {std::make_shared<const Project>(a), std::make_shared<const Project>(b)},
                                    forecast);
}
std::int64_t demand(const SharedResourceResult &value) {
    std::int64_t n = 0;
    for (const auto &row : value.days)
        n += row.demandUnitMinutes;
    return n;
}
} // namespace
int main() {
    try {
        auto a = sample("A"), b = sample("B", "2026-10-26", "2026-10-26 10:00");
        auto w = config(a, b);
        auto result = run(w, a, b);
        check(result.status == CalculationStatus::Valid && result.days.size() == 1 && result.days[0].peakUnits == 100 &&
                  result.days[0].overloadMinutes == 0 && demand(result) == 12000,
              "adjacent intervals were summed as concurrent");
        check(result.days[0].tasks.size() == 2 && result.days[0].tasks[0].taskId == result.days[0].tasks[1].taskId &&
                  result.days[0].tasks[0].projectId != result.days[0].tasks[1].projectId,
              "task IDs lost project scope");
        b.tasks[0].constraintAt = "2026-10-26 09:30";
        result = run(w, a, b);
        check(result.status == CalculationStatus::Conflict && result.days[0].peakUnits == 200 &&
                  result.days[0].overloadMinutes == 30 && result.days[0].excessUnitMinutes == 3000,
              "30-minute overlap mismatch");
        w.resources[0].maxUnits = 200;
        check(run(w, a, b).status == CalculationStatus::Valid, "shared capacity ignored");
        w.resources[0].maxUnits.reset();
        result = run(w, a, b);
        check(result.status == CalculationStatus::Incomplete && result.days[0].unknownCapacityMinutes == 90 &&
                  result.days[0].overloadMinutes == 0,
              "unknown capacity treated as zero");
        w.resources[0].maxUnits = 0;
        result = run(w, a, b);
        check(result.days[0].overloadMinutes == 90 && result.days[0].unknownCapacityMinutes == 0,
              "zero capacity treated as unknown");
        w.resources[0].exceptions = {{"2026-10-26", 200}};
        check(run(w, a, b).status == CalculationStatus::Valid, "daily capacity override ignored");
        w.resources[0].calendar.holidays = {"2026-10-26"};
        check(run(w, a, b).days[0].overloadMinutes == 90, "shared holiday did not set capacity zero");
        w = config(a, b);
        b.startDate = "2026-10-27";
        b.tasks[0].constraintAt = "2026-10-27 09:00";
        result = run(w, a, b);
        check(result.status == CalculationStatus::Valid && result.days.size() == 2 &&
                  result.days[1].date == "2026-10-27",
              "different origins mixed project-relative minutes/cache");
        // Split execution across lunch and an explicit break. Calendar intersection preserves 60m work.
        a = sample("A", "2026-10-26", "2026-10-26 11:30");
        a.calculationProfile = effortProfile;
        a.tasks[0].taskType = TaskType::FixedWork;
        a.tasks[0].workDriven = true;
        a.tasks[0].workUnitMinutes = 6000;
        TaskCalendar calendar;
        calendar.id = "C";
        calendar.name = "Resource calendar";
        calendar.periods = {{690, 720}, {780, 840}};
        a.calendars = {calendar};
        a.resources[0].calendarId = "C";
        a.tasks[0].breaks = {{"2026-10-26 13:00", "2026-10-26 13:15"}};
        b = sample("B", "2026-10-26", "2026-10-26 13:30");
        b.tasks[0].durationMinutes = 15;
        w = config(a, b);
        result = run(w, a, b);
        check(result.days[0].overloadMinutes == 15 && demand(result) == 7500,
              "resource calendar, lunch or split work lost");
        const auto original = a;
        a.statusAt = "2026-10-26 14:00";
        ProgressEntry progress;
        progress.id = "P";
        progress.taskId = "T";
        progress.statusAt = *a.statusAt;
        progress.recordedAt = "2026-10-26T05:00:00Z";
        progress.reporter = "PM";
        progress.reason = "Remaining";
        progress.sourceDocumentId = "A";
        progress.recordedRevision = 1;
        progress.planDurationMinutes = 60;
        progress.actualStart = "2026-10-26 11:30";
        progress.actualDurationMinutes = 30;
        progress.remainingMinutes = 30;
        progress.remainingWorkUnitMinutes = 3000;
        a.progress = {progress};
        b = sample("B", "2026-10-27", "2026-10-27 11:30");
        b.tasks[0].durationMinutes = 30;
        w = config(a, b);
        result = run(w, a, b, true);
        check(result.status == CalculationStatus::Conflict && result.days.size() == 1 &&
                  result.days[0].date == "2026-10-27" && demand(result) == 6000,
              "forecast used historical demand instead of remaining effort");
        a = original;
        w = config(a, b);
        const auto frozen = a;
        const auto frozenW = w;
        (void)run(w, a, b);
        check(a == frozen && w == frozenW, "aggregate mutated source");
        w.bindings.pop_back();
        result = run(w, a, b);
        check(result.unmappedResources == 1 && result.days[0].tasks.size() == 1,
              "unmapped resource silently merged by name");
        result = calculateSharedResources(w, {std::make_shared<const Project>(a)}, false);
        check(result.status == CalculationStatus::Incomplete && result.includedProjects == 1,
              "closed project concealed");
        w.bindings[0].resourceId = "missing";
        check(run(w, a, b).status == CalculationStatus::Incomplete, "deleted local resource concealed");
        auto control = std::make_shared<CalculationControl>();
        control->cancel();
        bool cancelled = false;
        try {
            (void)calculateSharedResources(w, {std::make_shared<const Project>(a)}, false, control);
        } catch (const CalculationCancelled &) {
            cancelled = true;
        }
        check(cancelled, "cancellation converted to conflict");
        // Same source-local IDs before the Unix epoch exercise civil floor division.
        a = sample("A", "1969-12-31", "1969-12-31 09:00");
        b = sample("B", "1969-12-31", "1969-12-31 09:00");
        result = run(config(a, b), a, b);
        check(result.days[0].date == "1969-12-31" && result.days[0].overloadMinutes == 60, "pre-epoch day rounding");
        for (int seed = 0; seed < 24; ++seed) {
            const int startA = 540 + (seed * 17) % 120, startB = 540 + (seed * 19) % 120;
            const auto stamp = [](int minute) {
                return std::string("2026-10-26 ") + (minute / 60 < 10 ? "0" : "") + std::to_string(minute / 60) + ":" +
                       (minute % 60 < 10 ? "0" : "") + std::to_string(minute % 60);
            };
            a = sample("A", "2026-10-26", stamp(startA));
            b = sample("B", "2026-10-26", stamp(startB));
            a.tasks[0].durationMinutes = 15 + (seed * 13) % 45;
            b.tasks[0].durationMinutes = 15 + (seed * 23) % 45;
            a.assignments[0].units = 25 * (1 + seed % 4);
            b.assignments[0].units = 50 * (1 + seed % 3);
            w = config(a, b);
            w.resources[0].maxUnits = 50 * (1 + seed % 4);
            result = run(w, a, b);
            std::int64_t peak = 0, total = 0, over = 0, excess = 0;
            for (int minute = 540; minute < 720; ++minute) {
                const auto used =
                    (minute >= startA && minute < startA + *a.tasks[0].durationMinutes ? a.assignments[0].units : 0) +
                    (minute >= startB && minute < startB + *b.tasks[0].durationMinutes ? b.assignments[0].units : 0);
                peak = std::max(peak, std::int64_t(used));
                total += used;
                if (used > *w.resources[0].maxUnits) {
                    ++over;
                    excess += used - *w.resources[0].maxUnits;
                }
            }
            check(result.days.size() == 1 && result.days[0].peakUnits == peak &&
                      result.days[0].demandUnitMinutes == total && result.days[0].overloadMinutes == over &&
                      result.days[0].excessUnitMinutes == excess,
                  "independent minute oracle disagrees");
        }
        std::cout << "PASS: absolute half-open overlap, capacity, scoped IDs, calendars, splits, remaining effort, "
                     "missing scope and cancellation\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << errorText(error) << '\n';
        return 1;
    }
}
