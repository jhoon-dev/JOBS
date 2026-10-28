#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <functional>
#include <iostream>
namespace {
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
void rejects(const std::function<void()> &f) {
    bool bad = false;
    try {
        f();
    } catch (const std::exception &) {
        bad = true;
    }
    check(bad, "expected atomic rejection");
}
jobs::PlanInput plan(const jobs::Document &d) {
    return static_cast<const jobs::PlanInput &>(*d.project());
}
QByteArray bytes(const std::string &path) {
    QFile f(QString::fromStdString(path));
    check(f.open(QIODevice::ReadOnly), "file read failed");
    return f.readAll();
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document d(store);
        d.create("Editing", "2026-10-26", {"A", "B", "C"});
        for (int i = 0; i < 3; ++i)
            d.setDuration(i, 60);
        const auto a = d.project()->tasks[0].id, b = d.project()->tasks[1].id, c = d.project()->tasks[2].id;
        const auto wbs = d.addWbs("Engineering");
        d.assignWbs(1, wbs);
        jobs::Resource resource;
        resource.name = "Engineer";
        const auto rid = d.putResource(resource);
        d.setAssignments(b, {{b, rid, 100}});
        d.setTaskCost({b, 123, "start", "physical"});
        d.setDependencies(1, {{a, jobs::DependencyType::SS, 60}});
        d.setDependencies(2, {{b, jobs::DependencyType::FF, -30}});
        d.setTaskEffort(1, jobs::TaskType::FixedUnits, false, 60, {}, {{"2026-10-26 13:00", "2026-10-26 14:00"}});
        d.captureBaseline("Approved", "PM", "Plan", "2026-10-26T00:00:00Z");
        const auto baselines = d.project()->baselines;
        d.saveAs(dir.filePath("editing.jobs").toStdString());
        const auto disk = bytes(d.path());
        const auto before = plan(d);
        const auto revision = d.project()->revision;
        const auto copies = d.duplicateTasks({c, b});
        check(copies.size() == 2 && d.project()->revision == revision + 1 && d.project()->tasks[3].id == copies[0] &&
                  d.project()->tasks[4].id == copies[1],
              "duplicate order or revision");
        check(d.project()->tasks[3].name == "B (복사)" &&
                  d.project()->tasks[3].predecessors == std::vector<std::string>{a} &&
                  d.project()->tasks[4].predecessors == std::vector<std::string>{copies[0]} &&
                  d.project()->tasks[4].dependencyRules[0].predecessorId == copies[0],
              "duplicate internal/external links");
        auto expected = d.project()->tasks[1];
        expected.id = copies[0];
        expected.name += " (복사)";
        check(d.project()->tasks[3] == expected && d.project()->assignments.back().taskId == copies[0] &&
                  d.project()->taskCosts.back().taskId == copies[0] && d.project()->taskCosts.back().fixedMinor == 123,
              "duplicate lost plan/configuration");
        check(d.project()->baselines == baselines && d.project()->progress.empty() && d.project()->costEntries.empty(),
              "duplicate changed facts");
        const auto duplicated = plan(d);
        d.undo();
        check(plan(d) == before && d.canRedo(), "duplicate undo not one step");
        d.redo();
        check(plan(d) == duplicated, "duplicate redo changed IDs");
        const auto info = d.previewRemoveTasks({b});
        check(info.tasks == 1 && info.links == 2 && info.assignments == 1 && info.costs == 1 &&
                  info.protectedTaskIds.empty(),
              "delete preview counts");
        d.removeTasks({b});
        check(d.project()->tasks[1].id == c && d.project()->tasks[1].predecessors.empty() &&
                  d.project()->tasks[1].dependencyRules.empty() && d.project()->assignments.size() == 1 &&
                  d.project()->taskCosts.size() == 1 && d.project()->baselines == baselines,
              "delete references or frozen baseline");
        d.undo();
        check(plan(d) == duplicated, "delete undo not atomic");
        const auto inserted = d.insertTasks({"New 1", "New 2"}, b, wbs);
        check(d.project()->tasks[1].id == inserted[0] && d.project()->tasks[2].id == inserted[1] &&
                  d.project()->tasks[1].wbsId == wbs,
              "insert position or WBS");
        d.undo();
        check(plan(d) == duplicated, "insert undo not atomic");
        d.redo();
        check(d.project()->tasks[1].id == inserted[0], "insert redo ID changed");
        d.undo();
        auto unchanged = *d.project();
        const auto label = d.undoLabel();
        rejects([&] { d.insertTasks({"Valid", "  "}, b); });
        rejects([&] { d.insertTasks({"Valid"}, "missing"); });
        rejects([&] { d.duplicateTasks({a, a}); });
        rejects([&] { d.removeTasks({a, "missing"}); });
        check(*d.project() == unchanged && d.undoLabel() == label, "failed structural edit changed history/data");
        jobs::TaskEdit ea;
        ea.taskId = a;
        ea.name = "A revised";
        ea.duration.emplace(120);
        jobs::TaskEdit eb;
        eb.taskId = b;
        eb.name = "B revised";
        eb.duration.emplace(45);
        d.editTasks({eb, ea});
        check(d.project()->tasks[0].name == "A revised" && d.project()->tasks[1].durationMinutes == 45,
              "batch targets or duration");
        d.undo();
        check(plan(d) == duplicated, "paste undo not one step");
        d.redo();
        check(d.project()->tasks[0].durationMinutes == 120, "paste redo failed");
        unchanged = *d.project();
        eb.name = "";
        rejects([&] { d.editTasks({ea, eb}); });
        eb.name = "Good";
        eb.edges = std::vector<jobs::Dependency>{{c, jobs::DependencyType::FS, 0}};
        rejects([&] { d.editTasks({eb}); });
        eb.edges = std::vector<jobs::Dependency>{{b, jobs::DependencyType::FS, 0}};
        rejects([&] { d.editTasks({eb}); });
        eb.edges = std::vector<jobs::Dependency>{{"missing", jobs::DependencyType::FS, 0}};
        rejects([&] { d.editTasks({eb}); });
        check(*d.project() == unchanged && bytes(d.path()) == disk, "batch rejection changed input/file");
        d.setAssignments(a, {{a, rid, 100}});
        d.setTaskEffort(0, jobs::TaskType::FixedWork, true, 60, 6000, {});
        d.setDuration(1, {});
        ea.duration.emplace(120);
        ea.name = "Fixed work";
        eb.edges.reset();
        eb.name = "Units";
        d.editTasks({eb, ea});
        check(d.project()->tasks[0].workUnitMinutes == 6000 && d.project()->assignments.back().taskId == a &&
                  d.project()->assignments.back().units == 50 && d.project()->tasks[1].durationMinutes == 45,
              "batch bypassed fixed-work/unit rules");
        d.setStatusAt("2026-10-26 10:00");
        jobs::ProgressEntry progress;
        progress.taskId = a;
        progress.actualStart = "2026-10-26 09:00";
        progress.actualDurationMinutes = 30;
        progress.remainingWorkUnitMinutes = 3000;
        progress.reporter = "PM";
        progress.reason = "Measured";
        progress.recordedAt = "2026-10-26T01:00:00Z";
        d.recordProgress(progress);
        jobs::CostEntry cost;
        cost.taskId = c;
        cost.actualMinor = 0;
        cost.physicalBasisPoints = 0;
        cost.reporter = "PM";
        cost.reason = "Zero confirmed";
        cost.recordedAt = progress.recordedAt;
        d.recordCost(cost);
        const auto facts = d.project()->progress;
        const auto costs = d.project()->costEntries;
        unchanged = *d.project();
        check(d.previewRemoveTasks({a, b, c}).protectedTaskIds.size() == 2, "reported tasks not protected");
        rejects([&] { d.removeTasks({a, b}); });
        rejects([&] { d.removeTasks({b, c}); });
        check(*d.project() == unchanged, "protected deletion removed unreported subset");
        d.duplicateTasks({a});
        check(d.project()->progress == facts && d.project()->costEntries == costs &&
                  d.project()->baselines == baselines,
              "duplicate copied actual facts");
        d.save();
        jobs::Document reader(store);
        reader.open(d.path(), true);
        check(*reader.project() == *d.project(), "editing save/reopen changed IDs/facts");
        unchanged = *reader.project();
        rejects([&] { reader.insertTasks({"X"}); });
        rejects([&] { reader.removeTasks({b}); });
        rejects([&] { reader.duplicateTasks({b}); });
        rejects([&] { reader.editTasks({ea}); });
        check(*reader.project() == unchanged, "readonly batch changed data");
        d.renameTask(1, "Unsaved batch recovery");
        jobs::JsonRecoveryStore recovery(dir.filePath("recovery"));
        auto session = recovery.newSession();
        session->checkpoint(d.recoverySnapshot("2026-10-26T01:00:01Z"));
        session.reset();
        const auto scan = recovery.scan();
        check(scan.candidates.size() == 1 && scan.candidates[0].valid, "batch recovery missing");
        auto claim = recovery.claim(scan.candidates[0]);
        jobs::Document restored(store);
        restored.recover(claim->snapshot);
        check(plan(restored) == plan(d) && restored.project()->progress == facts &&
                  restored.project()->costEntries == costs && restored.project()->baselines == baselines,
              "batch recovery lost state");
        claim->session->retire();
        std::cout << "PASS: insert/delete/duplicate references and IDs, protected ledgers, atomic batch/fixed-work "
                     "rules, undo/redo, failure/readonly, file and recovery\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
