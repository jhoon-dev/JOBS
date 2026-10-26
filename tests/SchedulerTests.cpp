#include "calculation/Scheduler.h"
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
bool diagnostic(const jobs::ScheduleResult& r, const std::string& code) {
    return std::any_of(r.diagnostics.begin(), r.diagnostics.end(), [&](const auto& d) { return d.code == code; });
}
jobs::Project project() {
    jobs::Project p; p.documentId = "fixture"; p.name = "fixture"; p.startDate = "2026-10-26"; return p;
}
}
int main() {
    try {
        auto p = project();
        p.tasks = {{"A", "A", 16 * 60, {}}, {"B", "B", 24 * 60, {"A"}},
                   {"C", "C", 8 * 60, {"A"}}, {"D", "D", 16 * 60, {"B", "C"}}};
        auto r = jobs::calculate(p);
        check(r.status == jobs::CalculationStatus::Valid && r.projectFinish == "2026-11-03 18:00", "F01 project finish mismatch");
        check(r.tasks[0].earlyStartMinutes == 0 && r.tasks[0].earlyFinishMinutes == 960, "F01 A mismatch");
        check(r.tasks[1].earlyStartMinutes == 960 && r.tasks[1].earlyFinishMinutes == 2400, "F01 B mismatch");
        check(r.tasks[2].lateStartMinutes == 1920 && r.tasks[2].totalFloatMinutes == 960 && r.tasks[2].freeFloatMinutes == 960,
              "F01 C late dates / total / free float mismatch");
        check(r.tasks[3].earlyFinishMinutes == 3360 && r.tasks[3].drivingPredecessors == std::vector<std::string>{"B"}, "F01 D/driving mismatch");
        check(r.tasks[0].critical && r.tasks[1].critical && !r.tasks[2].critical && r.tasks[3].critical, "critical path mismatch");
        check(r.inputRevision == p.revision && r.documentId == p.documentId && r.profile == p.calculationProfile, "input provenance mismatch");
        check(jobs::calculate(p).tasks == r.tasks, "not deterministic");
        p.tasks[2].durationMinutes = 24 * 60;
        r = jobs::calculate(p);
        check(r.tasks[1].critical && r.tasks[2].critical && r.tasks[3].drivingPredecessors.size() == 2, "multiple critical paths mismatch");
        // Dates and break boundaries use the native 09-12/13-18 profile, not the draft's continuous 09-17 fixture.
        p = project(); p.startDate = "2026-10-30";
        p.tasks = {{"A", "Friday", 480, {}}, {"M", "milestone", 0, {"A"}}, {"B", "next", 480, {"M"}}};
        r = jobs::calculate(p);
        check(r.tasks[0].finish == "2026-10-30 18:00" && r.tasks[1].start == "2026-10-30 18:00" &&
              r.tasks[1].finish == r.tasks[1].start && r.tasks[2].start == "2026-11-02 09:00", "weekend/milestone endpoint mismatch");
        p.holidays = {"2026-11-02"}; r = jobs::calculate(p);
        check(r.projectFinish == "2026-11-03 18:00" && r.tasks[2].start == "2026-11-03 09:00", "holiday mismatch");
        p.tasks = {{"A", "morning", 180, {}}, {"B", "afternoon", 60, {"A"}}};
        r = jobs::calculate(p);
        check(r.tasks[0].finish == "2026-10-30 12:00" && r.tasks[1].start == "2026-10-30 13:00" &&
              r.tasks[1].finish == "2026-10-30 14:00", "lunch boundary mismatch");
        p.startDate = "2026-10-31"; r = jobs::calculate(p);
        check(r.tasks[0].start == "2026-11-03 09:00", "weekend/holiday anchor mismatch");
        p.tasks[0].durationMinutes.reset(); r = jobs::calculate(p);
        check(r.status == jobs::CalculationStatus::Incomplete && !r.projectFinish && r.tasks.empty() && diagnostic(r, "MISSING_DURATION"), "missing duration produced a forecast");
        p = project(); p.tasks = {{"A", "A", 60, {"B"}}, {"B", "B", 60, {"A"}}, {"C", "downstream", 60, {"B"}}};
        r = jobs::calculate(p);
        check(r.status == jobs::CalculationStatus::Conflict && !r.projectFinish && r.tasks.empty() && diagnostic(r, "DEPENDENCY_CYCLE"), "cycle accepted");
        for (const auto& d : r.diagnostics) if (d.code == "DEPENDENCY_CYCLE") {
            check(d.taskIds.size() == 3 && d.taskIds.front() == d.taskIds.back() &&
                  std::find(d.taskIds.begin(), d.taskIds.end(), "C") == d.taskIds.end(), "cycle path includes non-cycle downstream task");
        }
        p.tasks[0].predecessors = {"unknown"}; p.tasks[1].predecessors.clear(); r = jobs::calculate(p);
        check(diagnostic(r, "INVALID_DEPENDENCY") && !r.projectFinish, "dangling reference accepted");
        p.tasks[0].predecessors = {"A"}; r = jobs::calculate(p);
        check(diagnostic(r, "INVALID_DEPENDENCY"), "self-reference accepted");
        p.tasks = {{"A", "large", 5000000, {}}}; r = jobs::calculate(p);
        check(diagnostic(r, "NO_AVAILABILITY_IN_HORIZON") && !r.projectFinish && r.tasks.empty(), "horizon silently truncated");
        p.startDate = "2026-02-30"; r = jobs::calculate(p);
        check(diagnostic(r, "INVALID_INPUT"), "invalid date accepted");
        p = project(); p.tasks = {{"A", "milestone", 0, {}}}; r = jobs::calculate(p);
        check(r.status == jobs::CalculationStatus::Valid && r.projectFinish == "2026-10-26 09:00", "zero milestone mismatch");
        p.tasks.clear(); check(!jobs::calculate(p).projectFinish, "empty plan got finish");
        std::cout << "PASS: F01 CPM/float, multiple critical paths, calendar/holidays/lunch, milestones, missing duration, cycle path, dangling/self refs, horizon, provenance, determinism\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
