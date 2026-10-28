#include "TaskTableTest.h"
#include "PerformanceFixture.h"
#include "ui/MainWindow.h"
#include "ui/TaskTableModel.h"
#include "persistence/JsonProjectStore.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QScrollBar>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <iostream>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool wait(const std::function<bool()>& ready) {
    QElapsedTimer timer; timer.start();
    while (!ready() && timer.elapsed() < 5000) QTest::qWait(1);
    return ready();
}
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir dir; jobs::JsonProjectStore store;
        const auto path = dir.filePath("UI.jobs").toUtf8().toStdString();
        store.save(path, performanceFixture(10000, false), {});
        jobs::Document doc(store); doc.open(path);
        for (int i = 0; i < 3; ++i) doc.captureBaseline("B" + std::to_string(i), "PM", "response", "2026-10-28T00:00:00Z");
        doc.save();
        jobs::MainWindow window(doc); window.show(); app.processEvents();
        auto* table = taskTable(window); auto* model = dynamic_cast<jobs::TaskTableModel*>(table->model());
        table->setCurrentCell(9999, 0); table->scrollToBottom(); app.processEvents();
        const auto scroll = table->verticalScrollBar()->value();
        const QPersistentModelIndex retained(cell(table, 9999, 0));
        QSignalSpy resets(model, &QAbstractItemModel::modelReset), changed(model, &QAbstractItemModel::dataChanged);
        auto* baselines = window.findChild<QComboBox*>("baselineChoice");
        QSignalSpy baselineResets(baselines->model(), &QAbstractItemModel::modelReset);
        setCellText(table, 9999, 0, "Visible last edit");
        check(model->lastTaskVisits() == 1 && doc.lastDirtyTaskComparisons() == 2 && retained.isValid() && resets.isEmpty() &&
              baselineResets.isEmpty() && table->currentRow() == 9999 && table->verticalScrollBar()->value() == scroll,
              "single-cell refresh rebuilt the model/view/selection or scanned all tasks");
        for (const auto& signal : changed) check(qvariant_cast<QModelIndex>(signal[0]).row() == 9999 &&
            qvariant_cast<QModelIndex>(signal[1]).row() == 9999, "unrelated rows were invalidated");
        auto* summary = window.findChild<QLabel*>("wbsSummary");
        setCellText(table, 9999, 1, "");
        check(model->lastTaskVisits() == 1 && summary->text().contains("9999h") && summary->text().contains("기간 미정 1개"),
              "incremental unknown-duration rollup mismatch");
        setCellText(table, 9999, 1, "0");
        check(summary->text().contains("9999h") && summary->text().contains("기간 미정 0개"), "zero duration was treated as unknown");
        setCellText(table, 9999, 1, "1");
        check(summary->text().contains("10000h"), "duration rollup was not restored");
        window.findChild<QAction*>("undoEdit")->trigger();
        check(model->lastTaskVisits() == 1 && summary->text().contains("9999h"), "undo did not incrementally update WBS");
        window.findChild<QAction*>("redoEdit")->trigger();
        check(model->lastTaskVisits() == 1 && summary->text().contains("10000h"), "redo did not incrementally update WBS");
        auto* search = window.findChild<QLineEdit*>("taskSearch"); search->setText("Visible last edit");
        check(wait([&] { return table->rowCount() == 1; }), "search mapping failed");
        setCellText(table, 0, 0, "Gone"); check(table->rowCount() == 0, "search edit did not take full mapping fallback");
        search->clear(); check(wait([&] { return table->rowCount() == 10000; }), "clearing search did not restore mapping");
        doc.applyCalculation(jobs::calculate(*doc.project()));
        setCellText(table, 0, 0, "Stale calculation");
        check(model->lastTaskVisits() == 10000 && doc.calculation()->status == jobs::CalculationStatus::Stale &&
              cellText(table, 0, 3) == "미계산", "calculation invalidation bypassed full fallback");
        setCellText(table, 0, 0, "Next edit"); check(model->lastTaskVisits() == 1, "steady stale edit did not return to incremental path");
        doc.renameTask(1, "Skipped one"); doc.renameTask(2, "Skipped two");
        check(!model->synchronizeTask(), "model accepted a nonconsecutive change hint");
        std::vector<std::size_t> rows(10000); for (std::size_t i = 0; i < rows.size(); ++i) rows[i] = i;
        model->synchronize(rows); check(model->lastTaskVisits() == 10000 && cellText(table, 2, 0) == "Skipped two", "skipped-revision fallback failed");
        std::cout << "PASS: 10k/3 baseline bounded model updates, selection/scroll, WBS delta, search, stale and missed-revision fallbacks\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
