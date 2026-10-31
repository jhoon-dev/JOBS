#include "TaskTableTest.h"
#include "calculation/Reports.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/TaskTableModel.h"
#include <QAction>
#include <QApplication>
#include <QElapsedTimer>
#include <QFile>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <functional>
#include <iostream>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
void waitFor(const std::function<bool()> &ready) {
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < 10000)
        QTest::qWait(10);
    check(ready(), "duration UI computation timed out");
}
QByteArray bytes(const std::string &path) {
    QFile file(QString::fromStdString(path));
    check(file.open(QIODevice::ReadOnly), "saved fixture read failed");
    return file.readAll();
}
void workTask(jobs::Document &doc, bool fixedUnits, std::optional<std::int64_t> nominal = 480,
              std::int64_t work = 48000, int units = 50, bool split = false) {
    const auto id = doc.project()->tasks[0].id;
    const auto resource = doc.putResource({{}, "R", "labor", 100, {}});
    doc.setAssignments(id, {{id, resource, units}});
    doc.setTaskEffort(0, fixedUnits ? jobs::TaskType::FixedUnits : jobs::TaskType::FixedWork, true, nominal, work,
                      split ? std::vector<jobs::TaskBreak>{{"2026-10-26 13:00", "2026-10-27 13:00"}}
                            : std::vector<jobs::TaskBreak>{});
}
void unchangedEditor(jobs::JsonProjectStore &store, const std::string &path, bool fixedUnits) {
    jobs::Document doc(store);
    doc.create("Unchanged duration", "2026-10-26", {"A"});
    workTask(doc, fixedUnits, 480, fixedUnits ? 48000 : 1001, fixedUnits ? 50 : 100);
    doc.renameTask(0, "Temporary");
    doc.undo();
    check(doc.canRedo(), "redo fixture missing");
    doc.applyCalculation(jobs::calculate(*doc.project()));
    doc.saveAs(path);
    const auto source = *doc.project();
    const auto saved = bytes(path);
    const auto undo = doc.undoLabel(), redo = doc.redoLabel();
    const auto history = doc.historyBytes();
    const auto generation = doc.calculationGeneration();
    jobs::MainWindow window(doc);
    window.show();
    auto *view = taskTable(window);
    auto *edit = window.findChild<QAction *>("editTaskCell");
    auto *cancel = window.findChild<QAction *>("cancelEdit");
    check(view && edit && cancel, "duration editor actions absent");
    for (bool enter : {true, false}) {
        view->setCurrentCell(0, 1);
        edit->trigger();
        auto *editor = view->findChild<QLineEdit *>();
        check(editor && editor->isVisible() && editor->text() == (fixedUnits ? "960m" : "11m"),
              "computed duration editor did not open");
        if (enter)
            QTest::keyClick(editor, Qt::Key_Return);
        else
            view->commitEditors();
        QTest::qWait(30);
        waitFor([&] { return !cancel->isEnabled(); });
        check(*doc.project() == source && !doc.dirty() && doc.revisionSaved() &&
                  doc.calculationGeneration() == generation && doc.historyBytes() == history &&
                  doc.undoLabel() == undo && doc.redoLabel() == redo && doc.canRedo() && bytes(path) == saved,
              "unchanged editor mutated work, units, basis, revision, calculation, file or history");
    }
    check(setCellText(view, 0, 1, fixedUnits ? "8" : "20m"), "changed duration edit rejected");
    waitFor([&] { return doc.project()->revision != source.revision && !cancel->isEnabled(); });
    check(doc.dirty() && !doc.canRedo(), "real edit did not create history");
    if (fixedUnits)
        check(!doc.project()->tasks[0].workDriven && !doc.project()->tasks[0].workUnitMinutes &&
                  doc.project()->tasks[0].durationMinutes == 480 && doc.project()->assignments == source.assignments,
              "real fixed-units duration edit lost basis conversion");
    else
        check(doc.project()->tasks[0].workUnitMinutes == 1001 && doc.project()->assignments != source.assignments,
              "real fixed-work duration edit lost inverse units calculation");
    window.hide();
}
const jobs::ReportTable &reportTable(const jobs::ProjectReport &report, const std::string &id) {
    for (const auto &table : report.tables)
        if (table.id == id)
            return table;
    throw std::runtime_error("duration report table absent");
}
void periods(jobs::JsonProjectStore &store, bool fixedUnits, bool nominal, bool split) {
    jobs::Document doc(store);
    doc.create("Calculated periods", "2026-10-26", {"A work", "B known", "C unknown"});
    workTask(doc, fixedUnits, nominal ? std::optional<std::int64_t>{480} : std::nullopt, 48000, 50, split);
    doc.setDuration(1, 60);
    jobs::TaskTableModel model(doc);
    auto sync = [&] {
        doc.applyCalculation(jobs::calculate(*doc.project()));
        model.synchronize({0, 1, 2});
    };
    auto duration = [&](int row) { return model.data(model.index(row, 1)).toString(); };
    jobs::ReportOptions options;
    options.reportId = "duration-regression";
    options.generatedAt = "2026-10-30T00:00:00Z";
    sync();
    check(doc.calculation()->status == jobs::CalculationStatus::Incomplete && duration(0).isEmpty() &&
              duration(1) == "1" && duration(2).isEmpty(),
          "incomplete plan exposed nominal work duration or hid known input duration");
    const auto incomplete = jobs::buildReport(*doc.project(), options);
    check(std::holds_alternative<std::monostate>(reportTable(incomplete, "schedule").rows[0][3].value) &&
              std::holds_alternative<std::monostate>(reportTable(incomplete, "effort").rows[0][6].value) &&
              std::get<std::int64_t>(reportTable(incomplete, "schedule").rows[1][3].value) == 60,
          "incomplete report used ignored nominal duration or lost known duration");
    doc.setDuration(2, 60);
    sync();
    check(doc.calculation()->status == jobs::CalculationStatus::Valid && duration(0) == "960m",
          "plan duration did not use work, assignment and split calendar");
    const auto valid = jobs::buildReport(*doc.project(), options);
    check(std::get<std::int64_t>(reportTable(valid, "schedule").rows[0][3].value) == 960,
          "valid report lost computed plan duration");
    doc.setStatusAt("2026-10-30 18:00");
    jobs::ProgressEntry entry;
    entry.taskId = doc.project()->tasks[0].id;
    entry.statusAt = "2026-10-30 18:00";
    entry.recordedAt = "2026-10-30T10:00:00Z";
    entry.reporter = "Tester";
    entry.reason = "Completed";
    entry.actualStart = "2026-10-26 09:00";
    entry.actualFinish = "2026-10-26 10:00";
    entry.actualDurationMinutes = 60;
    entry.remainingWorkUnitMinutes = 0;
    doc.recordProgress(entry);
    sync();
    check(doc.calculation()->status == jobs::CalculationStatus::Valid && doc.calculation()->isForecast &&
              duration(0) == "960m" &&
              model.data(model.index(0, 4)).toString() == QString::fromStdString(*entry.actualFinish),
          "forecast duration did not retain plan independently from actual dates");
    model.synchronize({1, 0});
    check(duration(1) == "960m", "filtered/reordered view lost planned duration identity");
    doc.setDuration(2, {});
    model.synchronize({0, 1, 2});
    check(duration(0).isEmpty(), "invalidated forecast retained cached duration");
    sync();
    check(doc.calculation()->status == jobs::CalculationStatus::Incomplete && duration(0).isEmpty(),
          "incomplete forecast fell back to nominal duration");
    doc.create("Replacement", "2026-10-26", {"New"});
    model.synchronize({0});
    check(duration(0).isEmpty(), "new session retained old planned duration");
}
void forecastUi(jobs::JsonProjectStore &store, bool nominal) {
    jobs::Document doc(store);
    doc.create("Forecast duration", "2026-10-26", {"A"});
    workTask(doc, false, nominal ? std::optional<std::int64_t>{480} : std::nullopt);
    doc.applyCalculation(jobs::calculate(*doc.project()));
    jobs::MainWindow window(doc);
    window.show();
    auto *view = taskTable(window);
    check(cellText(view, 0, 1) == "960m", "UI plan duration fixture incorrect");
    doc.setStatusAt("2026-10-26 09:00");
    for (auto *action : window.findChildren<QAction *>())
        if (action->shortcut() == QKeySequence(Qt::Key_F9)) {
            action->trigger();
            break;
        }
    waitFor([&] { return cellText(view, 0, 6) == "예측"; });
    check(cellText(view, 0, 1) == "960m", "worker forecast replaced computed plan duration with nominal input");
    window.hide();
}
void modelNoopGuards(jobs::JsonProjectStore &store, const std::string &path) {
    jobs::Document doc(store);
    doc.create("Guards", "2026-10-26", {"A"});
    doc.setDuration(0, 480);
    jobs::TaskTableModel model(doc);
    model.synchronize({0});
    int calls = 0;
    model.editRequested = [&](std::string, int, QString) {
        ++calls;
        return false;
    };
    QSignalSpy changes(&model, &QAbstractItemModel::dataChanged);
    check(model.setData(model.index(0, 1), " 8 ") && calls == 0 && changes.isEmpty(),
          "unchanged value dispatched callback or unnecessary notification");
    check(!model.setData(model.index(0, 1), "bad") && calls == 1, "changed invalid input bypassed edit validation");
    doc.renameTask(0, "Changed");
    check(!model.setData(model.index(0, 1), "8"), "stale unchanged edit bypassed revision guard");
    doc.saveAs(path);
    doc.open(path, true);
    check(!model.setData(model.index(0, 1), "8"), "unchanged edit bypassed session guard");
    model.synchronize({0});
    check(!model.setData(model.index(0, 1), "8"), "unchanged edit bypassed readonly guard");
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    try {
        QTemporaryDir directory;
        check(directory.isValid(), "temporary directory unavailable");
        jobs::JsonProjectStore store;
        for (bool fixedUnits : {false, true}) {
            unchangedEditor(store, directory.filePath(fixedUnits ? "units.jobs" : "work.jobs").toStdString(),
                            fixedUnits);
            for (bool nominal : {false, true})
                for (bool split : {false, true})
                    periods(store, fixedUnits, nominal, split);
        }
        forecastUi(store, false);
        forecastUi(store, true);
        modelNoopGuards(store, directory.filePath("guards.jobs").toStdString());
        std::cout << "PASS: unchanged duration editor/history, real inverse/basis edits, plan/forecast/incomplete "
                     "periods, report consistency and model safety guards\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
