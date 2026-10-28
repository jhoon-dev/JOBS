#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/AsyncEditController.h"
#include <QStatusBar>
#include "ui/GanttView.h"
#include "ui/TaskInput.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <functional>
#include <algorithm>
#include <iostream>
namespace {
struct ClipboardRestore {
    QMimeData *original = new QMimeData;
    ClipboardRestore() {
        if (const auto *data = QApplication::clipboard()->mimeData())
            for (const auto &format : data->formats())
                original->setData(format, data->data(format));
    }
    ~ClipboardRestore() {
        QApplication::clipboard()->setMimeData(original);
    }
};
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
    QElapsedTimer timer;
    timer.start();
    while (!f() && timer.elapsed() < 10000)
        QTest::qWait(10);
    return f();
}
void settle(QWidget &w) {
    for (auto *child : w.children())
        if (auto *c = dynamic_cast<jobs::AsyncEditController *>(child))
            check(wait([&] { return !c->busy(); }), "async edit timeout");
}
void action(QWidget &w, const char *id) {
    auto *a = w.findChild<QAction *>(id);
    check(a && a->isEnabled(), "editing action missing/disabled");
    a->trigger();
    settle(w);
}
void rejects(const std::function<void()> &f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "expected TSV/input rejection");
}
void dismissError(const std::function<void()> &f) {
    f();
    auto *w = QApplication::activeWindow();
    check(w, "missing window");
    settle(*w);
    check(!w->findChild<QStatusBar *>()->currentMessage().isEmpty(), "missing async error");
}
void calculate(jobs::MainWindow &w, jobs::Document &d) {
    for (auto *a : w.findChildren<QAction *>())
        if (a->shortcut() == QKeySequence(Qt::Key_F9)) {
            a->trigger();
            break;
        }
    check(wait([&] {
              return d.calculation() && d.calculation()->status == jobs::CalculationStatus::Valid &&
                     d.calculation()->inputRevision == d.project()->revision;
          }),
          "calculation failed");
}
QString id(jobs::TaskTableView *t, int row) {
    return cellText(t, row, 8);
}
void focus(jobs::MainWindow &w, jobs::TaskTableView *t) {
    w.activateWindow();
    QTest::qWait(20);
    t->setFocus();
    QTest::qWait(1);
}
void select(jobs::TaskTableView *t, int first, int last, int column = 0) {
    t->clearSelection();
    t->setCurrentCell(first, column);
    selectCellRange(t, first, column, last, column, true);
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] {
        std::cerr << "FAIL: editing UI timeout\n";
        std::_Exit(2);
    });
    watchdog.start(90000);
    try {
        ClipboardRestore clipboardRestore;
        const QList<QStringList> fields{{"A\tB", "quote \"name\"", "line\nnext"}, {"", "0", "1SS+2h"}};
        check(jobs::decodeTaskTsv(jobs::encodeTaskTsv(fields)) == fields, "quoted TSV roundtrip");
        check(jobs::decodeTaskTsv("A\t1\r\nB\t2\r\n").size() == 2, "CRLF terminal row");
        check(jobs::decodeTaskTsv("\t") == QList<QStringList>{{"", ""}}, "empty cells lost");
        rejects([] { jobs::decodeTaskTsv("A\t1\nB"); });
        rejects([] { jobs::decodeTaskTsv("\"unterminated"); });
        rejects([] { jobs::decodeTaskTsv("\"closed\"junk"); });
        rejects([] { jobs::decodeTaskTsv(QString(4 * 1024 * 1024 + 1, 'x')); });
        rejects([] { jobs::decodeTaskTsv(QString("a\n").repeated(10001)); });
        rejects([] { jobs::decodeTaskTsv(QString("a\t").repeated(30000)); });
        rejects([] { jobs::taskDurationInput("0.001"); });
        rejects([] { jobs::taskDurationInput("-1"); });
        check(jobs::taskDurationInput("1m") == 1 && !jobs::taskDurationInput(""), "duration grammar changed");
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("Task editing", "2026-10-26", {"Zulu", "Alpha", "Bravo"});
        doc.setDuration(0, 60);
        doc.setDuration(1, 120);
        doc.setDuration(2, 0);
        const auto a = doc.project()->tasks[0].id, b = doc.project()->tasks[1].id, c = doc.project()->tasks[2].id;
        doc.setPredecessors(1, {a});
        doc.saveAs(dir.filePath("tasks.jobs").toStdString());
        jobs::MainWindow window(doc);
        window.show();
        QTest::qWait(30);
        auto *table = taskTable(window);
        auto *sort = window.findChild<QComboBox *>("taskSort");
        auto *filter = window.findChild<QComboBox *>("taskFilter");
        auto *search = window.findChild<QLineEdit *>("taskSearch");
        auto *view = dynamic_cast<jobs::GanttView *>(window.findChild<QGraphicsView *>("ganttView"));
        check(table && sort && filter && search && view, "editing widgets missing");
        calculate(window, doc);
        if (app.arguments().contains("--capture-only")) {
            window.resize(1024, 650);
            app.processEvents();
            check(table->viewport()->height() >= 4 * jobs::GanttView::RowHeight,
                  "native compact window hid working rows");
            sort->setCurrentIndex(1);
            table->selectRow(0);
            app.processEvents();
            check(window.grab().save("task-editing-native-smoke.png"), "native screen capture failed");
            window.hide();
            std::cout << "PASS: native sorted task editing screen captured\n";
            return 0;
        }
        window.resize(1024, 650);
        app.processEvents();
        check(table->viewport()->height() >= 4 * jobs::GanttView::RowHeight &&
                  !window.findChild<QLabel *>("taskInputHelp")->isVisible(),
              "compact window hid the working rows");
        QTimer::singleShot(20, [] {
            auto *d = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            callback(d && d->text().contains("F9"), "compact help unavailable");
            d->accept();
        });
        action(window, "taskEditingHelp");
        window.resize(1560, 900);
        app.processEvents();
        const auto revision = doc.project()->revision;
        const auto original = *doc.project();
        table->selectRow(0);
        sort->setCurrentIndex(1);
        check(id(table, 0) == QString::fromStdString(b) && id(table, 2) == QString::fromStdString(a) &&
                  table->currentRow() == 2 && !doc.dirty() && doc.project()->revision == revision,
              "sort changed inputs or lost stable selection");
        check(cellText(table, 0, 2) == "1" && table->model()->headerData(0, Qt::Vertical).toString() == "2",
              "view order replaced canonical dependency numbers");
        bool aligned = false;
        for (auto *item : view->scene()->items())
            if (item->data(0) == QString::fromStdString(b) && item->data(1) == "task")
                aligned =
                    item->sceneBoundingRect().center().y() < jobs::GanttView::HeaderHeight + jobs::GanttView::RowHeight;
        check(aligned, "gantt did not follow sorted table order");
        sort->setCurrentIndex(5);
        check(id(table, 0) == QString::fromStdString(a) && id(table, 1) == QString::fromStdString(c),
              "start sort lost canonical tie order");
        sort->setCurrentIndex(1);
        select(table, 0, 1, 0);
        QApplication::clipboard()->setText("Alpha edited\t3\t1\r\nBravo edited\t0\t2SS+30m\r\n");
        focus(window, table);
        QTest::keySequence(table, QKeySequence::Paste);
        settle(window);
        check(doc.project()->revision == revision + 1 && doc.project()->tasks[0].name == "Zulu" &&
                  doc.project()->tasks[1].name == "Alpha edited" && doc.project()->tasks[1].durationMinutes == 180 &&
                  doc.project()->tasks[2].predecessors == std::vector<std::string>{b},
              "sorted paste changed wrong IDs/fields or shortcut failed");
        action(window, "undoEdit");
        check(static_cast<const jobs::PlanInput &>(*doc.project()) == static_cast<const jobs::PlanInput &>(original),
              "paste was not one undo");
        action(window, "redoEdit");
        check(doc.project()->tasks[2].dependencyRules[0].type == jobs::DependencyType::SS,
              "paste redo lost relationship");
        auto before = *doc.project();
        select(table, 0, 1, 0);
        QApplication::clipboard()->setText("Valid\t1\t1\nBad\t-1\t");
        dismissError([&] { action(window, "pasteTaskCells"); });
        check(*doc.project() == before, "bad second paste row applied first row");
        QApplication::clipboard()->setText("\t1");
        dismissError([&] { action(window, "pasteTaskCells"); });
        check(*doc.project() == before, "blank name partially committed");
        table->clearSelection();
        table->selectRow(0);
        focus(window, table);
        QTest::keySequence(table, QKeySequence::Copy);
        check(jobs::decodeTaskTsv(QApplication::clipboard()->text()).front().size() == 3,
              "whole row copy leaked output columns/IDs");
        select(table, 0, 1, 0);
        selectCellRange(table, 0, 0, 1, 2, true);
        QTest::keySequence(table, QKeySequence::Copy);
        check(jobs::decodeTaskTsv(QApplication::clipboard()->text()).size() == 2, "rectangular cell copy failed");
        search->setText("zULu");
        check(wait([&] { return table->rowCount() == 1; }), "name search failed");
        check(id(table, 0) == QString::fromStdString(a) && *doc.project() == before, "filter changed data");
        check(!window.findChild<QAction *>("deleteTasks")->isEnabled(), "hidden selection remained a deletion target");
        search->setText(QString::fromStdString(a).toUpper());
        check(wait([&] { return table->rowCount() == 1 && id(table, 0) == QString::fromStdString(a); }),
              "ID search failed");
        search->clear();
        check(wait([&] { return table->rowCount() == 3; }), "clear search failed");
        filter->setCurrentIndex(2);
        check(table->rowCount() == 1 && id(table, 0) == QString::fromStdString(c), "milestone filter");
        filter->setCurrentIndex(1);
        check(table->rowCount() == 0 && window.findChild<QLabel *>("taskViewStatus")->text().contains("계산 필요"),
              "critical filter treated stale dates as current");
        filter->setCurrentIndex(0);
        calculate(window, doc);
        filter->setCurrentIndex(1);
        check(table->rowCount() > 0, "valid critical filter empty");
        filter->setCurrentIndex(0);
        // Keyboard insertion inserts before the active canonical task even while sorted.
        table->setCurrentCell(0, 0);
        const auto anchor = id(table, 0).toStdString();
        auto oldSize = doc.project()->tasks.size();
        focus(window, table);
        QTimer::singleShot(20, [&] {
            auto *dialog = QApplication::activeModalWidget();
            callback(dialog && dialog->objectName() == "insertTasksDialog", "insert keyboard dialog");
            dialog->findChild<QPlainTextEdit *>("insertTaskNames")->setPlainText("Inserted 1\nInserted 2");
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        });
        QTest::keyClick(table, Qt::Key_Insert);
        settle(window);
        check(doc.project()->tasks.size() == oldSize + 2, "keyboard insert failed");
        auto anchorIndex = std::find_if(doc.project()->tasks.begin(), doc.project()->tasks.end(),
                                        [&](const auto &t) { return t.id == anchor; }) -
                           doc.project()->tasks.begin();
        check(anchorIndex >= 2 && doc.project()->tasks[std::size_t(anchorIndex - 2)].name == "Inserted 1",
              "insert used sorted row position");
        sort->setCurrentIndex(4);
        check(cellText(table, table->rowCount() - 1, 1).isEmpty() &&
                  cellText(table, table->rowCount() - 2, 1).isEmpty(),
              "unknown duration did not sort last descending");
        before = *doc.project();
        focus(window, table);
        QTest::keySequence(table, QKeySequence("Ctrl+D"));
        settle(window);
        check(doc.project()->tasks.size() == oldSize + 4 && doc.project()->revision == before.revision + 1,
              "multi-selection duplicate shortcut");
        action(window, "undoEdit");
        check(static_cast<const jobs::PlanInput &>(*doc.project()) == static_cast<const jobs::PlanInput &>(before),
              "duplicate UI undo");
        // Select the inserted tasks again, inspect/cancel deletion, then confirm once.
        search->setText("Inserted");
        check(wait([&] { return table->rowCount() == 2; }), "inserted search");
        table->selectAll();
        before = *doc.project();
        QTimer::singleShot(20, [] {
            auto *d = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            callback(d && d->objectName() == "deleteTasksConfirmation", "delete preview absent");
            d->done(QMessageBox::No);
        });
        action(window, "deleteTasks");
        check(*doc.project() == before, "cancel deletion changed inputs");
        focus(window, table);
        QTimer::singleShot(20, [] {
            auto *d = qobject_cast<QMessageBox *>(QApplication::activeModalWidget());
            callback(d, "delete preview missing");
            callback(wait([&] { return d->button(QMessageBox::Yes)->isEnabled(); }) && d->text().contains("2"),
                     "delete task count missing");
            d->done(QMessageBox::Yes);
        });
        QTest::keyClick(table, Qt::Key_Delete);
        settle(window);
        check(doc.project()->tasks.size() == oldSize && table->rowCount() == 0, "delete keyboard/filtered IDs");
        action(window, "undoEdit");
        check(doc.project()->tasks.size() == oldSize + 2 && table->rowCount() == 2, "delete undo lost search/IDs");
        window.findChild<QPushButton *>("resetTaskView")->click();
        table->setCurrentCell(0, 0);
        focus(window, table);
        QTest::keySequence(&window, QKeySequence::Find);
        check(search->hasFocus(), "Ctrl+F search focus");
        focus(window, table);
        table->setCurrentCell(0, 0);
        const auto editorBefore = *doc.project();
        QTest::keyClick(table, Qt::Key_F2);
        QTest::qWait(10);
        QLineEdit *editor = nullptr;
        for (auto *field : table->findChildren<QLineEdit *>())
            if (field->isVisible())
                editor = field;
        check(editor, "F2 did not start cell editor");
        QApplication::clipboard()->setText("Editor paste");
        QTest::keySequence(editor, QKeySequence::Paste);
        QTest::keyClick(editor, Qt::Key_Delete);
        check(*doc.project() == editorBefore, "editor shortcuts invoked row operations");
        QTest::keyClick(editor, Qt::Key_Escape);
        check(*doc.project() == editorBefore, "Escape committed cell draft");
        window.grab().save("task-editing-smoke.png");
        doc.save();
        jobs::Document reader(store);
        reader.open(doc.path(), true);
        jobs::MainWindow readonly(reader);
        readonly.show();
        auto *rt = taskTable(readonly);
        rt->selectRow(0);
        QApplication::clipboard()->setText("Changed");
        check(!readonly.findChild<QAction *>("editTaskCell")->isEnabled() &&
                  !readonly.findChild<QAction *>("insertTasks")->isEnabled() &&
                  !readonly.findChild<QAction *>("deleteTasks")->isEnabled() &&
                  !readonly.findChild<QAction *>("duplicateTasks")->isEnabled() &&
                  !readonly.findChild<QAction *>("pasteTaskCells")->isEnabled() &&
                  readonly.findChild<QAction *>("copyTaskCells")->isEnabled(),
              "readonly actions incorrect");
        const auto readBefore = *reader.project();
        readonly.findChild<QComboBox *>("taskSort")->setCurrentIndex(2);
        check(*reader.project() == readBefore && !reader.dirty(), "readonly sort changed file inputs");
        readonly.hide();
        window.hide();
        std::cout << "PASS: TSV/limits, sorted atomic clipboard/undo, stable IDs/Gantt, search/filter/selection, "
                     "insert/duplicate/delete keyboard, readonly\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
