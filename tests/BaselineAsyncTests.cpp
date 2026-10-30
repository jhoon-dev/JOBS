#include "PerformanceFixture.h"
#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/PrimaveraXer.h"
#include "ui/AsyncEditController.h"
#include "ui/MainWindow.h"
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <future>
#include <iostream>
#include <stdexcept>

namespace {
using jobs::EditState;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
jobs::BaselineEdit approval() {
    return {"Approved", "PM", "Reviewed", "2026-10-30T00:00:00Z"};
}
std::shared_ptr<jobs::Document::PreparedEdit>
prepare(jobs::Document &doc, jobs::BaselineEdit command,
        std::shared_ptr<jobs::CalculationControl> control = std::make_shared<jobs::CalculationControl>()) {
    auto request = doc.makeEditRequest(std::move(command));
    return std::async(std::launch::async,
                      [request = std::move(request), control]() mutable {
                          return jobs::Document::prepareEdit(std::move(request), control);
                      })
        .get();
}
void contracts(jobs::JsonProjectStore &store, const QTemporaryDir &directory) {
    jobs::Document doc(store);
    doc.create("Baseline requests", "2026-10-26", {"Task"});
    doc.setDuration(0, 60);
    doc.saveAs(directory.filePath("Requests.jobs").toStdString());
    auto before = *doc.project();
    const auto history = doc.historyBytes();
    auto prepared = prepare(doc, approval());
    check(prepared->state() == EditState::Ready && *doc.project() == before && !doc.dirty() &&
              doc.historyBytes() == history,
          "baseline preparation changed live input/history");
    auto commit = std::make_shared<jobs::CalculationControl>();
    {
        jobs::EvaluationScope scope(commit);
        check(doc.applyPreparedEdit(*prepared) == EditState::Applied, "prepared baseline was not applied");
    }
    check(commit->checkpoints.load() == 0, "baseline commit recalculated or validated the whole document");
    check(doc.project()->revision == before.revision + 1 && doc.project()->baselines.size() == 1 &&
              doc.project()->baselines[0].version == 1 && !doc.canUndo() && doc.dirty(),
          "approval did not preserve revision/history/dirty rules");
    jobs::verifyBaseline(doc.project()->baselines[0]);
    const auto first = doc.project()->baselines[0];
    check(doc.applyPreparedEdit(*prepared) == EditState::Stale, "baseline approval was applied twice");
    prepared = prepare(doc, approval());
    doc.save();
    check(doc.applyPreparedEdit(*prepared) == EditState::Applied && doc.project()->baselines.size() == 2 &&
              doc.project()->baselines[0] == first && doc.project()->baselines[1].version == 2 &&
              doc.project()->baselines[1].id != first.id && doc.dirty(),
          "normal save during preparation lost version/identity/dirty state");
    doc.setStatusAt("2026-10-26 10:00");
    jobs::ProgressEntry progress;
    progress.taskId = doc.project()->tasks[0].id;
    progress.actualStart = "2026-10-26 09:00";
    progress.actualDurationMinutes = 30;
    progress.remainingMinutes = 30;
    progress.reporter = "PM";
    progress.reason = "Measured";
    progress.recordedAt = "2026-10-30T00:00:00Z";
    doc.recordProgress(progress);
    jobs::CostEntry cost;
    cost.taskId = progress.taskId;
    cost.actualMinor = 100;
    cost.reporter = "PM";
    cost.reason = "Measured";
    cost.recordedAt = progress.recordedAt;
    doc.recordCost(cost);
    before = *doc.project();
    prepared = prepare(doc, approval());
    check(doc.applyPreparedEdit(*prepared) == EditState::Applied && doc.project()->progress == before.progress &&
              doc.project()->costEntries == before.costEntries && doc.project()->baselines[0] == first,
          "baseline approval changed existing facts");
    doc.save();
    check(store.load(doc.path()).project == *doc.project(), "prepared approval failed file roundtrip");
    before = *doc.project();
    auto control = std::make_shared<jobs::CalculationControl>();
    control->cancel();
    prepared = prepare(doc, approval(), control);
    check(prepared->state() == EditState::Cancelled && *doc.project() == before, "pre-cancel changed baseline state");
    control = std::make_shared<jobs::CalculationControl>();
    prepared = prepare(doc, approval(), control);
    control->cancel();
    check(doc.applyPreparedEdit(*prepared) == EditState::Cancelled && *doc.project() == before && !doc.dirty(),
          "late cancellation changed approved facts");
    auto invalid = approval();
    invalid.name.clear();
    prepared = prepare(doc, invalid);
    check(prepared->state() == EditState::Failed && doc.applyPreparedEdit(*prepared) == EditState::Failed &&
              *doc.project() == before,
          "invalid approval metadata changed the live document");
    prepared = prepare(doc, approval());
    doc.renameTask(0, "Concurrent edit");
    before = *doc.project();
    check(doc.applyPreparedEdit(*prepared) == EditState::Stale && *doc.project() == before,
          "concurrent edit accepted an old approval");
    doc.save();
    prepared = prepare(doc, approval());
    doc.open(doc.path());
    check(doc.applyPreparedEdit(*prepared) == EditState::Stale && *doc.project() == before,
          "same ID/revision reload accepted an old approval session");
    prepared = prepare(doc, approval());
    doc.saveAs(directory.filePath("Independent.jobs").toStdString());
    before = *doc.project();
    check(doc.applyPreparedEdit(*prepared) == EditState::Stale && *doc.project() == before,
          "Save As accepted the old document's approval");
    jobs::Document readOnly(store);
    readOnly.open(doc.path(), true);
    bool rejected = false;
    try {
        (void)readOnly.makeEditRequest(approval());
    } catch (const jobs::AppError &) {
        rejected = true;
    }
    check(rejected && *readOnly.project() == before, "read-only approval was allowed");
}
void wait(const std::function<bool()> &ready) {
    QElapsedTimer deadline;
    deadline.start();
    while (!ready() && deadline.elapsed() < 60000)
        QTest::qWait(5);
    check(ready(), "baseline UI worker timeout");
}
QAction *baselineAction(jobs::MainWindow &window) {
    for (auto *action : window.findChildren<QAction *>())
        if (action->text() == "기준선 저장")
            return action;
    throw std::runtime_error("baseline action missing");
}
void approve(jobs::MainWindow &window) {
    QTimer::singleShot(10, [] {
        auto *dialog = QApplication::activeModalWidget();
        if (!dialog)
            return;
        dialog->findChild<QLineEdit *>("baselineName")->setText("Large approval");
        dialog->findChild<QLineEdit *>("baselineApprover")->setText("PM");
        dialog->findChild<QLineEdit *>("baselineReason")->setText("Responsiveness regression");
        dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
    });
    check(baselineAction(window)->isEnabled(), "baseline action remained disabled");
    baselineAction(window)->trigger();
}
void largeUi(jobs::JsonProjectStore &store, const QTemporaryDir &directory, jobs::Project project) {
    const auto path = directory.filePath("Large.jobs").toStdString();
    (void)store.save(path, project, {});
    jobs::Document doc(store);
    doc.open(path);
    jobs::MainWindow window(doc);
    window.show();
    QTest::qWait(50);
    auto *controller = dynamic_cast<jobs::AsyncEditController *>(window.findChild<QObject *>("asyncEditController"));
    check(controller, "baseline async controller missing");
    approve(window);
    check(controller->busy(), "baseline UI bypassed worker preparation");
    auto control = controller->control();
    wait([&] { return control->stage.load() == jobs::CalculationStage::Forward && control->completed.load() > 10; });
    auto *cancel = window.findChild<QAction *>("cancelEdit");
    check(cancel && cancel->isEnabled(), "baseline worker cannot be cancelled");
    cancel->trigger();
    wait([&] { return !controller->busy(); });
    check(doc.project()->baselines.empty() && !doc.dirty() && !doc.canUndo(),
          "running cancellation approved a baseline");
    QElapsedTimer commitClock, beatClock, elapsed;
    qint64 commitNanos = -1, maxBeat = 0;
    int beats = 0;
    bool committedThroughController = false;
    const auto beforeApply = controller->beforeApply;
    controller->beforeApply = [&] {
        if (beforeApply)
            beforeApply();
        commitClock.start();
    };
    const auto listener = doc.subscribe([&](jobs::Document::Event event) {
        if (event == jobs::Document::Event::Edited && !doc.project()->baselines.empty()) {
            committedThroughController = controller->busy();
            commitNanos = commitClock.nsecsElapsed();
        }
    });
    QTimer heartbeat;
    heartbeat.setInterval(5);
    QObject::connect(&heartbeat, &QTimer::timeout, [&] {
        maxBeat = std::max(maxBeat, beatClock.restart());
        ++beats;
    });
    beatClock.start();
    elapsed.start();
    heartbeat.start();
    approve(window);
    wait([&] { return !controller->busy(); });
    heartbeat.stop();
    doc.unsubscribe(listener);
    controller->beforeApply = beforeApply;
    check(committedThroughController && commitNanos >= 0 && beats > 1 && doc.project()->baselines.size() == 1 &&
              doc.project()->baselines[0].schedule.size() == 10000 && doc.dirty() && !doc.canUndo(),
          "large approval lost worker/heartbeat/atomic commit/history contract");
    std::cout << QJsonDocument(QJsonObject{{"tasks", 10000},
                                           {"totalMs", elapsed.elapsed()},
                                           {"commitMs", double(commitNanos) / 1e6},
                                           {"heartbeatMaxMs", maxBeat},
                                           {"heartbeatCount", beats}})
                     .toJson(QJsonDocument::Compact)
                     .constData()
              << '\n';
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    try {
        QTemporaryDir directory;
        check(directory.isValid(), "temporary directory failed");
        jobs::JsonProjectStore store;
        contracts(store, directory);
        auto project = performanceFixture(10000, true);
        const auto option = app.arguments().indexOf("--xer");
        if (option >= 0 && option + 1 < app.arguments().size())
            project =
                jobs::importPrimaveraXer(jobs::readPrimaveraXer(app.arguments()[option + 1]), "review.xer").project;
        largeUi(store, directory, std::move(project));
        std::cout
            << "PASS: baseline worker validation, commit without recalculation, immutable facts, save/session races, "
               "read-only/failure/cancellation and 10k UI heartbeat\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
