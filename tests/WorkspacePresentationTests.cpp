#include "TaskTableTest.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "ui/ApplicationIcon.h"
#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QTest>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QPushButton>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPaintEvent>
#include <QScrollBar>

namespace {
class PaintCoverage : public QObject {
public:
    QRegion painted;
    bool eventFilter(QObject *, QEvent *event) override {
        if (event->type() == QEvent::Paint)
            painted += static_cast<QPaintEvent *>(event)->region();
        return false;
    }
};
} // namespace

class WorkspacePresentationTests : public QObject {
    Q_OBJECT
private slots:
    void frozenGanttHeaderIsRepaintedAfterMixedScroll() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        document.create("고정 헤더", "2026-11-02", std::vector<std::string>(240, "작업"));
        for (std::size_t row = 0; row < 240; ++row)
            document.setDuration(row, 480);
        document.setDuration(0, 48000);
        document.applyCalculation(jobs::calculate(*document.project()));
        jobs::MainWindow window(document);
        window.resize(1024, 650);
        window.show();
        QTest::qWait(30);
        auto *gantt = window.findChild<QGraphicsView *>("ganttView");
        auto *table = taskTable(window);
        table->verticalScrollBar()->setValue(2000);
        QTest::qWait(30);
        PaintCoverage coverage;
        gantt->viewport()->installEventFilter(&coverage);
        table->verticalScrollBar()->setValue(2028);
        QTest::qWait(30);
        QVERIFY(coverage.painted.contains(QRect(0, 0, gantt->viewport()->width(), jobs::GanttView::HeaderHeight)));
        coverage.painted = {};
        QVERIFY(gantt->horizontalScrollBar()->maximum() > 100);
        gantt->horizontalScrollBar()->setValue(100);
        QTest::qWait(30);
        // A scene bit-blit can otherwise leave overscan task labels in the frozen date header.
        QVERIFY(coverage.painted.contains(QRect(0, 0, gantt->viewport()->width(), jobs::GanttView::HeaderHeight)));
        QCOMPARE(table->verticalScrollBar()->value(), gantt->verticalScrollBar()->value());
        auto colors = gantt->palette();
        colors.setColor(QPalette::AlternateBase, QColor(255, 255, 255, 12));
        gantt->setPalette(colors);
        const QRect header(0, 0, gantt->viewport()->width(), jobs::GanttView::HeaderHeight);
        const auto before = gantt->viewport()->grab(header).toImage();
        auto *underlay = gantt->scene()->addRect(gantt->mapToScene(header).boundingRect(), Qt::NoPen, Qt::magenta);
        underlay->setZValue(10000);
        const auto after = gantt->viewport()->grab(header).toImage();
        QCOMPARE(after, before);
    }
    void componentHeightsAndApplicationIcon() {
        jobs::JsonProjectStore store;
        jobs::Document document(store);
        document.create("높이·아이콘 확인", "2026-11-02", {"작업 1", "작업 2"});
        auto scope = document.addWbs("하위 범위");
        document.assignWbs(0, scope);
        document.setDuration(0, 480);
        document.setDuration(1, 960);
        document.applyCalculation(jobs::calculate(*document.project()));
        jobs::MainWindow window(document);
        window.show();
        QTest::qWait(30);
        QVERIFY(!window.windowIcon().isNull());
        for (int size : {16, 24, 32, 48, 128, 256})
            QVERIFY(!jobs::applicationIcon().pixmap(size, size).isNull());
        auto *tree = window.findChild<QTreeWidget *>("wbsTree");
        auto *table = taskTable(window);
        auto *gantt = window.findChild<QGraphicsView *>("ganttView");
        QJsonArray measurements;
        const auto folder = qEnvironmentVariable("JOBS_LAYOUT_OUTPUT_DIR");
        for (const auto &size : {QSize(1024, 650), QSize(1280, 720), QSize(1440, 900)}) {
            window.resize(size);
            QTest::qWait(20);
            auto *search = window.findChild<QLineEdit *>("taskSearch");
            const int expected = search->height();
            QJsonObject positions;
            for (const auto *id : {"taskSearch", "taskFilter", "taskSort", "resetTaskView"}) {
                auto *widget = window.findChild<QWidget *>(id);
                positions[id] = QJsonObject{{"y", widget->mapTo(&window, QPoint()).y()}, {"height", widget->height()}};
            }
            qInfo().noquote() << QJsonDocument(positions).toJson(QJsonDocument::Compact);
            measurements.append(QJsonObject{{"width", window.width()},
                                            {"height", window.height()},
                                            {"control_height", expected},
                                            {"pane_header_height", tree->header()->height()},
                                            {"row_height", tree->visualItemRect(tree->topLevelItem(0)).height()},
                                            {"table_viewport_height", table->viewport()->height()},
                                            {"dpr", window.devicePixelRatioF()},
                                            {"controls", positions}});
            if (!folder.isEmpty()) {
                QVERIFY(QDir().mkpath(folder));
                QFile metrics(QDir(folder).filePath("geometry.json"));
                QVERIFY(metrics.open(QIODevice::WriteOnly));
                QVERIFY(metrics.write(QJsonDocument(measurements).toJson()) > 0);
                QVERIFY(window.grab().save(
                    QDir(folder).filePath(QString("layout-%1-%2.png").arg(size.width()).arg(size.height()))));
            }
            for (const auto *name : {"taskFilter", "taskSort", "resetTaskView", "toggleWbs", "ganttZoom",
                                     "baselineChoice", "toggleDiagnostics"})
                QCOMPARE(window.findChild<QWidget *>(name)->height(), expected);
            for (const auto *name : {"taskFilter", "taskSort", "resetTaskView"})
                QCOMPARE(window.findChild<QWidget *>(name)->mapTo(&window, QPoint()).y(),
                         search->mapTo(&window, QPoint()).y());
            for (const auto *name : {"selectedTaskMenu", "analysisMenu"})
                QCOMPARE(window.findChild<QToolButton *>(name)->height(), expected);
            QCOMPARE(tree->header()->height(), jobs::GanttView::HeaderHeight);
            QCOMPARE(tree->visualItemRect(tree->topLevelItem(0)).height(), jobs::GanttView::RowHeight);
            QCOMPARE(tree->viewport()->mapTo(&window, QPoint()).y(), table->viewport()->mapTo(&window, QPoint()).y());
            QCOMPARE(tree->viewport()->height(), table->viewport()->height());
            QCOMPARE(gantt->viewport()->height() - jobs::GanttView::HeaderHeight, table->viewport()->height());
            QVERIFY(table->viewport()->height() >= jobs::GanttView::RowHeight * 4);
        }
        if (!folder.isEmpty()) {
            QFile output(QDir(folder).filePath("geometry.json"));
            QVERIFY(output.open(QIODevice::WriteOnly));
            QVERIFY(output.write(QJsonDocument(measurements).toJson()) > 0);
        }
    }
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
        const auto capture = qEnvironmentVariable("JOBS_UI_SCREENSHOT_DIR");
        const QJsonObject geometry{{"window_height", window.height()},
                                   {"table_viewport_height", table->viewport()->height()},
                                   {"gantt_viewport_height", gantt->viewport()->height()},
                                   {"header_height", table->horizontalHeader()->height()}};
        qInfo().noquote() << QJsonDocument(geometry).toJson(QJsonDocument::Compact);
        table->selectRow(3);
        if (!capture.isEmpty()) {
            QVERIFY(QDir().mkpath(capture));
            QVERIFY(window.grab().save(QDir(capture).filePath("workspace-1440.png")));
            QFile output(QDir(capture).filePath("geometry.json"));
            QVERIFY(output.open(QIODevice::WriteOnly));
            QVERIFY(output.write(QJsonDocument(geometry).toJson()) > 0);
        }
        QCOMPARE(table->rowCount(), 8);
        QCOMPARE(table->horizontalHeader()->height(), jobs::GanttView::HeaderHeight);
        QCOMPARE(table->verticalHeader()->sectionSize(0), jobs::GanttView::RowHeight);
        QVERIFY(table->viewport()->height() >= 4 * jobs::GanttView::RowHeight);
        QCOMPARE(table->viewport()->height() + jobs::GanttView::HeaderHeight, gantt->viewport()->height());
        QCOMPARE(table->horizontalHeader()->mapTo(&window, QPoint()).y(),
                 gantt->viewport()->mapTo(&window, QPoint()).y());
        QVERIFY(!window.findChild<QPlainTextEdit *>("calculationDiagnostics")->isVisible());
        QVERIFY(!gantt->scene()->items().isEmpty());
        if (!capture.isEmpty()) {
            window.resize(1024, 650);
            QTest::qWait(30);
            QVERIFY(table->viewport()->height() >= 4 * jobs::GanttView::RowHeight);
            QVERIFY(window.grab().save(QDir(capture).filePath("workspace-1024.png")));
        }
    }
};
QTEST_MAIN(WorkspacePresentationTests)
#include "WorkspacePresentationTests.moc"
