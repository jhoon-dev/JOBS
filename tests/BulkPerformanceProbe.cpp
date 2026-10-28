#include "BuildIdentity.h"
#include "PerformanceFixture.h"
#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/AsyncEditController.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <future>
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        const auto args = app.arguments();
        const int n = args.value(args.indexOf("--tasks") + 1, "1000").toInt();
        const int baselines = args.value(args.indexOf("--baselines") + 1, "0").toInt();
        check((n == 1000 || n == 10000) && (baselines == 0 || baselines == 3), "fixture bounds");
        QTemporaryDir dir; jobs::JsonProjectStore store;
        auto path = dir.filePath("Bulk.jobs").toStdString(); store.save(path, performanceFixture(n, false), {});
        jobs::Document doc(store); doc.open(path);
        for (int i = 0; i < baselines; ++i) doc.captureBaseline("B" + std::to_string(i), "PM", "measurement", "2026-10-28T00:00:00Z");
        doc.save(); jobs::MainWindow window(doc); window.show(); QTest::qWait(30);
        auto* table = taskTable(window); jobs::AsyncEditController* controller = nullptr;
        // AsyncEditController has no Q_OBJECT: find by RTTI among QObject children.
        controller = nullptr; for (auto* child : window.children()) if (auto* c = dynamic_cast<jobs::AsyncEditController*>(child)) controller = c;
        check(controller, "controller missing");
        int resets = 0, inserted = 0, removed = 0;
        QObject::connect(table->model(), &QAbstractItemModel::modelReset, [&] { ++resets; });
        QObject::connect(table->model(), &QAbstractItemModel::rowsInserted, [&] { ++inserted; });
        QObject::connect(table->model(), &QAbstractItemModel::rowsRemoved, [&] { ++removed; });
        QJsonArray operations; const int count = n / 10;
        auto measure = [&](const char* name, const char* action) {
            QElapsedTimer clock, heartbeat; clock.start(); heartbeat.start(); qint64 maxGap = 0; int ticks = 0;
            QTimer pulse; pulse.setInterval(5); QObject::connect(&pulse, &QTimer::timeout, [&] { maxGap = std::max(maxGap, heartbeat.restart()); ++ticks; }); pulse.start();
            const int r = resets, i = inserted, d = removed;
            auto* command = window.findChild<QAction*>(action); check(command && command->isEnabled(), "action disabled"); command->trigger();
            const double trigger = clock.nsecsElapsed() / 1e6;
            while (controller->busy() && clock.elapsed() < 120000) QTest::qWait(2);
            check(!controller->busy(), "operation timeout"); app.processEvents(); maxGap = std::max(maxGap, heartbeat.elapsed());
            operations.append(QJsonObject{{"operation", name}, {"trigger_return_ms", trigger}, {"settled_ms", clock.nsecsElapsed() / 1e6},
                {"heartbeat_ticks", ticks}, {"observed_max_heartbeat_gap_ms", maxGap}, {"model_resets", resets-r}, {"row_insert_signals", inserted-i}, {"row_remove_signals", removed-d}});
        };
        table->setCurrentCell(0,0);
        QStringList names; for (int i = 0; i < count; ++i) names << "Inserted " + QString::number(i);
        QTimer::singleShot(10, [&] { auto* modal = QApplication::activeModalWidget(); if (!modal) std::_Exit(3); modal->findChild<QPlainTextEdit*>()->setPlainText(names.join('\n')); modal->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click(); });
        measure("insert", "insertTasks"); check(int(doc.project()->tasks.size()) == n + count, "insert failed");
        measure("undo_insert", "undoEdit"); check(int(doc.project()->tasks.size()) == n, "undo insert failed");
        table->clearSelection(); table->setCurrentCell(0,0); selectCellRange(table,0,0,count-1,0,true);
        measure("duplicate", "duplicateTasks"); check(int(doc.project()->tasks.size()) == n + count, "duplicate failed");
        measure("undo_duplicate", "undoEdit");
        table->setCurrentCell(0,0); QStringList rows; for (int i=0;i<count;++i) rows << "Pasted " + QString::number(i) + "\t2\t";
        QApplication::clipboard()->setText(rows.join('\n'));
        measure("paste", "pasteTaskCells"); check(doc.project()->tasks[0].name == "Pasted 0", "paste failed");
        measure("undo_paste", "undoEdit");
        table->clearSelection(); table->setCurrentCell(0,0); selectCellRange(table,0,0,count-1,0,true);
        QTimer confirm; confirm.setInterval(5); QObject::connect(&confirm,&QTimer::timeout,[&] { auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); if(box && box->objectName()=="deleteTasksConfirmation" && box->button(QMessageBox::Yes)->isEnabled()) { confirm.stop(); box->done(QMessageBox::Yes); } }); confirm.start();
        measure("delete", "deleteTasks"); check(int(doc.project()->tasks.size()) == n-count, "delete failed");
        measure("undo_delete", "undoEdit"); check(int(doc.project()->tasks.size()) == n && !doc.dirty(), "undo/save point failed");
        std::vector<jobs::TaskEdit> changes;
        for(int i=0;i<count;++i){jobs::TaskEdit edit;edit.taskId="T"+std::to_string(i);edit.name="Phase "+std::to_string(i);changes.push_back(std::move(edit));}
        QElapsedTimer phase;phase.start();auto request=doc.makeEditRequest(jobs::BatchTaskEdit{std::move(changes)});
        const double requestMs=phase.nsecsElapsed()/1e6;
        auto [prepared,workerMs]=std::async(std::launch::async,[request=std::move(request)]()mutable{
            QElapsedTimer clock;clock.start();auto result=jobs::Document::prepareEdit(std::move(request),std::make_shared<jobs::CalculationControl>());
            return std::pair{result,clock.nsecsElapsed()/1e6};}).get();
        check(prepared->state()==jobs::EditState::Ready,"phase preparation failed");phase.restart();
        check(doc.applyPreparedEdit(*prepared)==jobs::EditState::Applied,"phase apply failed");const double applyMs=phase.nsecsElapsed()/1e6;
        QJsonObject output{{"fixture","jobs-bulk-ui-v1"},{"status","success"},{"task_count",n},{"baseline_count",baselines},{"affected_tasks",count},
            {"batch_phases_ms",QJsonObject{{"request_gui",requestMs},{"prepare_worker",workerMs},{"apply_gui",applyMs}}},{"operations",operations},{"source_revision",JOBS_PERF_REVISION},{"built_product_source_sha256",JOBS_SOURCE_SHA256},{"platform",QApplication::platformName()},
            {"scope","One sample per operation; trigger includes automated modal input/confirmation; settled includes worker and GUI refresh, excludes initial open/save/baselines and OS presentation"}};
        QFile file(args.value(args.indexOf("--output")+1)); check(file.open(QIODevice::WriteOnly),"output unavailable"); file.write(QJsonDocument(output).toJson());
        std::cout << "PASS: bulk UI " << n << '/' << baselines << '\n';
    } catch(const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
