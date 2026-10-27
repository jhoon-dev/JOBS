#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QElapsedTimer>
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QLabel>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
bool wait(const std::function<bool()>& f){QElapsedTimer t;t.start();while(!f()&&t.elapsed()<15000)QTest::qWait(10);return f();}
}
int main(int argc,char** argv){QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);QTimer watchdog;watchdog.setSingleShot(true);QObject::connect(&watchdog,&QTimer::timeout,[]{std::cerr<<"FAIL: performance UI timeout\n";std::_Exit(2);});watchdog.start(60000);try{
    QTemporaryDir dir;jobs::JsonProjectStore store;jobs::Project p;p.documentId="large-ui";p.name="Large UI";p.startDate="2026-10-26";
    for(int i=0;i<1000;++i){jobs::Task t{"T"+std::to_string(i),"Task "+std::to_string(i),60,{}};if(i>=100)t.predecessors={"T"+std::to_string(i-100)};p.tasks.push_back(t);}
    const auto path=dir.filePath("large.jobs").toStdString();store.save(path,p,{});jobs::Document doc(store);doc.open(path);doc.applyCalculation(jobs::calculate(p));jobs::MainWindow window(doc);window.show();app.processEvents();
    auto* view=dynamic_cast<jobs::GanttView*>(window.findChild<QGraphicsView*>("ganttView"));auto* table=taskTable(window);auto* cancel=window.findChild<QAction*>("cancelCalculation");QAction* calculate=nullptr;for(auto* a:window.findChildren<QAction*>())if(a->shortcut()==QKeySequence(Qt::Key_F9))calculate=a;
    check(view&&table&&cancel&&calculate&&table->rowCount()==1000,"large table or controls missing");
    const auto bound=[&]{return 8*((view->viewport()->height()+jobs::GanttView::HeaderHeight)/jobs::GanttView::RowHeight+12);};
    check(view->scene()->items().size()<bound(),"gantt created every task graphic");
    view->verticalScrollBar()->setValue(view->verticalScrollBar()->maximum());app.processEvents();bool last=false;
    for(auto* item:view->scene()->items())if(item->data(0)=="T999"&&item->data(1)=="task")last=true;
    check(last&&view->scene()->items().size()<bound(),"scroll omitted final task or exceeded viewport bound");
    jobs::Baseline baseline;baseline.schedule=doc.calculation()->tasks;view->setBaseline(&baseline);bool baselineLast=false,dependencyLast=false;
    for(auto* item:view->scene()->items()){baselineLast=baselineLast||(item->data(0)=="T999"&&item->data(1)=="baseline");dependencyLast=dependencyLast||(item->data(0)=="T899->T999"&&item->data(1)=="dependency");}
    check(baselineLast&&dependencyLast&&view->scene()->items().size()<bound(),"viewport lost baseline or external visible-row predecessor");view->setBaseline(nullptr);
    window.resize(1560,1050);app.processEvents();check(view->scene()->items().size()<bound(),"resize expanded all rows");
    table->selectRow(0);app.processEvents();check(view->verticalScrollBar()->value()<100,"selection did not reveal offscreen ID");view->setDayWidth(16);check(view->scene()->items().size()<bound(),"zoom expanded all graphics");
    const auto before=*doc.project();calculate->trigger();check(cancel->isEnabled(),"cancel not enabled during request");cancel->trigger();
    check(wait([&]{return calculate->isEnabled();})&&doc.calculation()->status==jobs::CalculationStatus::Cancelled,"UI race did not retain cancellation");
    check(doc.calculation()->tasks.empty()&&!doc.calculation()->projectFinish&&*doc.project()==before&&!doc.dirty(),"cancel changed input or exposed dates");
    for(auto* item:view->scene()->items())check(item->data(1)!="task"&&item->data(1)!="milestone","cancel left normal bars");
    check(!cancel->isEnabled()&&window.findChild<QLabel*>("scheduleProgress")->text().isEmpty(),"terminal progress not cleared");window.grab().save("performance-cancel-smoke.png");
    calculate->trigger();check(wait([&]{return calculate->isEnabled();})&&doc.calculation()->status==jobs::CalculationStatus::Valid&&doc.calculation()->tasks.size()==1000,"UI restart failed");
    // A drag edit queues a fresh revision while the old snapshot is running.
    calculate->trigger();view->taskEdited(doc.project()->documentId,doc.project()->revision,"T0",false,"2026-10-27 09:00");const auto editedRevision=doc.project()->revision;cancel->trigger();
    check(wait([&]{return calculate->isEnabled();}),"cancel with queued revision did not finish");QTest::qWait(100);
    check(calculate->isEnabled()&&doc.project()->revision==editedRevision&&doc.calculation()->status==jobs::CalculationStatus::Stale&&doc.calculation()->tasks.empty(),"explicit cancel restarted queued calculation or exposed old dates");
    calculate->trigger();check(wait([&]{return calculate->isEnabled();})&&doc.calculation()->status==jobs::CalculationStatus::Valid,"restart after queue cancellation failed");
    // Replacement while a result is queued for the UI must discard that result.
    calculate->trigger();const auto session=doc.sessionGeneration();doc.create("Replacement","2026-10-26",{"New task"});
    check(doc.sessionGeneration()!=session,"replacement retained session");check(wait([&]{return calculate->isEnabled();})&&!doc.calculation()&&doc.project()->name=="Replacement","late result crossed document sessions");
    window.hide();std::cout<<"PASS: viewport object bound, last row, offscreen stable selection, zoom, UI cancel/restart and late session rejection\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
