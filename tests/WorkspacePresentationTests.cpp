#include "TaskTableTest.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QGraphicsScene>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>

class WorkspacePresentationTests : public QObject {
    Q_OBJECT
private slots:
    void compactWindowKeepsCommandsAndRows() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        jobs::MainWindow window(document);
        window.resize(1024, 650);
        window.show();
        QTest::qWait(30);
        QCOMPARE(window.findChildren<QToolBar *>().size(), 1);
        QVERIFY(window.findChild<QAction *>("newProject")->isEnabled());
        QVERIFY(window.findChild<QAction *>("openProject")->isEnabled());
        QVERIFY(window.findChild<QLabel *>("projectTitle")->text().contains("시작"));
        auto *table = taskTable(window);
        QVERIFY(table->viewport()->height() >= jobs::GanttView::RowHeight * 4);
        auto *gantt = window.findChild<QGraphicsView *>("ganttView");
        QCOMPARE(table->viewport()->height() + jobs::GanttView::HeaderHeight, gantt->viewport()->height());
        auto *search = window.findChild<QLineEdit *>("taskSearch");
        QVERIFY(search->isVisible() && search->width() >= 180);
        auto *zoom = window.findChild<QComboBox *>("ganttZoom");
        QVERIFY(zoom->isVisible() && zoom->mapTo(&window, zoom->rect().bottomRight()).x() < window.width());
        QVERIFY(!window.findChild<QLabel *>("taskInputHelp")->isVisible());
        QVERIFY(!window.findChild<QPlainTextEdit *>("calculationDiagnostics")->isVisible());
        QVERIFY(!window.findChild<QToolButton *>("cancelCalculationButton")->isVisible());
        QVERIFY(!window.findChild<QToolButton *>("cancelEditButton")->isVisible());
    }
    void cancellationCommandStaysAvailableWhenPresentationChanges() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        document.create("취소 명령", "2026-10-26", {"일정 계산"});
        document.setDuration(0, 480);
        jobs::MainWindow window(document);
        window.show();
        auto *cancel = window.findChild<QAction *>("cancelCalculation");
        auto *button = window.findChild<QToolButton *>("cancelCalculationButton");
        QAction *calculate = nullptr;
        for (auto *action : window.findChildren<QAction *>())
            if (action->shortcut() == QKeySequence(Qt::Key_F9))
                calculate = action;
        QVERIFY(calculate && cancel && button);
        QVERIFY(!button->isVisible() && cancel->isVisible());
        calculate->trigger();
        QVERIFY(cancel->isEnabled());
        // Toolbar presentation is laid out by the GUI's queued layout request.
        // Deliver layout requests without delivering the worker's completion yet.
        QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
        QVERIFY(button->isVisible());
        cancel->trigger();
        QTRY_VERIFY(calculate->isEnabled());
        QCOMPARE(document.calculation()->status, jobs::CalculationStatus::Cancelled);
        QVERIFY(!button->isVisible() && cancel->isVisible());
    }
    void wbsTogglePreservesScopeAndSelection() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        document.create("범위 유지", "2026-10-26", {"설계", "시공", "검수"});
        auto id = document.addWbs("설계 범위");
        document.assignWbs(0, id);
        jobs::MainWindow window(document);
        window.show();
        auto *tree = window.findChild<QTreeWidget *>("wbsTree");
        tree->setCurrentItem(tree->topLevelItem(0)->child(0));
        auto *table = taskTable(window);
        QCOMPARE(table->rowCount(), 1);
        table->selectRow(0);
        auto *toggle = window.findChild<QToolButton *>("toggleWbs");
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(!tree->isVisible());
        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(table->currentRow(), 0);
        QVERIFY(window.findChild<QLabel *>("wbsSummary")->text().contains("설계 범위"));
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(tree->isVisible());
        QCOMPARE(tree->currentItem()->data(0, Qt::UserRole).toString().toStdString(), id);
        auto *menu = window.findChild<QToolButton *>("selectedTaskMenu")->menu();
        auto *constraints = window.findChild<QAction *>("taskConstraints");
        QVERIFY(menu->actions().contains(constraints) && constraints->isEnabled());
    }
    void diagnosticsRevealNewProblemsAndRespectCollapse() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        document.create("진단", "2026-10-26", {"기간 미정"});
        document.applyCalculation(jobs::calculate(*document.project()));
        jobs::MainWindow window(document);
        window.show();
        auto *details = window.findChild<QPlainTextEdit *>("calculationDiagnostics");
        auto *toggle = window.findChild<QToolButton *>("toggleDiagnostics");
        QVERIFY(details->isVisible() && toggle->isChecked());
        QVERIFY(window.findChild<QLabel *>("calculationStatus")->text().contains("불완전"));
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(!details->isVisible());
        window.findChild<QComboBox *>("taskSort")->setCurrentIndex(1);
        QVERIFY(!details->isVisible());
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(details->isVisible());
    }
    void calculatedWorkspaceAndNativeCapture() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        document.create(
            "JOBS 사옥 신축 프로젝트", "2026-10-26",
            {"기획 검토", "기본 설계", "인허가 승인", "기반 공사", "구조 공사", "설비 설치", "시험 운전", "준공 검수"});
        for (std::size_t row = 0; row < document.project()->tasks.size(); ++row) {
            document.setDuration(row, row == 2 ? 0 : (row % 3 + 1) * 480);
            if (row)
                document.setPredecessors(row, {document.project()->tasks[row - 1].id});
        }
        const auto wbs = document.addWbs("프로젝트 실행");
        for (std::size_t row = 0; row < document.project()->tasks.size(); ++row)
            document.assignWbs(row, wbs);
        document.applyCalculation(jobs::calculate(*document.project()));
        jobs::MainWindow window(document);
        window.resize(1440, 900);
        window.show();
        QTest::qWait(40);
        auto *table = taskTable(window);
        auto *gantt = window.findChild<QGraphicsView *>("ganttView");
        QCOMPARE(table->rowCount(), 8);
        QCOMPARE(table->horizontalHeader()->height(), jobs::GanttView::HeaderHeight);
        QCOMPARE(table->verticalHeader()->sectionSize(0), jobs::GanttView::RowHeight);
        QVERIFY(table->viewport()->height() >= 500);
        QVERIFY(!window.findChild<QPlainTextEdit *>("calculationDiagnostics")->isVisible());
        QVERIFY(!gantt->scene()->items().isEmpty());
        table->selectRow(3);
        const auto capture = qEnvironmentVariable("JOBS_UI_SCREENSHOT_DIR");
        if (!capture.isEmpty()) {
            QVERIFY(QDir().mkpath(capture));
            QVERIFY(window.grab().save(QDir(capture).filePath("workspace-1440.png")));
            window.resize(1024, 650);
            QTest::qWait(30);
            QVERIFY(table->viewport()->height() >= 4 * jobs::GanttView::RowHeight);
            QVERIFY(window.grab().save(QDir(capture).filePath("workspace-1024.png")));
        }
    }
};
QTEST_MAIN(WorkspacePresentationTests)
#include "WorkspacePresentationTests.moc"
