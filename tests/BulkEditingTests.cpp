#include "TaskTableTest.h"
#include "PerformanceFixture.h"
#include "ui/MainWindow.h"
#include "ui/AsyncEditController.h"
#include "persistence/JsonProjectStore.h"
#include <QAbstractItemModelTester>
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QFile>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <future>
#include <iostream>
namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
auto prepare(jobs::Document& doc, jobs::EditCommand command, std::shared_ptr<jobs::CalculationControl> control=std::make_shared<jobs::CalculationControl>()) {
    return std::async(std::launch::async,[request=doc.makeEditRequest(std::move(command)),control]() mutable {return jobs::Document::prepareEdit(std::move(request),control);}).get();
}
QByteArray bytes(const std::string& path){QFile file(QString::fromStdString(path));check(file.open(QIODevice::ReadOnly),"read file");return file.readAll();}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    using jobs::EditState;
    QTemporaryDir dir;jobs::JsonProjectStore store;const auto path=dir.filePath("Bulk.jobs").toStdString();
    auto fixture=performanceFixture(1000,false);store.save(path,fixture,{});jobs::Document doc(store);doc.open(path);
    for(int i=0;i<3;++i)doc.captureBaseline("B"+std::to_string(i),"PM","bulk regression","2026-10-28T00:00:00Z");
    doc.save();const auto original=*doc.project();const auto disk=bytes(path);
    auto result=prepare(doc,jobs::InsertTasksEdit{{"First","Second"},"T0",{}});
    check(result->state()==EditState::Ready&&*doc.project()==original,"preparation mutated source");
    doc.save();check(doc.applyPreparedEdit(*result)==EditState::Applied,"normal save invalidated bulk edit");
    const auto ids=doc.bulkView()->createdIds;check(ids.size()==2&&ids[0]!=ids[1]&&doc.project()->tasks[0].id==ids[0],"reserved IDs/order");
    jobs::validate(*doc.project());check(doc.project()->baselines==original.baselines&&doc.historyNeedsWorker(false),"history changed immutable records");
    const auto edited=static_cast<const jobs::PlanInput&>(*doc.project());
    auto undo=prepare(doc,jobs::HistoryEdit{false});check(doc.applyPreparedEdit(*undo)==EditState::Applied&&!doc.dirty()&&doc.canRedo(),"async undo/save point");
    check(static_cast<const jobs::PlanInput&>(*doc.project())==static_cast<const jobs::PlanInput&>(original),"undo input mismatch");
    auto redo=prepare(doc,jobs::HistoryEdit{true});check(doc.applyPreparedEdit(*redo)==EditState::Applied&&static_cast<const jobs::PlanInput&>(*doc.project())==edited,"redo changed stable IDs");
    check(doc.applyPreparedEdit(*redo)==EditState::Stale,"replayed twice");
    doc.undo();const auto before=*doc.project();const auto weight=doc.historyBytes();
    auto cancelled=std::make_shared<jobs::CalculationControl>();cancelled->cancel();
    result=prepare(doc,jobs::DuplicateTasksEdit{{"T0","T1"}},cancelled);check(result->state()==EditState::Cancelled,"early cancel");
    cancelled=std::make_shared<jobs::CalculationControl>();result=prepare(doc,jobs::RemoveTasksEdit{{"T0"}},cancelled);cancelled->cancel();
    check(doc.applyPreparedEdit(*result)==EditState::Cancelled&&*doc.project()==before&&doc.historyBytes()==weight&&doc.canRedo(),"late cancel lost source/history");
    // Deterministically cancel while the decoder is running, before any proposal exists.
    auto entered=std::make_shared<std::promise<void>>();auto enteredFuture=entered->get_future();
    cancelled=std::make_shared<jobs::CalculationControl>();
    auto runningRequest=doc.makeEditRequest(jobs::DeferredTaskEdit{[entered](const jobs::Project&) -> jobs::BulkTaskCommand {
        entered->set_value();for(;;)jobs::calculationCheckpoint();
    }});
    auto running=std::async(std::launch::async,[request=std::move(runningRequest),cancelled]()mutable{return jobs::Document::prepareEdit(std::move(request),cancelled);});
    check(enteredFuture.wait_for(std::chrono::seconds(5))==std::future_status::ready,"decoder did not start");cancelled->cancel();
    check(running.get()->state()==EditState::Cancelled&&*doc.project()==before&&doc.historyBytes()==weight,"in-flight cancellation changed source");
    result=prepare(doc,jobs::InsertTasksEdit{{"stale"},{},{}});doc.open(path);check(doc.applyPreparedEdit(*result)==EditState::Stale,"same revision reload accepted stale result");
    std::vector<jobs::TaskEdit> edits;for(int i=0;i<1000;++i){jobs::TaskEdit edit;edit.taskId="T"+std::to_string(i);edit.name="Changed "+std::to_string(i);edits.push_back(edit);}edits.back().name="";
    result=prepare(doc,jobs::BatchTaskEdit{edits});check(result->state()==EditState::Failed&&*doc.project()==original,"last row failure partially committed");
    jobs::TaskEdit cycle;cycle.taskId="T0";cycle.edges=std::vector<jobs::Dependency>{{"T100","FS",0}};
    result=prepare(doc,jobs::BatchTaskEdit{{cycle}});check(result->state()==EditState::Failed&&*doc.project()==original,"cycle accepted");
    result=prepare(doc,jobs::DuplicateTasksEdit{{"T0","T100"}});check(doc.applyPreparedEdit(*result)==EditState::Applied,"duplicate failed");
    auto copies=doc.bulkView()->createdIds;check(doc.project()->tasks[102].predecessors==std::vector<std::string>{copies[0]},"duplicate internal edge not remapped");jobs::validate(*doc.project());doc.undo();
    check(bytes(path)==disk,"preparation/edit/undo wrote source file");
    // A report task is protected even though historical input validation is skipped.
    doc.setStatusAt("2026-10-26 17:00");jobs::ProgressEntry progress;progress.taskId="T0";progress.actualStart="2026-10-26 09:00";progress.remainingMinutes=30;progress.recordedAt="2026-10-28T00:00:00Z";progress.reporter="PM";progress.reason="protected task";doc.recordProgress(progress);
    const auto protectedSource=*doc.project();result=prepare(doc,jobs::RemoveTasksEdit{{"T0","T1"}});
    check(result->state()==EditState::Failed&&*doc.project()==protectedSource,"protected removal partially committed");doc.open(path);
    jobs::MainWindow window(doc);window.show();QTest::qWait(20);auto* table=taskTable(window);jobs::AsyncEditController* controller=nullptr;
    for(auto* child:window.children())if(auto* c=dynamic_cast<jobs::AsyncEditController*>(child))controller=c;check(controller,"controller missing");
    auto settle=[&]{QElapsedTimer timer;timer.start();while(controller->busy()&&timer.elapsed()<30000)QTest::qWait(2);check(!controller->busy(),"worker timeout");};
    auto action=[&](const char* name){auto* command=window.findChild<QAction*>(name);check(command&&command->isEnabled(),"action missing");command->trigger();settle();};
    QAbstractItemModelTester tester(table->model(),QAbstractItemModelTester::FailureReportingMode::Fatal);
    int resets=0,inserts=0,removes=0;QObject::connect(table->model(),&QAbstractItemModel::modelReset,[&]{++resets;});QObject::connect(table->model(),&QAbstractItemModel::rowsInserted,[&]{++inserts;});QObject::connect(table->model(),&QAbstractItemModel::rowsRemoved,[&]{++removes;});
    QPersistentModelIndex survivor(table->model()->index(500,8));table->setCurrentCell(0,0);
    QTimer::singleShot(10,[&]{auto* dialog=QApplication::activeModalWidget();if(!dialog)std::_Exit(3);dialog->findChild<QPlainTextEdit*>()->setPlainText("Inserted A\nInserted B");dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
    action("insertTasks");check(resets==0&&inserts==1&&survivor.isValid()&&survivor.row()==502&&survivor.data()=="T500","contiguous insertion reset/lost persistent ID");
    check(table->selectionModel()->selectedRows().size()==2,"new ID selection not restored");action("undoEdit");check(resets==0&&removes==1&&survivor.row()==500&&!doc.dirty(),"contiguous undo reset/save point");
    action("redoEdit");check(inserts==2&&survivor.row()==502,"structural redo reset");action("undoEdit");
    table->setCurrentCell(0,0);QApplication::clipboard()->setText("Pasted A\t2\t\nPasted B\t3\t");action("pasteTaskCells");
    check(resets==0&&survivor.isValid()&&cellText(table,0,0)=="Pasted A"&&cellText(table,1,1)=="3","same order paste reset/stale data");action("undoEdit");
    table->clearSelection();table->setCurrentCell(0,0);selectCellRange(table,0,0,1,0,true);
    QTimer confirm;confirm.setInterval(5);QObject::connect(&confirm,&QTimer::timeout,[&]{auto* box=qobject_cast<QMessageBox*>(QApplication::activeModalWidget());if(box&&box->objectName()=="deleteTasksConfirmation"&&box->button(QMessageBox::Yes)->isEnabled()){confirm.stop();box->done(QMessageBox::Yes);}});confirm.start();
    action("deleteTasks");check(resets==0&&survivor.isValid()&&survivor.row()==498&&table->rowCount()==998,"contiguous deletion reset/lost persistent ID");action("undoEdit");check(survivor.row()==500&&!doc.dirty(),"delete undo mismatch");
    table->clearSelection();selectCellRange(table,0,0,0,0,true);selectCellRange(table,2,0,2,0,true);confirm.start();action("deleteTasks");check(resets==1&&!survivor.isValid(),"disjoint removal did not reset safely");action("undoEdit");
    auto* sort=window.findChild<QComboBox*>("taskSort");sort->setCurrentIndex(1);const int beforeReset=resets;table->setCurrentCell(0,0);action("duplicateTasks");check(resets>beforeReset&&table->rowCount()==1001,"sorted structural fallback");action("undoEdit");
    sort->setCurrentIndex(0);table->setCurrentCell(0,0);QApplication::clipboard()->setText("Cancelled");auto* paste=window.findChild<QAction*>("pasteTaskCells");const auto cancelSource=*doc.project();paste->trigger();window.findChild<QAction*>("cancelEdit")->trigger();settle();check(*doc.project()==cancelSource,"UI cancellation committed bulk input");
    if(app.arguments().contains("--native"))check(window.grab().save("bulk-editing-native.png"),"native capture");
    window.hide();std::cout<<"PASS: bulk atomicity, validation oracle, cancellation/stale/save, shared undo, model tester/persistent IDs, compact selection, disjoint/sorted fallback\n";
}catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}}
