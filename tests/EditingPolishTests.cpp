#include "TaskTableTest.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFormLayout>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QTest>
#include <QTimer>
#include <QScopeGuard>

class EditingPolishTests : public QObject {
    Q_OBJECT
private slots:
    void ganttCacheDoesNotRetainDocumentAndUsesCalculationGeneration() {
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("캐시 수명", "2026-01-05", {"작업"});
        doc.setDuration(0, 480);
        auto schedule = jobs::calculate(*doc.project());
        jobs::GanttView gantt;
        gantt.setPlan(&*doc.project(), &schedule, {0}, true, doc.snapshot(), 1);
        QVERIFY(!doc.snapshotShared());
        const auto initialWidth = gantt.sceneRect().width();
        // Same input may receive a new calculation; its result must replace the cache.
        schedule.projectFinish = "2026-06-01 18:00";
        schedule.tasks[0].finish = *schedule.projectFinish;
        gantt.setPlan(&*doc.project(), &schedule, {0}, false, doc.snapshot(), 2);
        QVERIFY(gantt.sceneRect().width() > initialWidth);
        for (auto *item : gantt.scene()->items())
            QVERIFY(item->data(1) != "resizeHandle");
        gantt.setPlan(&*doc.project(), &schedule, {}, false, doc.snapshot(), 2);
        QVERIFY(!gantt.hasSchedule());
        gantt.setPlan(&*doc.project(), &schedule, {0}, false, doc.snapshot(), 2);
        QVERIFY(gantt.hasSchedule());
        doc.create("다른 문서", "2026-01-05", {"새 작업"});
        doc.setDuration(0, 480);
        auto replacement = jobs::calculate(*doc.project());
        gantt.setPlan(&*doc.project(), &replacement, {0}, true, doc.snapshot(), 2);
        QCOMPARE(gantt.sceneRect().width(), initialWidth);
        QVERIFY(!doc.snapshotShared());
    }
    void selectionFitAndFilteredScope() {
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("간트 탐색", "2026-01-05", {"긴 공정", "마지막 작업", "완료점"});
        doc.setDuration(0, 48000);
        doc.setDuration(1, 480);
        doc.setDuration(2, 0);
        doc.setPredecessors(1, {doc.project()->tasks[0].id});
        doc.setPredecessors(2, {doc.project()->tasks[1].id});
        doc.applyCalculation(jobs::calculate(*doc.project()));
        jobs::MainWindow window(doc);
        window.resize(1280, 720);
        window.show();
        QTest::qWait(30);
        auto *table = taskTable(window);
        auto *gantt = static_cast<jobs::GanttView *>(window.findChild<QGraphicsView *>("ganttView"));
        auto *fit = window.findChild<QAction *>("fitGanttSchedule");
        auto *focus = window.findChild<QAction *>("focusGanttSelection");
        const auto revision = doc.project()->revision;
        QVERIFY(fit->isEnabled());
        table->selectRow(2);
        QVERIFY(focus->isEnabled());
        QVERIFY(gantt->horizontalScrollBar()->value() > 0);
        const int manual = std::max(0, gantt->horizontalScrollBar()->value() - 100);
        gantt->horizontalScrollBar()->setValue(manual);
        gantt->setSelectedTask(doc.project()->tasks[2].id);
        QCOMPARE(gantt->horizontalScrollBar()->value(), manual);
        focus->trigger();
        QVERIFY(gantt->horizontalScrollBar()->value() > manual);
        fit->trigger();
        QCOMPARE(gantt->horizontalScrollBar()->value(), 0);
        QVERIFY(gantt->sceneRect().width() <= gantt->viewport()->width());
        window.findChild<QComboBox *>("taskFilter")->setCurrentIndex(2);
        QCOMPARE(table->rowCount(), 1);
        fit->trigger();
        QVERIFY(gantt->sceneRect().width() <= gantt->viewport()->width());
        QCOMPARE(doc.project()->revision, revision);
        window.findChild<QComboBox *>("ganttZoom")->setCurrentIndex(0);
        QVERIFY(gantt->horizontalScrollBar()->maximum() > 0);
        QCOMPARE(table->verticalScrollBar()->value(), gantt->verticalScrollBar()->value());
    }
    void noScheduleDisablesNavigation() {
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        jobs::MainWindow window(doc);
        QVERIFY(!window.findChild<QAction *>("fitGanttSchedule")->isEnabled());
        QVERIFY(!window.findChild<QAction *>("focusGanttSelection")->isEnabled());
    }
    void editorGeometryDraftsAndErrors_data() {
        QTest::addColumn<bool>("dark");
        QTest::newRow("light") << false;
        QTest::newRow("dark") << true;
    }
    void editorGeometryDraftsAndErrors() {
        QFETCH(bool, dark);
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("상세 편집", "2026-11-02", {"작업"});
        doc.setDuration(0, 480);
        doc.putResource({{}, "검증 팀", "labor", 100, {}});
        doc.setStatusAt("2026-11-02 18:00");
        const auto originalPalette = QApplication::palette();
        const auto restorePalette = qScopeGuard([&] { QApplication::setPalette(originalPalette); });
        if (dark) {
            auto palette = originalPalette;
            palette.setColor(QPalette::Window, QColor("#242a33"));
            palette.setColor(QPalette::Base, QColor("#19202a"));
            palette.setColor(QPalette::AlternateBase, QColor("#303947"));
            palette.setColor(QPalette::WindowText, Qt::white);
            palette.setColor(QPalette::Text, Qt::white);
            palette.setColor(QPalette::Button, QColor("#303947"));
            palette.setColor(QPalette::ButtonText, Qt::white);
            QApplication::setPalette(palette);
        } else {
            auto palette = originalPalette;
            palette.setColor(QPalette::Window, QColor("#f4f5f7"));
            palette.setColor(QPalette::Base, Qt::white);
            palette.setColor(QPalette::AlternateBase, QColor("#e7eaf0"));
            palette.setColor(QPalette::WindowText, Qt::black);
            palette.setColor(QPalette::Text, Qt::black);
            palette.setColor(QPalette::Button, QColor("#e7eaf0"));
            palette.setColor(QPalette::ButtonText, Qt::black);
            QApplication::setPalette(palette);
        }
        jobs::MainWindow window(doc);
        window.show();
        QTest::qWait(30);
        auto *table = taskTable(window);
        const auto capture = qEnvironmentVariable("JOBS_EDITOR_CAPTURE_DIR");
        for (const auto *name : {"taskConstraints", "manageCalendars", "manageResources", "taskCost", "resourceRates",
                                 "taskEffort", "reportTaskProgress"}) {
            table->selectRow(0);
            auto *action = window.findChild<QAction *>(name);
            QVERIFY(action && action->isEnabled());
            QString failure;
            QTimer::singleShot(0, [&] {
                auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
                if (!dialog) {
                    failure = "dialog missing";
                    return;
                }
                dialog->setPalette(window.palette());
                dialog->resize(640, 480);
                QTest::qWait(20);
                if (!dialog->findChild<QScrollArea *>("editorScrollArea") || dialog->height() > 520)
                    failure = "small-window editor cannot scroll";
                int height = -1;
                for (auto *line : dialog->findChildren<QLineEdit *>()) {
                    if (!line->testAttribute(Qt::WA_LayoutUsesWidgetRect))
                        continue;
                    if (height >= 0 && line->height() != height)
                        failure = "input height mismatch";
                    height = line->height();
                }
                for (auto *combo : dialog->findChildren<QComboBox *>())
                    if (height >= 0 && combo->height() != height)
                        failure = "combo height mismatch";
                if (auto *field = dialog->findChild<QLineEdit *>(); field && field->isEnabled()) {
                    field->setFocus();
                    QTest::keyClick(field, Qt::Key_Tab);
                    if (!dialog->focusWidget() || !dialog->isAncestorOf(dialog->focusWidget()))
                        failure = "tab navigation left editor";
                }
                for (auto *form : dialog->findChildren<QFormLayout *>())
                    for (int row = 0; row < form->rowCount(); ++row) {
                        auto *a = form->itemAt(row, QFormLayout::LabelRole),
                             *b = form->itemAt(row, QFormLayout::FieldRole);
                        auto *label = a ? qobject_cast<QLabel *>(a->widget()) : nullptr;
                        if (label && b && b->widget() && label->buddy() != b->widget())
                            failure = "missing label buddy";
                    }
                const auto before = doc.project()->revision;
                QLineEdit *invalid = nullptr;
                QLabel *error = nullptr;
                if (dialog->objectName() == "taskConstraintsDialog") {
                    auto *type = dialog->findChild<QComboBox *>("constraintType");
                    type->setCurrentIndex(type->findData("SNET"));
                    invalid = dialog->findChild<QLineEdit *>("constraintAt");
                    invalid->setText("잘못된 날짜");
                    dialog->findChild<QPushButton *>("applyConstraints")->click();
                    error = dialog->findChild<QLabel *>("constraintError");
                } else if (dialog->objectName() == "calendarDialog") {
                    dialog->findChild<QLineEdit *>("calendarName")->setText("검증 달력");
                    invalid = dialog->findChild<QLineEdit *>("calendarPeriods");
                    invalid->setText("잘못된 시간대");
                    dialog->findChild<QPushButton *>("applyCalendar")->click();
                    error = dialog->findChild<QLabel *>("calendarError");
                } else if (dialog->objectName() == "taskCostDialog") {
                    invalid = dialog->findChild<QLineEdit *>("fixedCost");
                    invalid->setText("잘못된 금액");
                    dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
                    error = dialog->findChild<QLabel *>("taskCostError");
                }
                if (invalid && (invalid->text().isEmpty() || !error || error->text().isEmpty() || !invalid->hasFocus()))
                    failure = "invalid draft/focus/inline feedback lost";
                if (error) {
                    auto *viewport = dialog->findChild<QScrollArea *>("editorScrollArea")->viewport();
                    if (!viewport->rect().contains(error->mapTo(viewport, error->rect().center())))
                        failure = "inline feedback outside scroll viewport";
                }
                if (doc.project()->revision != before)
                    failure = "invalid draft modified document";
                if (!capture.isEmpty()) {
                    QDir().mkpath(capture);
                    dialog->grab().save(
                        QDir(capture).filePath((dark ? "dark-" : "light-") + dialog->objectName() + ".png"));
                }
                dialog->reject();
            });
            action->trigger();
            QVERIFY2(failure.isEmpty(), qPrintable(QString::fromLatin1(name) + ": " + failure));
        }
    }
};
QTEST_MAIN(EditingPolishTests)
#include "EditingPolishTests.moc"
