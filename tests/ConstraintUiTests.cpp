#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QTimer>
#include <QElapsedTimer>
#include <QTest>
#include <QTemporaryDir>
#include <iostream>
namespace {
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
void callback(bool b, const char *m) {
    if (!b) {
        std::cerr << "FAIL: " << m << '\n';
        std::_Exit(2);
    }
}
bool wait(const std::function<bool()> &f) {
    QElapsedTimer t;
    t.start();
    while (!f() && t.elapsed() < 7000)
        QTest::qWait(10);
    return f();
}
void action(QWidget &w, const char *id) {
    auto *a = w.findChild<QAction *>(id);
    check(a && a->isEnabled(), "action unavailable");
    a->trigger();
}
jobs::GanttView *gantt(QWidget &w) {
    auto *g = dynamic_cast<jobs::GanttView *>(w.findChild<QGraphicsView *>("ganttView"));
    check(g, "gantt missing");
    return g;
}
QGraphicsItem *bar(jobs::GanttView *view, const std::string &id) {
    for (auto *item : view->scene()->items())
        if ((item->data(1) == "task" || item->data(1) == "milestone") && item->data(0).toString().toStdString() == id)
            return item;
    throw std::runtime_error("bar missing");
}
void calculate(jobs::MainWindow &w, jobs::Document &doc) {
    for (auto *a : w.findChildren<QAction *>())
        if (a->shortcut() == QKeySequence(Qt::Key_F9))
            a->trigger();
    check(wait([&] {
              return doc.calculation() && doc.calculation()->status != jobs::CalculationStatus::Stale &&
                     doc.calculation()->inputRevision == doc.project()->revision;
          }),
          "calculation timeout");
}
void mouse(jobs::GanttView *view, QEvent::Type type, const QPoint &point, Qt::MouseButton button,
           Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers = {}) {
    QMouseEvent event(type, QPointF(point), QPointF(view->viewport()->mapToGlobal(point)), button, buttons, modifiers);
    QApplication::sendEvent(view->viewport(), &event);
}
void drag(jobs::GanttView *view, const QPoint &from, const QPoint &to, bool escape = false,
          Qt::KeyboardModifiers modifiers = {}) {
    mouse(view, QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton, modifiers);
    mouse(view, QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton, modifiers);
    if (escape)
        QTest::keyClick(view, Qt::Key_Escape);
    mouse(view, QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton, modifiers);
}
QPoint center(jobs::GanttView *view, const std::string &id) {
    auto *b = bar(view, id);
    return view->mapFromScene(b->sceneBoundingRect().center());
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] {
        std::cerr << "constraint UI timeout\n";
        std::_Exit(2);
    });
    watchdog.start(45000);
    try {
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("간트 날짜 편집", "2026-10-26", {"작업 A", "작업 B"});
        doc.setDuration(0, 480);
        doc.setDuration(1, 240);
        auto id = doc.project()->tasks[0].id;
        doc.setPredecessors(1, {id});
        jobs::MainWindow window(doc);
        window.show();
        auto *table = taskTable(window);
        table->selectRow(0);
        QTimer::singleShot(30, [&] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "taskConstraintsDialog", "timing dialog missing");
            d->findChild<QComboBox *>("constraintType")->setCurrentIndex(2);
            d->findChild<QLineEdit *>("constraintAt")->setText("2026-10-27 09:00");
            d->findChild<QLineEdit *>("deadlineAt")->setText("2026-10-27 17:00");
            d->grab().save("constraints-dialog-smoke.png");
            // Start an old snapshot without processing its queued completion, then apply new timing.
            for (auto *a : window.findChildren<QAction *>())
                if (a->shortcut() == QKeySequence(Qt::Key_F9)) {
                    a->trigger();
                    break;
                }
            d->findChild<QPushButton *>("applyConstraints")->click();
        });
        action(window, "taskConstraints");
        check(wait([&] { return doc.calculation() && doc.calculation()->status == jobs::CalculationStatus::Valid; }),
              "constraint auto calculation failed");
        check(cellText(table, 0, 10).contains("SNET") && cellText(table, 0, 11) == "2026-10-27 17:00",
              "table timing fields missing");
        auto *view = gantt(window);
        bool deadline = false;
        for (auto *item : view->scene()->items())
            deadline = deadline || item->data(1) == "deadline";
        check(deadline, "deadline marker missing");
        // Real viewport mouse events exercise preview, commit, automatic recalculation and one-step undo.
        auto pos = center(view, id);
        auto revision = doc.project()->revision;
        drag(view, pos, pos + QPoint(64, 0));
        check(wait([&] {
                  return doc.project()->revision == revision + 1 && doc.calculation() &&
                         doc.calculation()->status == jobs::CalculationStatus::Valid;
              }),
              "move did not commit/recalculate");
        check(doc.project()->tasks[0].constraintAt == "2026-10-28 09:00" &&
                  doc.project()->tasks[0].durationMinutes == 480,
              "move changed wrong inputs");
        check(doc.calculation()->tasks[1].start == "2026-10-29 09:00", "successor not propagated");
        action(window, "undoEdit");
        check(doc.project()->tasks[0].constraintAt == "2026-10-27 09:00", "move not one undo step");
        action(window, "redoEdit");
        calculate(window, doc);
        pos = center(view, id);
        revision = doc.project()->revision;
        drag(view, pos, pos + QPoint(64, 0), true);
        check(doc.project()->revision == revision, "escape committed drag");
        // Resize by a whole civil day: the working calendar supplies the extra 480 minutes.
        auto *shape = bar(view, id);
        const auto end = view->mapFromScene(
            QPointF(shape->sceneBoundingRect().right() - 1, shape->sceneBoundingRect().center().y()));
        revision = doc.project()->revision;
        drag(view, end, end + QPoint(64, 0), false, Qt::ShiftModifier);
        check(wait([&] {
                  return doc.project()->revision == revision + 1 && doc.calculation() &&
                         doc.calculation()->status == jobs::CalculationStatus::Valid;
              }),
              "resize failed");
        check(doc.project()->tasks[0].durationMinutes == 960 &&
                  doc.project()->tasks[0].constraintAt == "2026-10-28 09:00",
              "resize lost duration or constraint");
        // A document mutation between press and release retires the preview with no second edit.
        pos = center(view, id);
        mouse(view, QEvent::MouseButtonPress, pos, Qt::LeftButton, Qt::LeftButton);
        mouse(view, QEvent::MouseMove, pos + QPoint(64, 0), Qt::NoButton, Qt::LeftButton);
        doc.renameTask(1, "외부 편집");
        revision = doc.project()->revision;
        QTimer::singleShot(20, [] {
            for (auto *w : QApplication::topLevelWidgets())
                if (w->inherits("QMessageBox"))
                    QMetaObject::invokeMethod(w, "accept");
        });
        mouse(view, QEvent::MouseButtonRelease, pos + QPoint(64, 0), Qt::LeftButton, Qt::NoButton);
        check(doc.project()->revision == revision, "stale drag changed document");
        calculate(window, doc);
        // The soft deadline remains editable and is reported as a warning with task navigation.
        QTimer poll;
        poll.setInterval(10);
        bool selected = false;
        QObject::connect(&poll, &QTimer::timeout, [&] {
            auto *d = QApplication::activeModalWidget();
            if (!d || d->objectName() != "scheduleIssuesDialog")
                return;
            auto *t = d->findChild<QTableWidget *>("scheduleIssuesTable");
            for (int r = 0; r < t->rowCount(); ++r)
                if (t->item(r, 1)->text() == "DEADLINE_MISSED") {
                    t->setCurrentCell(r, 1);
                    d->grab().save("constraints-diagnostics-smoke.png");
                    selected = true;
                    poll.stop();
                    d->findChild<QPushButton *>("selectDiagnosticTask")->click();
                    return;
                }
        });
        poll.start();
        action(window, "scheduleIssues");
        check(selected && table->currentRow() == 0, "deadline diagnostic navigation failed");
        window.grab().save("constraints-gantt-smoke.png");
        // Week zoom still commits through the same stable-ID/revision boundary.
        view->setDayWidth(16);
        pos = center(view, id);
        revision = doc.project()->revision;
        drag(view, pos, pos + QPoint(16, 0));
        check(wait([&] {
                  return doc.project()->revision == revision + 1 && doc.calculation() &&
                         doc.calculation()->status == jobs::CalculationStatus::Valid;
              }),
              "week zoom move failed");
        doc.setStatusAt("2026-10-26 18:00");
        calculate(window, doc);
        pos = center(view, id);
        revision = doc.project()->revision;
        drag(view, pos, pos + QPoint(32, 0));
        check(doc.project()->revision == revision, "forecast drag changed input");
        doc.setStatusAt({});
        calculate(window, doc);
        doc.setTaskTiming(1, "SNLT", "2026-10-26 17:00", {});
        calculate(window, doc);
        check(doc.calculation()->status == jobs::CalculationStatus::Conflict, "hard conflict hidden");
        QTimer hardPoll;
        hardPoll.setInterval(10);
        bool hardSelected = false;
        QObject::connect(&hardPoll, &QTimer::timeout, [&] {
            auto *d = QApplication::activeModalWidget();
            if (!d || d->objectName() != "scheduleIssuesDialog")
                return;
            auto *t = d->findChild<QTableWidget *>("scheduleIssuesTable");
            for (int r = 0; r < t->rowCount(); ++r)
                if (t->item(r, 1)->text() == "CONSTRAINT_CONFLICT") {
                    t->setCurrentCell(r, 1);
                    callback(t->item(r, 0)->text() == "충돌/입력" &&
                                 d->findChild<QComboBox *>("diagnosticTask")->count() >= 2,
                             "hard conflict lost causal tasks");
                    d->grab().save("constraints-conflict-smoke.png");
                    hardSelected = true;
                    hardPoll.stop();
                    d->findChild<QPushButton *>("selectDiagnosticTask")->click();
                    return;
                }
        });
        hardPoll.start();
        action(window, "scheduleIssues");
        check(hardSelected && table->currentRow() == 1, "hard conflict navigation failed");
        QTimer::singleShot(30, [] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "taskConstraintsDialog", "repair dialog missing");
            d->findChild<QComboBox *>("constraintType")->setCurrentIndex(0);
            d->findChild<QPushButton *>("applyConstraints")->click();
        });
        action(window, "taskConstraints");
        check(wait([&] { return doc.calculation() && doc.calculation()->status == jobs::CalculationStatus::Valid; }),
              "repair did not restore schedule");
        auto path = dir.filePath("readonly.jobs");
        doc.saveAs(path.toStdString());
        jobs::Document readonly(store);
        readonly.open(path.toStdString(), true);
        readonly.applyCalculation(jobs::calculate(*readonly.project()));
        jobs::MainWindow readWindow(readonly);
        readWindow.show();
        auto *readTable = taskTable(readWindow);
        readTable->selectRow(0);
        QTimer::singleShot(30, [] {
            auto *d = QApplication::activeModalWidget();
            callback(d && !d->findChild<QPushButton *>("applyConstraints")->isEnabled(), "readonly timing editable");
            qobject_cast<QDialog *>(d)->reject();
        });
        action(readWindow, "taskConstraints");
        auto *readView = gantt(readWindow);
        pos = center(readView, id);
        const auto readRevision = readonly.project()->revision;
        drag(readView, pos, pos + QPoint(64, 0));
        check(readonly.project()->revision == readRevision, "readonly drag changed input");
        std::cout << "PASS: timing UI, causal deadline navigation, real mouse move/resize/undo/Esc/stale guards, "
                     "successor propagation, week zoom, forecast/readonly drag exclusion\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
