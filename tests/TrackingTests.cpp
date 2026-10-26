#include "application/Document.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool c, const char* m) { if (!c) throw std::runtime_error(m); }
void rejects(const std::function<void()>& f) { bool rejected = false; try { f(); } catch (const std::exception&) { rejected = true; } check(rejected, "expected tracking rejection"); }
class Store final : public jobs::ProjectStore {
    mutable int id_ = 0;
public:
    jobs::Project stored;
    std::string newId() const override { return "tracking-" + std::to_string(++id_); }
    std::string normalizePath(const std::string& s) const override { return s; }
    std::unique_ptr<jobs::WriteLease> lock(const std::string&) override { return nullptr; }
    jobs::StoredProject load(const std::string&) const override { return {stored, "test", false}; }
    std::string save(const std::string&, const jobs::Project&, const std::string&) override { return "test"; }
};
jobs::ProgressEntry entry(const std::string& id, std::int64_t actual, std::int64_t remaining) {
    jobs::ProgressEntry e; e.taskId = id; e.actualStart = "2026-10-26 09:00"; e.actualDurationMinutes = actual; e.remainingMinutes = remaining;
    e.recordedAt = "2026-10-26T09:00:00Z"; e.reporter = "scheduler"; e.reason = "상태 보고"; return e;
}
bool diagnostic(const jobs::ScheduleResult& r, const std::string& code) {
    return std::any_of(r.diagnostics.begin(), r.diagnostics.end(), [&](const auto& d) { return d.code == code; });
}
}
int main() {
    try {
        Store store; jobs::Document doc(store); doc.create("F09", "2026-10-26", {"A"}); doc.setDuration(0, 960);
        const auto id = doc.project()->tasks[0].id;
        auto snapshot = *doc.project(); rejects([&] { doc.captureBaseline("", "", "", "bad"); }); check(*doc.project() == snapshot, "failed baseline modified document");
        const auto b1 = doc.captureBaseline("최초 승인", "PM", "초기 계획 승인", "2026-10-26T00:00:00Z");
        const auto frozen = doc.project()->baselines[0];
        check(frozen.version == 1 && frozen.projectFinish == "2026-10-27 18:00" && frozen.input.tasks[0].durationMinutes == 960, "baseline snapshot mismatch");
        doc.setStatusAt("2026-10-26 18:00"); doc.recordProgress(entry(id, 480, 960));
        auto forecast = jobs::calculate(*doc.project());
        check(forecast.status == jobs::CalculationStatus::Valid && forecast.isForecast && forecast.projectFinish == "2026-10-28 18:00", "F09 forecast24h mismatch");
        check(forecast.tasks[0].start == "2026-10-26 09:00" && forecast.tasks[0].remainingStart == "2026-10-27 09:00", "F09 actual/remaining boundary mismatch");
        auto comparison = jobs::compareToBaseline(*doc.project(), b1);
        check(comparison.planFinishVariance == 0 && comparison.forecastFinishVariance == 480 && comparison.tasks[0].forecastFinishVariance == 480,
              "F09 separate plan0h / forecast+8h variance mismatch");
        check(comparison.reportedTasks == 1 && comparison.tasks[0].durationProgressPercent && std::abs(*comparison.tasks[0].durationProgressPercent - 100.0 / 3) < 1e-9,
              "duration progress formula/coverage mismatch");
        doc.setDuration(0, 1200); comparison = jobs::compareToBaseline(*doc.project(), b1);
        check(comparison.planFinishVariance == 240 && comparison.forecastFinishVariance == 480 &&
              comparison.tasks[0].changes == std::vector<std::string>{"계획 기간 변경"}, "plan edit not separated from reported remaining");
        const auto b2 = doc.captureBaseline("변경 승인", "PM2", "기간 변경 승인", "2026-10-26T01:00:00Z");
        check(doc.project()->baselines[0] == frozen && doc.project()->baselines[1].version == 2 && doc.project()->baselines[1].projectFinish == "2026-10-28 14:00", "new baseline replaced original / froze forecast");
        check(jobs::compareToBaseline(*doc.project(), b2).planFinishVariance == 0, "baseline selection mismatch");
        doc.setHolidays({"2026-10-27"});
        comparison = jobs::compareToBaseline(*doc.project(), b1);
        check(comparison.forecastFinishVariance == 960 && !comparison.contextChanges.empty() && doc.project()->baselines[0] == frozen,
              "calendar edit changed baseline / variance calendar");
        auto tampered = frozen; tampered.schedule[0].finish = "2026-10-28 18:00"; rejects([&] { jobs::verifyBaseline(tampered); });
        auto prepared = jobs::prepareBaseline(*doc.project(), "pending", "PM", "reason", "2026-10-26T01:00:00Z");
        doc.renameTask(0, "renamed"); rejects([&] { doc.appendBaseline(prepared); });
        check(doc.project()->baselines.size() == 2, "stale prepared baseline appended");
        // New correction events preserve old reports; as-of selection uses report dates, not append time.
        jobs::Document history(store); history.create("history", "2026-10-26", {"A"}); history.setDuration(0, 960);
        auto hid = history.project()->tasks[0].id; history.setStatusAt("2026-10-26 18:00"); history.recordProgress(entry(hid, 480, 960));
        const auto old = history.project()->progress[0];
        history.setStatusAt("2026-10-27 18:00"); history.recordProgress(entry(hid, 960, 480));
        const auto later = history.project()->progress[1];
        history.setStatusAt("2026-10-26 18:00"); check(jobs::progressAsOf(*history.project(), hid)->remainingMinutes == 960, "past as-of used future report");
        auto correction = entry(hid, 480, 1200); correction.reason = "정정: 잔여 재산정"; history.recordProgress(correction);
        check(history.project()->progress[0] == old && history.project()->progress[1] == later && history.project()->progress[2].previousEntryId == later.id &&
              jobs::progressAsOf(*history.project(), hid)->remainingMinutes == 1200, "correction overwrote history or tie selection");
        history.setStatusAt("2026-10-27 18:00"); check(jobs::progressAsOf(*history.project(), hid)->id == later.id, "backdated correction superseded later status report");
        history.setStatusAt(std::nullopt); check(history.project()->progress.size() == 3 && !jobs::calculate(*history.project()).isForecast, "plan mode erased reports");
        history.setStatusAt("2026-10-28 18:00"); auto done = entry(hid, 720, 0); done.actualFinish = "2026-10-27 14:00";
        history.recordProgress(done); auto complete = jobs::calculate(*history.project());
        check(complete.tasks[0].completedActual && complete.projectFinish == done.actualFinish && complete.tasks[0].start == done.actualStart,
              "completed actual dates moved to status date");
        auto valid = *history.project();
        auto bad = done; bad.actualFinish = "2026-10-29 09:00"; rejects([&] { history.recordProgress(bad); });
        bad = done; bad.actualFinish = "2026-10-25 09:00"; rejects([&] { history.recordProgress(bad); });
        bad = done; bad.remainingMinutes = 60; rejects([&] { history.recordProgress(bad); });
        bad = done; bad.actualFinish.reset(); rejects([&] { history.recordProgress(bad); });
        bad = done; bad.actualStart.reset(); rejects([&] { history.recordProgress(bad); });
        bad = done; bad.reporter.clear(); rejects([&] { history.recordProgress(bad); });
        bad = done; bad.reason.clear(); rejects([&] { history.recordProgress(bad); });
        bad = done; bad.remainingMinutes = -1; rejects([&] { history.recordProgress(bad); });
        check(*history.project() == valid, "invalid progress changed ledger/revision");
        // F08 retained logic in the native split-day calendar: data=12h, A finish20h, B remaining20..24h.
        jobs::Document outOfSequence(store); outOfSequence.create("F08", "2026-10-26", {"A", "B"});
        outOfSequence.setDuration(0, 960); outOfSequence.setDuration(1, 480);
        const auto aid = outOfSequence.project()->tasks[0].id, bid = outOfSequence.project()->tasks[1].id;
        outOfSequence.setPredecessors(1, {aid}); outOfSequence.setStatusAt("2026-10-27 14:00");
        outOfSequence.recordProgress(entry(aid, 480, 480)); auto e = entry(bid, 240, 240); e.actualStart = "2026-10-26 14:00"; outOfSequence.recordProgress(e);
        const auto retained = jobs::calculate(*outOfSequence.project());
        check(retained.status == jobs::CalculationStatus::Valid && retained.tasks[0].earlyFinishMinutes == 1200 && retained.tasks[1].earlyFinishMinutes == 1440 &&
              retained.tasks[1].remainingStart == "2026-10-28 14:00" && retained.tasks[1].start == e.actualStart && diagnostic(retained, "OUT_OF_SEQUENCE"), "F08 retained remaining / actual preservation mismatch");
        auto noBaseline = jobs::compareToBaseline(*outOfSequence.project(), ""); check(!noBaseline.forecastFinishVariance && !noBaseline.tasks[0].baselineFinish, "missing baseline became zero variance");
        jobs::Document milestone(store); milestone.create("milestone history", "2026-10-26", {"M"}); milestone.setDuration(0, 0); milestone.setStatusAt("2026-10-26 18:00");
        auto milestoneReport = entry(milestone.project()->tasks[0].id, 0, 0); milestoneReport.actualStart.reset(); milestone.recordProgress(milestoneReport);
        const auto oldMilestoneReport = milestone.project()->progress[0]; milestone.setDuration(0, 480);
        check(milestone.project()->progress[0] == oldMilestoneReport && oldMilestoneReport.planDurationMinutes == 0, "plan edit revalidated/rewrote historical milestone report");
        rejects([&] { milestone.recordProgress(milestoneReport); });
        outOfSequence.addTask("new unknown"); const auto incomplete = jobs::calculate(*outOfSequence.project());
        check(incomplete.status == jobs::CalculationStatus::Incomplete && !incomplete.projectFinish && incomplete.tasks.empty(), "incomplete plan produced partial forecast");
        store.stored = *doc.project(); jobs::Document readOnly(store); readOnly.open("fixture", true);
        rejects([&] { readOnly.captureBaseline("blocked", "PM", "reason", "2026-10-26T00:00:00Z"); });
        rejects([&] { readOnly.setStatusAt("2026-10-26 18:00"); }); rejects([&] { readOnly.recordProgress(entry(id, 480, 480)); });
        std::cout << "PASS: F09 +8h, immutable/versioned baseline, plan-vs-forecast variance, calendar snapshot, as-of corrections, validation/atomicity, completed actuals, F08 retained logic, unknowns/read-only\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
