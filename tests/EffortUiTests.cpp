#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "calculation/Effort.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QGraphicsScene>
#include <QGraphicsPathItem>
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
    while (!f() && t.elapsed() < 10000)
        QTest::qWait(10);
    return f();
}
void action(QWidget &w, const char *id) {
    auto *a = w.findChild<QAction *>(id);
    check(a && a->isEnabled(), "action unavailable");
    a->trigger();
}
void calculate(jobs::MainWindow &w, jobs::Document &doc) {
    for (auto *a : w.findChildren<QAction *>())
        if (a->shortcut() == QKeySequence(Qt::Key_F9)) {
            a->trigger();
            break;
        }
    check(wait([&] {
              return doc.calculation() && doc.calculation()->inputRevision == doc.project()->revision &&
                     doc.calculation()->status == jobs::CalculationStatus::Valid;
          }),
          "calculation not valid");
}
QPushButton *okay(QWidget *d) {
    return d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] {
        std::cerr << "effort UI timed out\n";
        std::_Exit(2);
    });
    watchdog.start(45000);
    try {
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("공수·달력·분할 일정", "2026-10-26", {"기본 설계", "설계 검토"});
        doc.setDuration(0, 480);
        doc.setDuration(1, 120);
        const auto id = doc.project()->tasks[0].id;
        doc.setPredecessors(1, {id});
        jobs::Resource resource;
        resource.name = "설계자";
        const auto rid = doc.putResource(resource);
        doc.setAssignments(id, {{id, rid, 100}});
        jobs::MainWindow window(doc);
        window.show();
        auto *table = taskTable(window);
        table->selectRow(0);
        auto revision = doc.project()->revision;
        QTimer::singleShot(30, [&] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "effortDialog", "effort dialog absent");
            d->findChild<QComboBox *>("taskType")->setCurrentIndex(1);
            d->findChild<QComboBox *>("effortBasis")->setCurrentIndex(1);
            d->findChild<QLineEdit *>("effortHours")->setText("8");
            auto *gaps = d->findChild<QPlainTextEdit *>("taskBreaks");
            gaps->setPlainText("2026-10-27 13:00 | 2026-10-26 13:00");
            okay(d)->click();
            callback(wait([&] { return okay(d)->isEnabled(); }) && doc.project()->revision == revision &&
                         !d->findChild<QLabel *>("effortError")->text().isEmpty(),
                     "invalid split committed or error hidden");
            gaps->setPlainText("2026-10-26 13:00 | 2026-10-27 13:00");
            d->grab().save("effort-dialog-smoke.png");
            okay(d)->click();
        });
        action(window, "taskEffort");
        check(wait([&] {
                  return doc.calculation() && doc.calculation()->inputRevision == doc.project()->revision &&
                         doc.calculation()->status == jobs::CalculationStatus::Valid;
              }),
              "effort apply did not recalculate");
        check(doc.project()->revision == revision + 1 && doc.calculation()->tasks[0].finish == "2026-10-27 18:00" &&
                  doc.calculation()->tasks[1].start == "2026-10-28 09:00",
              "split/FS propagation wrong");
        check(cellText(table, 0, 12).contains("고정 공수") && cellText(table, 0, 13) == "1", "effort table missing");
        auto *view = dynamic_cast<jobs::GanttView *>(window.findChild<QGraphicsView *>("ganttView"));
        check(view, "gantt missing");
        bool splitPath = false;
        for (auto *item : view->scene()->items())
            if (item->data(0).toString().toStdString() == id && item->data(1) == "task") {
                auto *path = dynamic_cast<QGraphicsPathItem *>(item);
                check(path, "split gantt not segmented");
                splitPath = path->path().contains(QPointF(64.0 * 10 / 24, 58)) &&
                            !path->path().contains(QPointF(64.0 * 15 / 24, 58));
            }
        check(splitPath, "gantt filled pause with work bar");
        window.grab().save("effort-gantt-smoke.png");
        action(window, "undoEdit");
        check(doc.project()->tasks[0].taskType == jobs::TaskType::FixedDuration &&
                  doc.project()->tasks[0].breaks.empty(),
              "effort not one undo");
        action(window, "redoEdit");
        calculate(window, doc);
        const auto calendar =
            doc.putCalendar({{}, "화요일 휴무", {1, 2, 3, 4, 5}, {{540, 720}, {780, 1080}}, {"2026-10-27"}});
        QTimer::singleShot(30, [&] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "resourceDialog", "resource dialog absent");
            d->findChild<QTableWidget *>("resourceList")->selectRow(0);
            auto *c = d->findChild<QComboBox *>("resourceCalendar");
            callback(c, "resource calendar control absent");
            c->setCurrentIndex(c->findData(QString::fromStdString(calendar)));
            d->findChild<QPushButton *>("saveResource")->click();
            callback(doc.project()->resources[0].calendarId == calendar, "calendar not persisted");
            d->grab().save("effort-resource-calendar-smoke.png");
            QMetaObject::invokeMethod(d, "reject");
        });
        action(window, "manageResources");
        calculate(window, doc);
        check(doc.calculation()->tasks[0].finish == "2026-10-28 15:00", "resource calendar did not postpone work");
        doc.setStatusAt("2026-10-26 18:00");
        calculate(window, doc);
        table->selectRow(0);
        QTimer::singleShot(30, [&] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "progressDialog", "progress dialog absent");
            callback(!d->findChild<QSpinBox *>("remainingMinutes")->isEnabled(),
                     "work report requires invented duration");
            d->findChild<QLineEdit *>("actualStart")->setText("2026-10-26 09:00");
            d->findChild<QSpinBox *>("actualMinutes")->setValue(180);
            d->findChild<QLineEdit *>("remainingWorkHours")->setText("5");
            d->findChild<QLineEdit *>("progressReporter")->setText("PM");
            d->findChild<QLineEdit *>("progressReason")->setText("공수 확인");
            d->grab().save("effort-progress-smoke.png");
            d->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Save)->click();
        });
        bool progressAction = false;
        for (auto *a : window.findChildren<QAction *>())
            if (a->text() == "선택 작업 실적") {
                check(a->isEnabled(), "progress action disabled");
                a->trigger();
                progressAction = true;
                break;
            }
        check(progressAction && doc.project()->progress.back().remainingWorkUnitMinutes == 30000 &&
                  doc.project()->progress.back().remainingMinutes == 0,
              "work report not saved");
        calculate(window, doc);
        check(doc.calculation()->tasks[0].start == "2026-10-26 09:00" &&
                  doc.calculation()->tasks[0].finish == "2026-10-28 15:00",
              "explicit work forecast wrong");
        // A read-only document exposes definitions without an enabled apply button.
        auto file = dir.filePath("effort.jobs");
        doc.saveAs(file.toStdString());
        jobs::Document readonly(store);
        readonly.open(file.toStdString(), true);
        jobs::MainWindow reader(readonly);
        reader.show();
        taskTable(reader)->selectRow(0);
        const auto before = *readonly.project();
        QTimer::singleShot(30, [] {
            auto *d = QApplication::activeModalWidget();
            callback(d && d->objectName() == "effortDialog" && !okay(d)->isEnabled(), "read-only effort apply enabled");
            QMetaObject::invokeMethod(d, "reject");
        });
        action(reader, "taskEffort");
        check(*readonly.project() == before, "read-only inspection changed data");
        reader.hide();
        window.hide();
        std::cout << "PASS: effort/type/split UI, validation with no partial edit, automatic FS propagation, split "
                     "Gantt paths, single-step undo/redo, resource calendar edit and read-only inspection\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
