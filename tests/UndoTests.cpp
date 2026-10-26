#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <functional>
#include <iostream>
namespace {
void check(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
void rejects(const std::function<void()>& f) { bool bad = false; try { f(); } catch (const std::exception&) { bad = true; } check(bad, "expected rejected edit/undo"); }
QByteArray read(const QString& p) { QFile f(p); check(f.open(QIODevice::ReadOnly), "read"); return f.readAll(); }
struct State { jobs::PlanInput plan; std::optional<std::string> status; };
State state(const jobs::Document& d) { return {static_cast<const jobs::PlanInput&>(*d.project()), d.project()->statusAt}; }
bool same(const jobs::Document& d, const State& s) { return static_cast<const jobs::PlanInput&>(*d.project()) == s.plan && d.project()->statusAt == s.status; }
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir; jobs::JsonProjectStore store; jobs::Document d(store); rejects([&] { d.undo(); }); d.create("undo", "2026-10-26", {"A", "B"}); check(!d.canUndo(), "creation undo crossed document boundary");
        d.setDuration(0, 480); auto path = dir.filePath("undo.jobs"); d.saveAs(path.toStdString()); check(d.canUndo() && !d.dirty(), "first save lost edit history"); const auto disk = read(path); const auto saved = *d.project();
        d.renameTask(0, "changed"); d.applyCalculation(jobs::calculate(*d.project())); const auto revision = d.project()->revision; d.undo();
        check(d.project()->tasks[0].name == "A" && d.project()->revision > revision && !d.dirty() && !d.revisionSaved() && d.canRedo() && d.calculation()->status == jobs::CalculationStatus::Stale, "undo rolled revision backward or falsely claimed saved revision/STALE");
        d.undo(); check(!d.project()->tasks[0].durationMinutes && d.dirty(), "undo over save point failed"); d.redo(); check(!d.dirty(), "redo to saved inputs not clean");
        auto before = *d.project(); auto label = d.redoLabel(); rejects([&] { d.setDuration(0, -1); }); check(*d.project() == before && d.redoLabel() == label && !d.dirty(), "failed edit consumed history");
        d.setDuration(0, 480); check(d.canRedo(), "no-op edit cleared redo"); d.renameTask(0, "branch"); check(!d.canRedo(), "new branch retained redo"); check(read(path) == disk && saved.documentId == d.project()->documentId, "undo/redo wrote saved file or changed stable identity");
        d.open(path.toStdString()); check(!d.canUndo() && !d.canRedo(), "reopen retained another session's history");
        std::vector<State> states{state(d)};
        std::string root, child, rid;
        auto step = [&](const std::function<void()>& f) { f(); states.push_back(state(d)); };
        step([&] { d.setCurrency("USD"); }); step([&] { root = d.addWbs("root"); }); step([&] { child = d.addWbs("child", root); });
        step([&] { d.assignWbs(0, child); }); step([&] { d.renameWbs(child, "renamed"); }); step([&] { d.moveWbs(child, ""); });
        step([&] { d.setDuration(1, 240); }); step([&] { d.setPredecessors(1, {d.project()->tasks[0].id}); }); step([&] { d.setHolidays({"2026-11-02"}); });
        step([&] { rid = d.putResource({{}, "team", "labor", 100, {}, {{"2026-10-26", 50}}}); });
        const auto id = d.project()->tasks[0].id; step([&] { d.setAssignments(id, {{id, rid, 50}}); }); step([&] { d.setTaskCost({id, 200, "start", "physical"}); }); step([&] { d.setStatusAt("2026-10-26 18:00"); });
        step([&] { d.addTask("new task", root); }); step([&] { d.assignWbs(0, root); }); step([&] { d.removeWbs(child); });
        for (std::size_t i = states.size() - 1; i > 0; --i) { const auto r = d.project()->revision; d.undo(); check(same(d, states[i - 1]) && d.project()->revision == r + 1, "compound input undo/ID/status mismatch"); }
        for (std::size_t i = 1; i < states.size(); ++i) { d.redo(); check(same(d, states[i]), "redo failed to preserve IDs/input fields"); }
        d.undo(); d.undo(); d.undo(); d.save(); check(d.canUndo() && d.canRedo() && !d.dirty(), "manual save reset history"); d.redo(); check(d.dirty(), "redo after saved point not dirty"); d.undo(); check(!d.dirty(), "undo back to saved point dirty");
        // Approvals/booked actuals are append-only boundaries, never erased by undo.
        auto baseline = d.captureBaseline("approval", "PM", "approved", "2026-10-26T00:00:00Z"); check(!d.canUndo() && !d.historyNotice().empty(), "approved baseline entered removable history");
        const auto frozen = d.project()->baselines; d.setDuration(0, 960); d.undo(); check(d.project()->baselines == frozen, "undo rewrote frozen baseline");
        jobs::ProgressEntry progress; progress.taskId = id; progress.actualStart = "2026-10-26 09:00"; progress.actualDurationMinutes = 240; progress.remainingMinutes = 240; progress.reporter = "PM"; progress.reason = "measured"; progress.recordedAt = "2026-10-26T09:00:00Z"; d.recordProgress(progress); check(!d.canUndo(), "submitted progress can be erased");
        jobs::CostEntry cost; cost.taskId = id; cost.actualMinor = 100; cost.physicalBasisPoints = 4000; cost.reporter = "PM"; cost.reason = "invoice"; cost.recordedAt = progress.recordedAt; d.recordCost(cost); check(!d.canUndo(), "booked actual can be erased");
        auto ledger = d.project()->costEntries; d.renameTask(0, "after facts"); d.undo(); check(d.project()->costEntries == ledger && d.project()->baselines == frozen, "undo changed immutable audit data");
        d.saveScenario("scenario", "task-order", 260, progress.recordedAt, d.project()->documentId, d.project()->revision); check(!d.canUndo(), "saved scenario entered edit history");
        d.renameTask(0, "pending"); d.saveAs(dir.filePath("independent.jobs").toStdString()); check(!d.canUndo(), "independent copy retained source history");
        jobs::Document viewer(store); viewer.open(d.path(), true); rejects([&] { viewer.undo(); }); rejects([&] { viewer.redo(); }); check(!viewer.canUndo() && !viewer.canRedo(), "readonly history enabled");
        jobs::Document bounded(store); bounded.create("bounded", "2026-10-26", {"task"}); for (int i = 1; i <= 150; ++i) bounded.renameTask(0, "task" + std::to_string(i)); int undos = 0; while (bounded.canUndo()) { bounded.undo(); ++undos; } check(undos == 100 && bounded.project()->tasks[0].name == "task50", "history count limit failed");
        jobs::Document large(store); large.create("large", "2026-10-26", {std::string(17 * 1024 * 1024, 'a')}); large.setDuration(0, 480); check(!large.canUndo() && large.historyNotice().find("32 MiB") != std::string::npos, "history memory budget not applied");
        int events = 0; auto token = d.subscribe([&](auto) { ++events; }); d.renameTask(0, "event"); d.unsubscribe(token); d.renameTask(0, "after unsubscribe"); check(events == 1, "document listener lifetime failed");
        (void)baseline; std::cout << "PASS: atomic input history, monotonic revisions/STALE, saved-content dirty state, save/branch/no-op/error boundaries, stable WBS/resource/cost IDs, immutable facts, count/memory limits, readonly and observers\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
