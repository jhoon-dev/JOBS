#include "TaskTableTest.h"
#include "BuildIdentity.h"
#include "PerformanceFixture.h"
#include "ui/AsyncEditController.h"
#include "ui/GanttView.h"
#include "ui/MainWindow.h"
#include "persistence/JsonProjectStore.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFutureWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSysInfo>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>
#include <QTimer>
#include <algorithm>
#include <iostream>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool wait(const std::function<bool()>& predicate, int timeout = 30000) {
    QElapsedTimer timer; timer.start();
    while (!predicate() && timer.elapsed() < timeout) QTest::qWait(1);
    return predicate();
}
QByteArray bytes(const std::string& path) { QFile file(QString::fromUtf8(path)); check(file.open(QIODevice::ReadOnly), "read failed"); return file.readAll(); }
QAction* action(QWidget& window, const char* name) {
    auto* found = window.findChild<QAction*>(name); check(found, "action missing"); return found;
}
QAction* calculate(QWidget& window) {
    for (auto* candidate : window.findChildren<QAction*>()) if (candidate->shortcut() == QKeySequence(Qt::Key_F9)) return candidate;
    throw std::runtime_error("F9 missing");
}
QAction* save(QWidget& window) {
    for (auto* candidate : window.findChildren<QAction*>()) if (candidate->shortcut() == QKeySequence::Save) return candidate;
    throw std::runtime_error("save missing");
}
QAction* progress(QWidget& window) {
    for (auto* candidate : window.findChildren<QAction*>()) if (candidate->text() == "선택 작업 실적") return candidate;
    throw std::runtime_error("progress action missing");
}
void callbackCheck(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << std::endl; std::_Exit(2); }
}
void editDuration(jobs::TaskTableView* table, const QString& text) {
    table->setCurrentCell(0, 1); table->setFocus(); table->edit(cell(table, 0, 1));
    QLineEdit* editor = nullptr;
    for (auto* candidate : table->findChildren<QLineEdit*>()) if (candidate->isVisible()) editor = candidate;
    check(editor, "duration editor missing"); editor->setText(text);
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv); app.setQuitOnLastWindowClosed(false);
    QTimer watchdog; watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "FAIL: asynchronous editing watchdog\n"; std::_Exit(2); }); watchdog.start(150000);
    try {
        QTemporaryDir directory; jobs::JsonProjectStore store;
        const auto fixture = performanceFixture(10000, true);
        const auto path = directory.filePath("AsyncPlan.jobs").toUtf8().toStdString(); store.save(path, fixture, {});
        jobs::Document document(store); document.open(path);
        jobs::MainWindow window(document); window.show(); app.processEvents();
        auto* table = taskTable(window);
        auto* controller = dynamic_cast<jobs::AsyncEditController*>(window.findChild<QObject*>("asyncEditController"));
        check(table && controller && table->rowCount() == 10000 && window.findChildren<QTableWidget*>().isEmpty(), "task table is not model/view only");
        auto before = *document.project(); const auto disk = bytes(path);
        auto* cancel = action(window, "cancelEdit");
        int heartbeats = 0; qint64 maxGapNs = 0; bool measuredPhase = true;
        QElapsedTimer heartbeatClock; heartbeatClock.start();
        QTimer heartbeat; heartbeat.setInterval(1);
        QObject::connect(&heartbeat, &QTimer::timeout, [&] {
            if (measuredPhase) maxGapNs = std::max(maxGapNs, heartbeatClock.nsecsElapsed());
            heartbeatClock.restart();
            if (controller->busy()) ++heartbeats;
        }); heartbeat.start();
        QElapsedTimer elapsed; elapsed.start();
        check(setCellText(table, 0, 1, "2") && controller->busy() && cancel->isEnabled(), "inverse edit was synchronous or cancel unavailable");
        const double enqueueMs = double(elapsed.nsecsElapsed()) / 1000000;
        bool running = false;
        QTimer cancelWhenRunning; cancelWhenRunning.setInterval(1);
        QObject::connect(&cancelWhenRunning, &QTimer::timeout, [&] {
            const auto control = controller->control();
            if (control && control->stage.load() == jobs::CalculationStage::Forward && control->completed.load() > 10) {
                running = true; cancelWhenRunning.stop(); cancel->trigger();
            }
        }); cancelWhenRunning.start();
        check(wait([&] { return !controller->busy(); }) && running && heartbeats > 0, "GUI heartbeat or running cancellation failed");
        measuredPhase = false;
        cancelWhenRunning.stop();
        check(*document.project() == before && !document.dirty() && !document.canUndo() && bytes(path) == disk, "cancel modified the input/history/source file");
        std::cout << "stage: responsive running cancellation" << std::endl;
        // Let a worker finish without delivering its queued GUI completion, then cancel.
        heartbeat.stop();
        check(setCellText(table, 0, 1, "2"), "late-cancel edit rejected");
        const auto watchers = controller->findChildren<QFutureWatcherBase*>(); check(!watchers.isEmpty(), "worker watcher missing");
        auto* watcher = watchers.back();
        QElapsedTimer lateClock; lateClock.start();
        while (!watcher->isFinished() && lateClock.elapsed() < 30000) QThread::yieldCurrentThread();
        check(watcher->isFinished(), "worker did not finish");
        cancel->trigger(); check(wait([&] { return !controller->busy(); }) && *document.project() == before, "completed worker ignored cancellation before GUI commit");
        heartbeatClock.restart(); heartbeat.start();
        measuredPhase = true; heartbeatClock.restart(); elapsed.restart(); setCellText(table, 0, 1, "2");
        check(wait([&] { return !controller->busy(); }) && document.project()->tasks[0].durationMinutes == 120 &&
              document.project()->tasks[0].workUnitMinutes == 6000 && document.project()->assignments[0].units == 50,
              "asynchronous inverse edit did not preserve work/apply assignments");
        const double settledMs = double(elapsed.nsecsElapsed()) / 1000000;
        measuredPhase = false;
        action(window, "undoEdit")->trigger();
        check(static_cast<const jobs::PlanInput&>(*document.project()) == static_cast<const jobs::PlanInput&>(fixture) && !document.dirty(), "asynchronous edit was not one undo");
        before = *document.project();
        setCellText(table, 0, 1, "3"); setCellText(table, 1, 0, "Concurrent name");
        check(wait([&] { return !controller->busy(); }) && document.project()->tasks[1].name == "Concurrent name" &&
              document.project()->tasks[0] == before.tasks[0], "background edit overwrote a concurrent edit");
        action(window, "undoEdit")->trigger();
        const auto revision = document.project()->revision;
        setCellText(table, 0, 1, "2"); setCellText(table, 0, 1, "2.5"); setCellText(table, 0, 1, "3");
        check(wait([&] { return !controller->busy(); }) && document.project()->revision == revision + 1 &&
              document.project()->tasks[0].durationMinutes == 180 && cellText(table, 0, 1) == "3", "latest-only pending edit applied intermediate requests");
        action(window, "undoEdit")->trigger(); document.save(); before = *document.project();
        setCellText(table, 0, 1, "2"); document.open(path);
        check(wait([&] { return !controller->busy(); }) && *document.project() == before, "reload with identical ID/revision accepted stale session");
        std::cout << "stage: stale session, concurrent and latest-only edits" << std::endl;
        const auto savedBefore = bytes(path);
        editDuration(table, "2"); save(window)->trigger();
        check(controller->busy() && bytes(path) == savedBefore, "save ran before pending cell edit");
        check(wait([&] { return !controller->busy(); }) && !document.dirty() && document.project()->tasks[0].durationMinutes == 120 &&
              store.load(path).project == *document.project(), "queued save did not include committed inverse edit");
        before = *document.project(); const auto savedAfter = bytes(path); const auto history = document.historyBytes();
        editDuration(table, "3"); save(window)->trigger(); cancel->trigger();
        check(wait([&] { return !controller->busy(); }) && *document.project() == before && bytes(path) == savedAfter &&
              document.historyBytes() == history, "cancelled cell edit still executed queued save/history");
        editDuration(table, "2.5"); calculate(window)->trigger();
        check(wait([&] { return !controller->busy() && document.calculation() && document.calculation()->status == jobs::CalculationStatus::Valid &&
                    document.calculation()->inputRevision == document.project()->revision; }), "F9 did not wait for pending edit");
        check(document.project()->tasks[0].durationMinutes == 150, "F9 used the old task input");
        auto* gantt = dynamic_cast<jobs::GanttView*>(window.findChild<QGraphicsView*>("ganttView")); check(gantt, "Gantt missing");
        gantt->taskEdited(document.project()->documentId, document.project()->revision, "T0", true, "2026-10-27 12:00");
        check(controller->busy() && wait([&] { return !controller->busy() && document.calculation() &&
            document.calculation()->status == jobs::CalculationStatus::Valid && document.calculation()->inputRevision == document.project()->revision; }),
            "Gantt effort resize did not prepare/recalculate asynchronously");
        before = *document.project();
        table->setCurrentCell(0, 1); QApplication::clipboard()->setText("2");
        auto* paste = action(window, "pasteTaskCells");
        check(wait([&] { return QApplication::clipboard()->text() == "2" && paste->isEnabled(); }),
              "native clipboard/paste action did not become ready");
        paste->trigger();
        check(controller->busy(), "fixed-work paste did not submit asynchronous preparation");
        check(wait([&] { return !controller->busy(); }) && document.project()->tasks[0].durationMinutes == 120,
              "fixed-work paste did not apply the prepared duration");
        action(window, "undoEdit")->trigger();
        check(static_cast<const jobs::PlanInput&>(*document.project()) == static_cast<const jobs::PlanInput&>(before), "asynchronous paste was not one undo");
        std::cout << "stage: deferred save/F9, Gantt and batch inversion" << std::endl;
        table->selectRow(0); before = *document.project();
        QTimer::singleShot(0, [&] {
            auto* dialog = QApplication::activeModalWidget(); check(dialog && dialog->objectName() == "effortDialog", "effort dialog missing");
            auto* field = dialog->findChild<QLineEdit*>("effortHours"); field->setText("User draft");
            dialog->findChild<QPushButton*>("cancelEffortPreview")->click();
            check(wait([&] { return dialog->findChild<QLabel*>("effortPreview")->text().contains("취소"); }) && field->text() == "User draft",
                  "preview overwrote the draft or ignored cancellation");
            QMetaObject::invokeMethod(dialog, "reject");
        });
        action(window, "taskEffort")->trigger(); check(*document.project() == before, "preview cancellation edited the document");
        document.setStatusAt("2026-10-26 18:00"); calculate(window)->trigger();
        check(wait([&] { return document.calculation() && document.calculation()->status == jobs::CalculationStatus::Valid &&
            document.calculation()->inputRevision == document.project()->revision; }), "effort forecast failed");
        check(document.calculation()->plannedTasks.size() == 10000, "forecast omitted its worker-prepared plan metadata");
        const auto summary = jobs::summarizeWbs(*document.project(), &*document.calculation(), {});
        check(summary.taskCount == 10000 && summary.missingDurations == 0, "forecast rollup lost planned durations");
        before = *document.project(); const auto modalDisk = bytes(path);
        table->selectRow(0);
        QTimer::singleShot(0, [&] {
            auto* dialog = QApplication::activeModalWidget();
            callbackCheck(dialog && dialog->objectName() == "progressDialog", "progress dialog missing");
            dialog->findChild<QLineEdit*>("actualStart")->setText("2026-10-26 09:00");
            dialog->findChild<QSpinBox*>("actualMinutes")->setValue(30);
            auto* remaining = dialog->findChild<QLineEdit*>("remainingWorkHours"); remaining->setText("0.5");
            dialog->findChild<QLineEdit*>("progressReporter")->setText("PM");
            dialog->findChild<QLineEdit*>("progressReason")->setText("Async progress retry");
            auto* accept = dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save);
            accept->click();
            const bool progressRunning = wait([&] { const auto control = controller->control(); return control &&
                control->stage.load() == jobs::CalculationStage::Forward && control->completed.load() > 10; });
            if (!progressRunning) std::cerr << "progress state: " << dialog->findChild<QLabel*>("progressError")->text().toStdString() << std::endl;
            callbackCheck(progressRunning, "progress worker never ran");
            dialog->findChild<QPushButton*>("cancelDialogEdit")->click();
            callbackCheck(wait([&] { return accept->isEnabled(); }) && *document.project() == before && bytes(path) == modalDisk &&
                remaining->text() == "0.5" && dialog->findChild<QLabel*>("progressError")->text().contains("취소"),
                "progress cancellation changed document/file or lost draft");
            accept->click();
        });
        progress(window)->trigger();
        check(document.project()->progress.size() == 1 && document.project()->progress[0].remainingWorkUnitMinutes == 3000 &&
            !document.canUndo(), "progress retry did not append exactly one immutable fact");
        table->selectRow(4); before = *document.project();
        QTimer::singleShot(0, [&] {
            auto* dialog = QApplication::activeModalWidget();
            callbackCheck(dialog && dialog->objectName() == "assignmentDialog", "assignment dialog missing");
            auto* units = dialog->findChild<QSpinBox*>("assignmentUnits_4"); units->setValue(200);
            auto* accept = dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok); accept->click();
            callbackCheck(wait([&] { const auto control = controller->control(); return control &&
                control->stage.load() == jobs::CalculationStage::Forward && control->completed.load() > 10; }), "assignment worker never ran");
            dialog->findChild<QPushButton*>("cancelDialogEdit")->click();
            callbackCheck(wait([&] { return accept->isEnabled(); }) && *document.project() == before && bytes(path) == modalDisk &&
                units->value() == 200 && dialog->findChild<QLabel*>("assignmentError")->text().contains("취소"),
                "assignment cancellation changed document/file or lost draft");
            accept->click();
        });
        action(window, "assignResources")->trigger();
        check(document.project()->tasks[4].workDriven && document.project()->tasks[4].workUnitMinutes == 6000 &&
            std::any_of(document.project()->assignments.begin(), document.project()->assignments.end(), [](const auto& assignment) {
                return assignment.taskId == "T4" && assignment.resourceId == "R4" && assignment.units == 200;
            }) && document.project()->progress == before.progress,
            "assignment retry did not retain original work/immutable progress");
        action(window, "undoEdit")->trigger();
        check(static_cast<const jobs::PlanInput&>(*document.project()) == static_cast<const jobs::PlanInput&>(before) &&
            document.project()->progress == before.progress, "assignment retry was not one undo");
        table->selectRow(10); before = *document.project();
        QTimer::singleShot(0, [&] {
            auto* dialog = QApplication::activeModalWidget();
            callbackCheck(dialog && dialog->objectName() == "assignmentDialog", "closing assignment dialog missing");
            dialog->findChild<QSpinBox*>("assignmentUnits_2")->setValue(200);
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            QMetaObject::invokeMethod(dialog, "reject");
        });
        action(window, "assignResources")->trigger();
        check(wait([&] { return !controller->busy(); }) && *document.project() == before,
            "closing a pending assignment dialog applied results or retained destroyed widgets");
        std::cout << "stage: progress/assignment running cancellation, draft retry, immutable facts and dialog close" << std::endl;
        calculate(window)->trigger();
        check(wait([&] { return document.calculation() && document.calculation()->status == jobs::CalculationStatus::Valid &&
            document.calculation()->inputRevision == document.project()->revision; }), "final progress forecast failed");
        table->setCurrentCell(0, 0); table->selectRow(0); table->scrollToTop(); app.processEvents();
        check(window.grab().save("async-editing-ui.png"), "async UI screenshot failed");
        window.hide();
        // Closing/destroying a view never leaves a worker holding its Document or widgets.
        jobs::Document temporary(store); temporary.open(path, true);
        temporary.saveAs(directory.filePath("Lifetime.jobs").toUtf8().toStdString());
        const auto lifetimeBefore = *temporary.project();
        jobs::MainWindow* dying = nullptr;
        const auto destructionObserver = temporary.subscribe([&](auto event) {
            if (event == jobs::Document::Event::Edited && dying) { delete dying; dying = nullptr; }
        });
        dying = new jobs::MainWindow(temporary); dying->show(); app.processEvents();
        setCellText(taskTable(*dying), 0, 1, "3");
        auto* dyingController = dynamic_cast<jobs::AsyncEditController*>(dying->findChild<QObject*>("asyncEditController"));
        auto dyingControl = dyingController->control();
        temporary.renameTask(1, "Destroy during notification");
        check(!dying, "view did not close during observer delivery");
        const auto lifetimeAfter = *temporary.project();
        check(dyingControl->cancelRequested.load() && wait([] { return QThreadPool::globalInstance()->activeThreadCount() == 0; }) &&
              *temporary.project() == lifetimeAfter && temporary.project()->tasks[0] == lifetimeBefore.tasks[0], "view destruction retained callbacks/live document access");
        temporary.unsubscribe(destructionObserver);
        // An observer may also close the view during the worker's GUI commit.
        dying = new jobs::MainWindow(temporary); dying->show(); app.processEvents();
        const auto commitDestructionObserver = temporary.subscribe([&](auto event) {
            if (event == jobs::Document::Event::Edited && dying) { delete dying; dying = nullptr; }
        });
        setCellText(taskTable(*dying), 0, 1, "3");
        check(wait([&] { return !dying; }) && temporary.project()->tasks[0].durationMinutes == 180,
            "view destruction during prepared commit retained callbacks");
        temporary.unsubscribe(commitDestructionObserver);
        heartbeat.stop();
        QJsonObject metrics{{"status", "success"}, {"task_count", 10000}, {"platform", QApplication::platformName()},
            {"fixture", "jobs-perf-layered-v1"}, {"source_revision", JOBS_PERF_REVISION}, {"built_product_source_sha256", JOBS_SOURCE_SHA256},
            {"qt", qVersion()}, {"os", QSysInfo::prettyProductName()}, {"build_configuration", JOBS_PERF_CONFIG},
            {"enqueue_gui_ms", enqueueMs}, {"successful_edit_settled_ms", settledMs}, {"heartbeat_count_while_editing", heartbeats},
            {"largest_observed_timer_gap_ms", double(maxGapNs) / 1000000},
            {"scope", "Single observation; timer gaps sampled during first running-cancel and one successful inverse edit only; excludes deliberate blocked delivery and test file I/O; not an FPS/worst-latency guarantee"}};
        QFile output("async-editing-ui.json"); check(output.open(QIODevice::WriteOnly), "metrics file failed"); output.write(QJsonDocument(metrics).toJson());
        std::cout << "PASS: 10k model/view heartbeat, running/late cancellation, stale/session/latest-only requests, deferred save/F9, Gantt/batch, preview draft and lifetime safety\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
