#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QInputDialog>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QMessageBox>
#include <QGraphicsItem>
#include <QGraphicsScene>
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
        std::cerr << m << '\n';
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
    check(a && a->isEnabled(), "missing/disabled action");
    a->trigger();
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] {
        std::cerr << "PDM UI timeout\n";
        std::_Exit(2);
    });
    watchdog.start(30000);
    try {
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("교대 일정", "2026-10-26", {"설계", "시공"});
        doc.setDuration(0, 480);
        doc.setDuration(1, 240);
        jobs::MainWindow window(doc);
        window.show();
        auto *table = taskTable(window);
        QTimer::singleShot(30, [&] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "calendarDialog", "calendar manager missing");
            d->findChild<QLineEdit *>("calendarName")->setText("오후 현장팀");
            d->findChild<QLineEdit *>("calendarWeekdays")->setText("1,2,3,4,5,6");
            d->findChild<QLineEdit *>("calendarPeriods")->setText("13:00-17:00");
            d->findChild<QPlainTextEdit *>("calendarHolidays")->setPlainText("2026-10-27");
            d->findChild<QPushButton *>("applyCalendar")->click();
            callback(doc.project()->calendars.size() == 1, "calendar not registered");
            d->grab().save("pdm-calendar-smoke.png");
            qobject_cast<QDialog *>(d)->reject();
        });
        action(window, "manageCalendars");
        table->selectRow(1);
        QTimer::singleShot(30, [&] {
            auto *d = qobject_cast<QInputDialog *>(QApplication::activeModalWidget());
            callback(d, "assignment chooser missing");
            d->findChild<QComboBox *>()->setCurrentIndex(1);
            d->accept();
        });
        action(window, "assignTaskCalendar");
        check(!doc.project()->tasks[1].calendarId.empty() && cellText(table, 1, 9) == "오후 현장팀",
              "task calendar assignment lost");
        setCellText(table, 1, 2, "1SS+1.5h");
        check(doc.project()->tasks[1].dependencyRules[0].lagMinutes == 90 && cellText(table, 1, 2) == "1SS+90m",
              "typed relation parse/format wrong");
        auto before = *doc.project();
        QTimer::singleShot(30, [] {
            auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            callback(box, "invalid input not explained");
            box->accept();
        });
        setCellText(table, 1, 2, "1SS+0.01m");
        check(*doc.project() == before, "invalid relation committed");
        action(window, "undoEdit");
        check(doc.project()->tasks[1].predecessors.empty(), "relationship undo failed");
        action(window, "redoEdit");
        check(doc.project()->tasks[1].dependencyRules[0].type == "SS", "relationship redo failed");
        for (auto *a : window.findChildren<QAction *>()) {
            if (a->shortcut() == QKeySequence(Qt::Key_F9))
                a->trigger();
        }
        check(wait([&] { return doc.calculation() && doc.calculation()->status == jobs::CalculationStatus::Valid; }),
              "PDM UI calculation failed");
        check(cellText(table, 1, 3) == "2026-10-26 14:30" && cellText(table, 1, 4) == "2026-10-28 14:30",
              "calendar UI dates wrong");
        auto *gantt = dynamic_cast<jobs::GanttView *>(window.findChild<QGraphicsView *>("ganttView"));
        check(gantt, "Gantt view missing");
        bool edge = false, calendar = false;
        for (auto *item : gantt->scene()->items()) {
            if (item->data(1) == "dependency")
                edge = item->toolTip().contains("SS+90m");
            if (item->data(1) == "task" && item->toolTip().contains("오후 현장팀"))
                calendar = item->toolTip().contains("2026-10-27");
        }
        check(edge && calendar, "Gantt relation/calendar provenance missing");
        window.grab().save("pdm-main-smoke.png");
        jobs::Document readonly(store);
        auto path = dir.filePath("pdm-ui.jobs");
        doc.saveAs(path.toStdString());
        readonly.open(path.toStdString(), true);
        {
            jobs::MainWindow view(readonly);
            view.show();
            auto *t = taskTable(view);
            t->selectRow(1);
            check(!view.findChild<QAction *>("assignTaskCalendar")->isEnabled(), "readonly allowed assignment");
            QTimer::singleShot(30, [] {
                auto *d = QApplication::activeModalWidget();
                callback(d && !d->findChild<QPushButton *>("applyCalendar")->isEnabled(),
                         "readonly allowed calendar edits");
                qobject_cast<QDialog *>(d)->reject();
            });
            action(view, "manageCalendars");
        }
        std::cout << "PASS: calendar registration/assignment/readonly, typed PDM units and invalid input, undo/redo, "
                     "F9 dates, Gantt endpoint metadata\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
