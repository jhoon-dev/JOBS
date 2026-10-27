#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QInputDialog>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTableWidget>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QTest>
#include <QTimer>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool c, const char* message) { if (!c) throw std::runtime_error(message); }
void trigger(QWidget& w, const QString& label) {
    for (auto* a : w.findChildren<QAction*>()) if (a->text() == label) { check(a->isEnabled(), "WBS action unexpectedly disabled"); a->trigger(); return; }
    throw std::runtime_error("WBS action missing");
}
void selectWbs(QTreeWidget* tree, const std::string& id) {
    for (QTreeWidgetItemIterator it(tree); *it; ++it) if ((*it)->data(0, Qt::UserRole).toString().toStdString() == id) {
        tree->setCurrentItem(*it); QTest::qWait(10); return;
    }
    throw std::runtime_error("WBS tree item missing");
}
int count(jobs::GanttView* view, const QString& kind) {
    int result = 0; for (auto* item : view->scene()->items()) if (item->data(1).toString() == kind) ++result; return result;
}
QGraphicsItem* bar(jobs::GanttView* view, const std::string& id) {
    for (auto* item : view->scene()->items()) if (item->data(0).toString().toStdString() == id &&
        (item->data(1).toString() == "task" || item->data(1).toString() == "milestone")) return item;
    throw std::runtime_error("Gantt bar missing");
}
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer watchdog; watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "WBS/Gantt modal timeout\n"; std::exit(2); }); watchdog.start(20000);
    try {
        jobs::JsonProjectStore store; jobs::Document doc(store);
        doc.create("WBS·간트 검증", "2026-10-26", {"설계", "개발", "승인", "조달"});
        const auto a = doc.project()->tasks[0].id, b = doc.project()->tasks[1].id;
        doc.setDuration(0, 480); doc.setDuration(1, 960); doc.setDuration(2, 0); doc.setDuration(3, 240);
        doc.setPredecessors(1, {a}); doc.setPredecessors(2, {b});
        auto w1 = doc.addWbs("기획"), w2 = doc.addWbs("실행"), child = doc.addWbs("상세", w2);
        doc.assignWbs(0, w1); doc.assignWbs(1, child); doc.assignWbs(3, w1);
        doc.applyCalculation(jobs::calculate(*doc.project()));
        jobs::MainWindow window(doc); window.show(); QTest::qWait(80);
        auto* table = taskTable(window); auto* tree = window.findChild<QTreeWidget*>("wbsTree");
        // GanttView has no custom metaobject; find by the base class and cast explicitly.
        auto* gantt = dynamic_cast<jobs::GanttView*>(window.findChild<QGraphicsView*>("ganttView"));
        check(gantt && table && tree, "missing WBS/table/Gantt pane");
        check(table->rowCount() == 4 && count(gantt, "task") == 3 && count(gantt, "milestone") == 1 && count(gantt, "dependency") == 2,
              "root Gantt bars/milestone/relations mismatch");
        table->selectRow(1);
        check(dynamic_cast<QAbstractGraphicsShapeItem*>(bar(gantt, b))->pen().widthF() == 3, "table selection did not highlight same ID in Gantt");
        auto* aBar = bar(gantt, a);
        QTest::mouseClick(gantt->viewport(), Qt::LeftButton, Qt::NoModifier, gantt->mapFromScene(aBar->sceneBoundingRect().center()));
        check(table->currentRow() == 0, "Gantt click did not select same ID in table");
        selectWbs(tree, w2);
        check(table->rowCount() == 1 && table->model()->headerData(0, Qt::Vertical).toString() == "2" && cellText(table, 0, 2) == "1", "filtered global row/FS identity mismatch");
        check(count(gantt, "task") == 1 && count(gantt, "dependency") == 0 && bar(gantt, b)->toolTip().contains("WBS 밖 선행: 1"),
              "filtered Gantt hid external dependency evidence");
        setCellText(table, 0, 1, "24");
        check(doc.project()->tasks[1].durationMinutes == 1440 && doc.project()->tasks[0].durationMinutes == 480 && count(gantt, "task") == 0,
              "filtered edit changed wrong task / kept stale Gantt");
        auto recalculate = [&] {
            trigger(window, "일정 계산(&C)");
            for (int i = 0; i < 200; ++i) {
                QTest::qWait(10);
                if (!window.findChild<QPlainTextEdit*>("calculationDiagnostics")->toPlainText().startsWith("계산 중")) break;
            }
            check(doc.calculation()->status == jobs::CalculationStatus::Valid, "WBS calculation not valid");
        };
        recalculate(); check(count(gantt, "task") == 1, "filtered Gantt failed after recalculation");
        table->selectRow(0);
        QTimer::singleShot(50, [] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget()); if (!dialog) return;
            dialog->findChild<QComboBox*>("wbsDestination")->setCurrentIndex(0);
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        trigger(window, "선택 작업의 WBS 변경");
        check(doc.project()->tasks[1].wbsId.empty() && doc.project()->tasks[1].id == b && doc.project()->tasks[1].predecessors == std::vector<std::string>{a} && table->rowCount() == 0,
              "task WBS move lost graph/identity or filter");
        selectWbs(tree, child);
        QTimer::singleShot(50, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget()); if (!dialog) return;
            auto* combo = dialog->findChild<QComboBox*>("wbsDestination");
            if (combo->findData(QString::fromStdString(child)) != -1) { std::cerr << "self move offered\n"; std::exit(2); }
            combo->setCurrentIndex(combo->findData(QString::fromStdString(w1)));
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        trigger(window, "WBS 이동");
        check(doc.project()->wbsNodes[2].parentId == w1, "WBS parent move dialog failed");
        selectWbs(tree, "");
        auto nameDialog = [](const QString& text) {
            QTimer::singleShot(50, [text] {
                auto* dialog = qobject_cast<QInputDialog*>(QApplication::activeModalWidget()); if (!dialog) return;
                dialog->setTextValue(text); dialog->accept();
            });
        };
        nameDialog("검토"); trigger(window, "WBS 추가"); check(doc.project()->wbsNodes.size() == 4, "add WBS dialog failed");
        nameDialog("검토 완료"); trigger(window, "WBS 이름 변경"); check(doc.project()->wbsNodes.back().name == "검토 완료", "rename WBS dialog failed");
        trigger(window, "빈 WBS 삭제"); check(doc.project()->wbsNodes.size() == 3, "empty WBS delete failed");
        recalculate();
        auto* zoom = window.findChild<QComboBox*>("ganttZoom"); const auto dailyWidth = gantt->sceneRect().width();
        zoom->setCurrentIndex(1); check(gantt->sceneRect().width() == dailyWidth / 4 && count(gantt, "dependency") == 2, "zoom changed dates/relations");
        zoom->setCurrentIndex(0); window.grab().save("wbs-gantt-smoke.png");
        // Verify row scrolling uses the same pixel offset when both views have content to scroll.
        for (int i = 0; i < 35; ++i) { doc.addTask("extra" + std::to_string(i)); doc.setDuration(doc.project()->tasks.size() - 1, 0); }
        doc.applyCalculation(jobs::calculate(*doc.project()));
        selectWbs(tree, w1); selectWbs(tree, "");
        table->verticalScrollBar()->setValue(120);
        check(table->verticalScrollBar()->value() == gantt->verticalScrollBar()->value(), "table/Gantt row scroll desynchronized");
        setCellText(table, 0, 1, ""); table->verticalScrollBar()->setValue(120);
        check(count(gantt, "task") == 0 && count(gantt, "milestone") == 0 && table->verticalScrollBar()->value() == gantt->verticalScrollBar()->value(),
              "stale Gantt retained bars or lost empty-row scroll alignment");
        QTimer::singleShot(50, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Discard)->click();
        });
        window.close();
        std::cout << "PASS: WBS tree/filter/global rows, filtered edits, task and subtree moves, add/rename/delete, Gantt IDs/links/milestones/selection/zoom/stale hiding\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
