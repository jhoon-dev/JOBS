#include "TaskTableTest.h"
#include "PerformanceFixture.h"
#include "ui/MainWindow.h"
#include "ui/TaskViewProjection.h"
#include "ui/TaskTableModel.h"
#include "ui/AsyncEditController.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QAbstractItemModelTester>
#include <QApplication>
#include <QAction>
#include <QClipboard>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <iostream>
#include <algorithm>
#include <future>
#include <chrono>
namespace {
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
template<class F> void wait(F f){QElapsedTimer timer;timer.start();while(!f()&&timer.elapsed()<30000)QTest::qWait(2);check(f(),"wait timeout");}
void wbs(jobs::MainWindow& window,const QString& id){
    auto* tree=window.findChild<QTreeWidget*>("wbsTree");
    for(QTreeWidgetItemIterator it(tree);*it;++it)if((*it)->data(0,Qt::UserRole).toString()==id){tree->setCurrentItem(*it);return;}
    throw std::runtime_error("WBS item missing");
}
jobs::AsyncEditController* controller(jobs::MainWindow& window){
    for(auto* child:window.children())if(auto* c=dynamic_cast<jobs::AsyncEditController*>(child))return c;
    throw std::runtime_error("controller missing");
}
void action(jobs::MainWindow& window,const char* name){auto* a=window.findChild<QAction*>(name);check(a,"action missing");wait([&]{return a->isEnabled();});a->trigger();wait([&]{return !controller(window)->busy();});}
void oracle(jobs::MainWindow& window,jobs::Document& doc,const jobs::TaskViewSpec& spec){
    auto* table=taskTable(window);const auto& result=doc.calculation();
    const auto expected=jobs::projectTaskView(*doc.project(),result?&*result:nullptr,spec);
    check(table->rowCount()==int(expected.rows.size()),"projection row count differs from full oracle");
    jobs::TaskTableModel full(doc);full.synchronize(expected.rows);
    for(int r=0;r<table->rowCount();++r){
        check(table->model()->headerData(r,Qt::Vertical)==full.headerData(r,Qt::Vertical),"canonical header differs");
        for(int c=0;c<14;++c)check(cellText(table,r,c)==full.index(r,c).data().toString(),"projected cell differs from full oracle");
    }
    const auto rollup=jobs::summarizeWbs(*doc.project(),result?&*result:nullptr,spec.wbs);
    const auto summary=window.findChild<QLabel*>("wbsSummary")->text();
    check(summary.contains(QString("작업 %1개").arg(rollup.taskCount))&&summary.contains(QString("기간 미정 %1개").arg(rollup.missingDurations)),"WBS rollup narrowed by filter");
    check(window.findChild<QLabel*>("taskViewStatus")->text().contains(QString("WBS 범위 %1").arg(expected.scopeCount)),"WBS count became search count");
}
}
int main(int argc,char** argv){QApplication app(argc,argv);try{
    QTemporaryDir dir;jobs::JsonProjectStore store;auto fixture=performanceFixture(1000,false);
    fixture.wbsNodes={{"A","Scope A",{}},{"A1","Child","A"},{"B","Other",{}}};
    fixture.resources={{"R","Crew","labor",100,{}}};
    for(int i=0;i<1000;++i){fixture.tasks[std::size_t(i)].name="Task "+std::to_string(10000+i);fixture.tasks[std::size_t(i)].wbsId=i<500?"A1":"B";if(i%5==0)fixture.tasks[std::size_t(i)].durationMinutes=0;if(i%3==0)fixture.assignments.push_back({"T"+std::to_string(i),"R",100});}
    const auto path=dir.filePath("Views.jobs").toStdString();store.save(path,fixture,{});jobs::Document doc(store);doc.open(path);
    doc.captureBaseline("Approved","PM","view tests","2026-10-28T00:00:00Z");doc.save();const auto original=*doc.project();
    for(int mode=0;mode<7;++mode){
        doc.open(path);jobs::MainWindow window(doc);window.show();QTest::qWait(10);auto* table=taskTable(window);
        jobs::TaskViewSpec spec;
        if(mode==0||mode==4)spec.query="tAsK";
        if(mode==1||mode==4)spec.sort=1;
        if(mode==2||mode==4)spec.wbs="A";
        if(mode==3)spec.filter=2;
        if(mode==5)spec.filter=3;
        if(mode==6)spec.sort=4;
        if(!spec.wbs.empty())wbs(window,QString::fromStdString(spec.wbs));
        window.findChild<QComboBox*>("taskSort")->setCurrentIndex(spec.sort);
        window.findChild<QComboBox*>("taskFilter")->setCurrentIndex(spec.filter);
        window.findChild<QLineEdit*>("taskSearch")->setText(spec.query);QTest::qWait(210);
        oracle(window,doc,spec);
        QAbstractItemModelTester tester(table->model(),QAbstractItemModelTester::FailureReportingMode::Fatal);
        int resets=0,layouts=0;QObject::connect(table->model(),&QAbstractItemModel::modelReset,[&]{++resets;});QObject::connect(table->model(),&QAbstractItemModel::layoutChanged,[&]{++layouts;});
        QPersistentModelIndex survivor(table->model()->index(table->rowCount()-1,8));const auto survivorId=survivor.data();
        table->setCurrentCell(0,mode==3?1:0);selectCellRange(table,0,0,9,0);
        QStringList lines;for(int i=0;i<10;++i)lines<<(mode==3?QString("1"):QString("Z edited %1").arg(i));
        QApplication::clipboard()->setText(lines.join('\n'));action(window,"pasteTaskCells");oracle(window,doc,spec);
        check(resets==0&&survivor.isValid()&&survivor.data()==survivorId,"paste reset/lost survivor ID");
        if(spec.sort==1&&spec.query.isEmpty())check(layouts>0,"name sorting did not use layout change");
        if(!spec.query.isEmpty())check(!table->selectionModel()->hasSelection(),"filtered-out edits remained selected");
        action(window,"undoEdit");oracle(window,doc,spec);check(!doc.dirty()&&doc.project()->baselines==original.baselines,"undo save point/archive mismatch");
        table->setCurrentCell(0,0);selectCellRange(table,0,0,2,0);action(window,"duplicateTasks");oracle(window,doc,spec);
        check(resets==0&&survivor.isValid()&&survivor.data()==survivorId,"duplicate reset/lost survivor");
        action(window,"undoEdit");oracle(window,doc,spec);
        table->setCurrentCell(0,0);
        QTimer::singleShot(10,[]{auto* d=QApplication::activeModalWidget();if(!d)std::_Exit(3);d->findChild<QPlainTextEdit*>()->setPlainText("Inserted view task");d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();});
        action(window,"insertTasks");oracle(window,doc,spec);check(resets==0,"insert reset projected model");action(window,"undoEdit");oracle(window,doc,spec);
        check(static_cast<const jobs::PlanInput&>(*doc.project())==static_cast<const jobs::PlanInput&>(original)&&!doc.dirty(),"view editing changed wrong input");
        if(app.arguments().contains("--native")&&mode==4)check(window.grab().save("projected-views-native.png"),"native capture failed");
        window.hide();
    }
    {
        doc.open(path);jobs::MainWindow window(doc);window.show();QTest::qWait(10);auto* table=taskTable(window);
        auto* sort=window.findChild<QComboBox*>("taskSort");sort->setCurrentIndex(1);
        QAbstractItemModelTester tester(table->model(),QAbstractItemModelTester::FailureReportingMode::Fatal);
        int resets=0;QObject::connect(table->model(),&QAbstractItemModel::modelReset,[&]{++resets;});
        // 129 disjoint sorted insert ranges exceed the GUI mutation budget.
        table->setCurrentCell(0,0);selectCellRange(table,0,0,128,0);action(window,"duplicateTasks");
        check(resets==1,"fragmented changes did not use bounded prepared reset");oracle(window,doc,{{},{},0,1});action(window,"undoEdit");
        table->setCurrentCell(0,0);QApplication::clipboard()->setText("Changed captured task");auto* paste=window.findChild<QAction*>("pasteTaskCells");wait([&]{return paste->isEnabled();});paste->trigger();wbs(window,"B");wait([&]{return !controller(window)->busy();});
        oracle(window,doc,{"B",{},0,1});check(doc.project()->tasks[0].name=="Changed captured task"&&table->rowCount()==500,"view race retargeted the captured edit");
        window.hide();
    }
    {
        doc.open(path);jobs::MainWindow window(doc);window.show();QTest::qWait(10);
        window.findChild<QComboBox*>("taskSort")->setCurrentIndex(1);auto* table=taskTable(window);
        table->setCurrentCell(0,0);
        auto entered=std::make_shared<std::promise<void>>();auto started=entered->get_future();
        auto release=std::make_shared<std::promise<void>>();auto released=release->get_future().share();
        controller(window)->submit(jobs::DeferredTaskEdit{[entered,released](const jobs::Project&) -> jobs::BulkTaskCommand {
            entered->set_value();while(released.wait_for(std::chrono::milliseconds(1))!=std::future_status::ready)jobs::calculationCheckpoint();
            jobs::TaskEdit edit;edit.taskId="T0";edit.name="Older worker value";return jobs::BatchTaskEdit{{edit}};
        }},"draft publication test",{});
        check(started.wait_for(std::chrono::seconds(5))==std::future_status::ready,"draft worker did not start");
        table->setCurrentCell(500,0);const auto draftId=cellText(table,500,8).toStdString();table->edit(cell(table,500,0));
        QLineEdit* editor=nullptr;for(auto* field:table->findChildren<QLineEdit*>())if(field->isVisible())editor=field;
        check(editor,"pending editor missing");editor->setText("New user draft");release->set_value();
        wait([&]{return !controller(window)->busy();});
        const auto found=std::find_if(doc.project()->tasks.begin(),doc.project()->tasks.end(),[&](const auto& t){return t.id==draftId;});
        check(found!=doc.project()->tasks.end()&&found->name=="New user draft"&&doc.project()->tasks[0].name==original.tasks[0].name,"worker publication overwrote a new cell draft");
        oracle(window,doc,{{},{},0,1});window.hide();
    }
    {
        doc.open(path);jobs::MainWindow window(doc);window.show();
        for(auto* a:window.findChildren<QAction*>())if(a->shortcut()==QKeySequence(Qt::Key_F9)){a->trigger();break;}
        wait([&]{return doc.calculation()&&doc.calculation()->status==jobs::CalculationStatus::Valid;});
        window.findChild<QComboBox*>("taskFilter")->setCurrentIndex(1);auto* table=taskTable(window);check(table->rowCount()>0,"critical fixture empty");
        QAbstractItemModelTester tester(table->model(),QAbstractItemModelTester::FailureReportingMode::Fatal);
        table->setCurrentCell(0,0);QApplication::clipboard()->setText("Critical changed");action(window,"pasteTaskCells");
        check(table->rowCount()==0&&window.findChild<QLabel*>("taskViewStatus")->text().contains("계산 필요"),"stale critical rows retained");
        auto* gantt=window.findChild<QGraphicsView*>("ganttView");for(auto* item:gantt->scene()->items())check(item->data(1)!="task","stale Gantt bar retained");
        oracle(window,doc,{{},{},1,0});window.hide();
    }
    std::cout<<"PASS: projected search/sort/WBS/filter oracle, membership/layout/persistent IDs, range budget, view race and stale critical/Gantt\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
