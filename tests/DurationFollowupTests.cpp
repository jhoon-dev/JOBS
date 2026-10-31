#include "TaskTableTest.h"
#include "calculation/Effort.h"
#include "persistence/JsonProjectStore.h"
#include "ui/AsyncEditController.h"
#include "ui/MainWindow.h"
#include "ui/TaskViewProjection.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QElapsedTimer>
#include <QFile>
#include <QFutureWatcher>
#include <QLineEdit>
#include <QSemaphore>
#include <QTemporaryDir>
#include <QTest>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrentRun>
#include <iostream>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void waitFor(const std::function<bool()> &ready) {
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < 15000)
        QTest::qWait(10);
    check(ready(), "duration follow-up operation timed out");
}
QByteArray bytes(const std::string &path) {
    QFile file(QString::fromStdString(path));
    check(file.open(QIODevice::ReadOnly), "saved fixture unreadable");
    return file.readAll();
}
std::string setup(jobs::Document &doc, bool fixedUnits, bool nominal = true, bool rounding = false) {
    doc.create("Duration follow-ups", "2026-10-26", {"A"});
    const auto id = doc.project()->tasks[0].id;
    const auto resource = doc.putResource({{}, "R", "labor", 100, {}});
    doc.setAssignments(id, {{id, resource, rounding ? 100 : 50}});
    doc.setTaskEffort(0, fixedUnits ? jobs::TaskType::FixedUnits : jobs::TaskType::FixedWork, true,
                      nominal ? std::optional<std::int64_t>{480} : std::nullopt, rounding ? 1001 : 48000, {});
    return resource;
}
jobs::AsyncEditController *controller(jobs::MainWindow &window) {
    auto *result = dynamic_cast<jobs::AsyncEditController *>(window.findChild<QObject *>("asyncEditController"));
    check(result, "edit controller missing");
    return result;
}
void paste(jobs::MainWindow &window, const QString &text) {
    QApplication::clipboard()->setText(text);
    auto *action = window.findChild<QAction *>("pasteTaskCells");
    waitFor([&] { return action && action->isEnabled(); });
    action->trigger();
    waitFor([&] { return !controller(window)->busy(); });
}
void pasteRoundtrip(jobs::JsonProjectStore &store, const std::string &path, bool fixedUnits, bool forecast,
                    bool nominal, bool calculated) {
    jobs::Document doc(store);
    setup(doc, fixedUnits, nominal, !fixedUnits);
    if (forecast)
        doc.setStatusAt("2026-10-26 09:00");
    doc.renameTask(0, "Temporary");
    doc.undo();
    if (calculated)
        doc.applyCalculation(jobs::calculate(*doc.project()));
    doc.saveAs(path);
    const auto source = *doc.project();
    const auto history = doc.historyBytes(), generation = doc.calculationGeneration();
    const auto redo = doc.redoLabel(), undo = doc.undoLabel();
    const auto disk = bytes(path);
    jobs::MainWindow window(doc);
    window.show();
    auto *view = taskTable(window);
    for (bool wholeRow : {false, true}) {
        if (!calculated && !wholeRow)
            continue;
        view->setCurrentCell(0, wholeRow ? 0 : 1);
        if (wholeRow)
            selectCellRange(view, 0, 0, 0, 2);
        auto *copy = window.findChild<QAction *>("copyTaskCells");
        check(copy && copy->isEnabled(), "copy action unavailable");
        copy->trigger();
        paste(window, QApplication::clipboard()->text());
        check(*doc.project() == source && !doc.dirty() && doc.revisionSaved() && doc.canRedo() &&
                  doc.redoLabel() == redo && doc.undoLabel() == undo && doc.historyBytes() == history &&
                  doc.calculationGeneration() == generation && bytes(path) == disk,
              "same-cell/row paste changed work, basis, assignments, revision, file or history");
    }
    view->setCurrentCell(0, 0);
    const auto period = cellText(view, 0, 1);
    paste(window, "Renamed\t" + period + "\t");
    auto renamed = source.tasks;
    renamed[0].name = "Renamed";
    check(doc.project()->tasks == renamed && doc.project()->assignments == source.assignments &&
              doc.project()->revision == source.revision + 1 && doc.dirty(),
          "mixed row paste altered unchanged duration semantics");
    const auto beforeBad = *doc.project();
    paste(window, "Rejected\tinvalid\t");
    check(*doc.project() == beforeBad, "invalid paste partially applied a name or duration");
    view->setCurrentCell(0, 1);
    paste(window, fixedUnits ? "8" : "20m");
    check(doc.project()->revision == beforeBad.revision + 1, "actual duration paste was skipped");
    if (fixedUnits)
        check(!doc.project()->tasks[0].workDriven && !doc.project()->tasks[0].workUnitMinutes &&
                  doc.project()->tasks[0].durationMinutes == 480,
              "actual fixed-units paste lost its basis change");
    else
        check(doc.project()->tasks[0].workUnitMinutes == 1001 && doc.project()->assignments[0].units == 50,
              "actual fixed-work paste lost inverse assignment calculation");
    window.hide();
}
class WorkerGate {
    QSemaphore entered_, release_;
    int threads_ = QThreadPool::globalInstance()->maxThreadCount();
    QFuture<void> future_;
    bool released_ = false;

public:
    WorkerGate() {
        QThreadPool::globalInstance()->setMaxThreadCount(1);
        future_ = QtConcurrent::run([this] {
            entered_.release();
            release_.acquire();
        });
        entered_.acquire();
    }
    void release() {
        if (!released_) {
            released_ = true;
            release_.release();
            future_.waitForFinished();
        }
    }
    ~WorkerGate() {
        release();
        QThreadPool::globalInstance()->setMaxThreadCount(threads_);
    }
};
void revertPending(jobs::JsonProjectStore &store, const std::string &path, bool rounding, bool queued, bool late) {
    jobs::Document doc(store);
    setup(doc, false, true, rounding);
    doc.applyCalculation(jobs::calculate(*doc.project()));
    doc.saveAs(path);
    const auto source = *doc.project();
    const auto history = doc.historyBytes(), generation = doc.calculationGeneration();
    const auto disk = bytes(path);
    jobs::MainWindow window(doc);
    window.show();
    auto *view = taskTable(window);
    const auto original = cellText(view, 0, 1);
    auto *edits = controller(window);
    std::unique_ptr<WorkerGate> gate;
    if (!late)
        gate = std::make_unique<WorkerGate>();
    check(setCellText(view, 0, 1, rounding ? "20m" : "480m") && edits->busy(), "initial edit absent");
    if (queued)
        check(setCellText(view, 0, 1, rounding ? "30m" : "240m"), "queued edit rejected");
    if (late) {
        const auto watchers = edits->findChildren<QFutureWatcherBase *>();
        check(!watchers.isEmpty(), "late completion watcher missing");
        QElapsedTimer timer;
        timer.start();
        while (!watchers.back()->isFinished() && timer.elapsed() < 15000)
            QThread::yieldCurrentThread();
        check(watchers.back()->isFinished(), "worker did not complete before GUI delivery");
        check(setCellText(view, 0, 1, original), "late revert rejected");
    } else {
        view->setCurrentCell(0, 1);
        window.findChild<QAction *>("editTaskCell")->trigger();
        auto *editor = view->findChild<QLineEdit *>();
        check(editor && editor->isVisible(), "revert editor missing");
        editor->selectAll();
        QTest::keyClicks(editor, original);
        check(editor->isModified(), "revert was not typed");
        QTest::keyClick(editor, Qt::Key_Return);
        gate->release();
    }
    waitFor([&] { return !edits->busy(); });
    check(*doc.project() == source && !doc.dirty() && doc.historyBytes() == history &&
              doc.calculationGeneration() == generation && bytes(path) == disk,
          "reverting active/queued/finished duration did not preserve original input/history");
    window.hide();
}
void unrelatedPending(jobs::JsonProjectStore &store) {
    jobs::Document doc(store);
    const auto resource = setup(doc, false, true, true);
    doc.addTask("B");
    const auto bid = doc.project()->tasks[1].id;
    doc.setAssignments(bid, {{bid, resource, 100}});
    doc.setTaskEffort(1, jobs::TaskType::FixedWork, true, 60, 6000, {});
    doc.applyCalculation(jobs::calculate(*doc.project()));
    const auto source = *doc.project();
    jobs::MainWindow window(doc);
    window.show();
    auto *view = taskTable(window);
    const auto original = cellText(view, 0, 1);
    WorkerGate gate;
    check(setCellText(view, 0, 1, "20m") && setCellText(view, 1, 1, "120m"), "two-task queue failed");
    check(setCellText(view, 0, 1, original), "unrelated unchanged edit rejected");
    gate.release();
    waitFor([&] { return !controller(window)->busy(); });
    check(doc.project()->revision == source.revision + 1 && doc.project()->tasks[0] == source.tasks[0] &&
              doc.project()->assignments[0] == source.assignments[0] && doc.project()->assignments[1].units == 50,
          "unchanged cell cancelled another task's newer duration request");
    window.hide();
}
void sortedPeriods(jobs::JsonProjectStore &store, bool fixedUnits, bool nominal, int other, bool split) {
    jobs::Document doc(store);
    setup(doc, fixedUnits, nominal);
    if (split)
        doc.setTaskEffort(0, fixedUnits ? jobs::TaskType::FixedUnits : jobs::TaskType::FixedWork, true,
                          nominal ? std::optional<std::int64_t>{480} : std::nullopt, 48000,
                          {{"2026-10-26 13:00", "2026-10-27 13:00"}});
    doc.addTask("B");
    doc.setDuration(1, other);
    auto order = [&](const jobs::ScheduleResult *result, int sort) {
        jobs::TaskViewSpec spec;
        spec.sort = sort;
        return jobs::projectTaskView(*doc.project(), result, spec).rows;
    };
    auto plan = jobs::calculate(*doc.project());
    for (bool forecast : {false, true}) {
        if (forecast) {
            doc.setStatusAt("2026-10-28 09:00");
            plan = jobs::calculate(*doc.project());
        }
        check(plan.status == jobs::CalculationStatus::Valid, "sort fixture invalid");
        const std::vector<std::size_t> asc =
            other < 960 ? std::vector<std::size_t>{1, 0} : std::vector<std::size_t>{0, 1};
        auto desc = asc;
        std::reverse(desc.begin(), desc.end());
        check(order(&plan, 3) == asc && order(&plan, 4) == desc,
              "duration sort disagreed with planned work duration in plan/forecast");
    }
    for (int sort : {3, 4}) {
        check(order(nullptr, sort) == std::vector<std::size_t>({1, 0}), "uncalculated work period sorted as known");
        plan.status = jobs::CalculationStatus::Stale;
        check(order(&plan, sort) == std::vector<std::size_t>({1, 0}), "stale work period sorted as known");
        plan.status = jobs::CalculationStatus::Incomplete;
        check(order(&plan, sort) == std::vector<std::size_t>({1, 0}), "incomplete work period sorted as known");
    }
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    try {
        QTemporaryDir dir;
        check(dir.isValid(), "temporary directory unavailable");
        jobs::JsonProjectStore store;
        int sequence = 0;
        auto path = [&] { return dir.filePath(QString::number(sequence++) + ".jobs").toStdString(); };
        for (bool units : {false, true}) {
            for (bool forecast : {false, true})
                for (bool nominal : {false, true})
                    pasteRoundtrip(store, path(), units, forecast, nominal, true);
            pasteRoundtrip(store, path(), units, false, true, false);
            for (bool nominal : {false, true})
                for (int other : {600, 1200})
                    for (bool split : {false, true})
                        sortedPeriods(store, units, nominal, other, split);
        }
        for (bool rounding : {false, true}) {
            revertPending(store, path(), rounding, false, false);
            revertPending(store, path(), rounding, true, false);
            revertPending(store, path(), rounding, false, true);
        }
        unrelatedPending(store);
        std::cout << "PASS: no-op and mixed duration paste, real/invalid changes, active/queued/late revert, "
                     "unrelated requests and plan/forecast/unknown period sorting\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
