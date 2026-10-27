#include "PerformanceFixture.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QLineEdit>
#include <QMessageBox>
#include <QScrollBar>
#include <QSet>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTreeWidget>
#include <QTimer>
#include <iostream>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    bool unexpectedModal = false;
    QTimer modalGuard;
    QObject::connect(&modalGuard, &QTimer::timeout, [&] {
        if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) {
            unexpectedModal = true;
            std::cerr << "Unexpected modal: " << box->text().toStdString() << std::endl;
            box->reject();
        }
    });
    modalGuard.start(50);
    try {
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        const auto path = dir.filePath("EditingUi.jobs").toUtf8().toStdString();
        const auto fixture = performanceFixture(1000, false);
        store.save(path, fixture, {});
        jobs::Document document(store); document.open(path); document.applyCalculation(jobs::calculate(fixture));
        jobs::MainWindow window(document); window.show(); app.processEvents();
        std::cout << "stage: initial" << std::endl;
        auto* table = window.findChild<QTableWidget*>("taskTable");
        auto* tree = window.findChild<QTreeWidget*>("wbsTree");
        auto* undo = window.findChild<QAction*>("undoEdit");
        auto* redo = window.findChild<QAction*>("redoEdit");
        check(table && tree && undo && redo, "editing controls missing");
        const auto* unchangedItem = table->item(999, 0);
        const auto* root = tree->topLevelItem(0);
        table->setCurrentCell(0, 0);
        table->item(0, 0)->setText("First edit"); app.processEvents();
        check(document.calculation()->status == jobs::CalculationStatus::Stale, "edit left a valid calculation");
        for (int row = 0; row < table->rowCount(); ++row)
            check(table->item(row, 3)->text() == "미계산", "cached table left stale dates");
        QSet<int> changedRows;
        QObject::connect(table->model(), &QAbstractItemModel::dataChanged, &window,
                         [&](const QModelIndex& first, const QModelIndex& last) {
                             for (int row = first.row(); row <= last.row(); ++row) changedRows.insert(row);
                         });
        table->item(0, 0)->setText("Second edit"); app.processEvents();
        std::cout << "stage: incremental" << std::endl;
        check(changedRows == QSet<int>{0}, "single edit refreshed unchanged table rows");
        check(table->item(999, 0) == unchangedItem && tree->topLevelItem(0) == root,
              "single edit recreated unrelated cells or the WBS tree");
        changedRows.clear(); undo->trigger(); app.processEvents();
        check(document.project()->tasks[0].name == "First edit" && table->item(0, 0)->text() == "First edit" &&
              changedRows == QSet<int>{0}, "undo failed to update only the changed row");
        redo->trigger(); app.processEvents();
        check(table->item(0, 0)->text() == "Second edit", "redo did not refresh the edited row");
        table->item(0, 0)->setText("  Second edit  "); app.processEvents();
        check(table->item(0, 0)->text() == "Second edit", "no-op edit bypassed display normalization");
        std::cout << "stage: undo/redo" << std::endl;
        table->setCurrentCell(500, 0); table->selectRow(500); table->scrollToItem(table->item(500, 0)); app.processEvents();
        const auto scroll = table->verticalScrollBar()->value();
        const auto id = table->item(500, 8)->text();
        table->item(500, 1)->setText("3"); app.processEvents();
        check(document.project()->tasks[500].durationMinutes == 180 && table->currentRow() == 500 &&
              table->item(500, 8)->text() == id && table->verticalScrollBar()->value() == scroll,
              "refresh changed selected ID, scroll or duration");
        auto* sort = window.findChild<QComboBox*>("taskSort");
        sort->setCurrentIndex(2); app.processEvents();
        const auto currentRow = table->currentRow();
        std::cout << "stage: sort" << std::endl;
        check(currentRow >= 0 && table->item(currentRow, 8)->text() == id, "sort lost the selected stable ID");
        auto* search = window.findChild<QLineEdit*>("taskSearch");
        search->setText("Second edit"); QTest::qWait(220); app.processEvents();
        std::cout << "stage: search" << std::endl;
        check(table->rowCount() == 1 && table->item(0, 8)->text() == "T0", "filter reused the wrong cached row");
        table->setCurrentCell(0, 0); table->item(0, 0)->setText("Hidden by search"); app.processEvents();
        check(table->rowCount() == 0 && document.project()->tasks[0].name == "Hidden by search",
              "rename did not update search membership");
        undo->trigger(); app.processEvents();
        std::cout << "stage: filtered undo" << std::endl;
        check(table->rowCount() == 1 && table->item(0, 0)->text() == "Second edit", "undo failed to restore filtered row");
        search->clear(); QTest::qWait(220); app.processEvents();
        document.save();
        jobs::Document reopened(store); reopened.open(path, true);
        check(*reopened.project() == *document.project(), "optimized editing did not persist unchanged input/facts");
        check(!unexpectedModal, "editing opened an unexpected failure dialog");
        check(window.grab().save("editing-performance-ui.png"), "editing UI screenshot failed");
        window.hide();
        std::cout << "PASS: incremental rows/items/tree, stale dates, normalization, undo/redo, selection, scrolling, sorting/filtering and save\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
