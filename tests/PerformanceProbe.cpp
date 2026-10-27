#include "ui/MainWindow.h"
#include "PerformanceFixture.h"
#include "BuildIdentity.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QTemporaryDir>
#include <QSysInfo>
#include <QThread>
#include <QScrollBar>
#include <QGraphicsScene>
#include <algorithm>
#include <iostream>
#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#else
#include <sys/resource.h>
#endif
namespace {
double peakBytes() {
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS info{};info.cb=sizeof(info);return GetProcessMemoryInfo(GetCurrentProcess(),&info,sizeof(info))?double(info.PeakWorkingSetSize):-1;
#else
    rusage info{};if(getrusage(RUSAGE_SELF,&info))return -1;
#ifdef Q_OS_MACOS
    return double(info.ru_maxrss);
#else
    return double(info.ru_maxrss)*1024;
#endif
#endif
}

QJsonObject samples(const std::vector<double>& values) {auto sorted=values;std::sort(sorted.begin(),sorted.end());QJsonArray raw;for(auto n:values)raw.append(n);const auto n=sorted.size();return {{"samples_ms",raw},{"median_ms",n%2?sorted[n/2]:(sorted[n/2-1]+sorted[n/2])/2},{"observed_p95_ms",sorted[(95*n+99)/100-1]}};}
}
int main(int argc,char** argv){QApplication app(argc,argv);app.setQuitOnLastWindowClosed(false);try {
    QCommandLineParser parser;parser.addHelpOption();parser.addOptions({{{"n","tasks"},"Task count","count","1000"},{{"r","repeats"},"Independent samples in this process","count","3"},{"profile","legacy or effort","name","legacy"},{"output","JSON output path","path","performance.json"},{"no-ui","Measure calculation only"}});parser.process(app);
    bool okay=false;const int count=parser.value("tasks").toInt(&okay);if(!okay||count<1||count>100000)throw std::runtime_error("tasks must be 1..100000");const int repeats=parser.value("repeats").toInt(&okay);if(!okay||repeats<1||repeats>100)throw std::runtime_error("repeats must be 1..100");const auto profile=parser.value("profile");if(profile!="legacy"&&profile!="effort")throw std::runtime_error("unknown profile");
    auto p=performanceFixture(count,profile=="effort");std::vector<double> calculation,preparation,scroll;std::string finish;QString fingerprint;qsizetype maxItems=0;QTemporaryDir directory;
    for(int sample=0;sample<repeats;++sample){std::cout<<"begin sample "<<sample+1<<" / "<<repeats<<" tasks="<<count<<" profile="<<profile.toStdString()<<std::endl;QElapsedTimer timer;timer.start();auto result=jobs::calculate(p);calculation.push_back(double(timer.nsecsElapsed())/1000000);
        if(result.status!=jobs::CalculationStatus::Valid||result.tasks.size()!=p.tasks.size()||!result.projectFinish)throw std::runtime_error("fixture did not produce a complete valid schedule");
        if(sample&&finish!=*result.projectFinish)throw std::runtime_error("nondeterministic finish");
        finish=*result.projectFinish;
        const auto digest=scheduleFingerprint(result);if(sample&&digest!=fingerprint)throw std::runtime_error("nondeterministic complete schedule");fingerprint=digest;
        if(count==1000){const auto expected=profile=="effort"?"98015f053bc59a7e9fc386c8e819c05d4c81bc52cef561f2454f1ea1d43debfe":"38274fbd24fd7979b8e58f0d5d7a16815c47c2150f79447ace8af8de2e9f0433";if(digest!=expected)throw std::runtime_error("complete schedule differs from pre-optimization 9a40185 reference");}
        if(!parser.isSet("no-ui")){jobs::JsonProjectStore store;const auto path=directory.filePath(QString::number(sample)+".jobs").toStdString();store.save(path,p,{});jobs::Document doc(store);doc.open(path);doc.applyCalculation(result);timer.restart();jobs::MainWindow window(doc);window.show();app.processEvents();preparation.push_back(double(timer.nsecsElapsed())/1000000);auto* view=dynamic_cast<jobs::GanttView*>(window.findChild<QGraphicsView*>("ganttView"));if(!view)throw std::runtime_error("gantt missing");
            for(int step=0;step<12;++step){timer.restart();view->verticalScrollBar()->setValue(int(std::int64_t(view->verticalScrollBar()->maximum())*step/11));app.processEvents();scroll.push_back(double(timer.nsecsElapsed())/1000000);maxItems=std::max(maxItems,view->scene()->items().size());}window.hide();
        }
    }
    QJsonObject out{{"fixture","jobs-perf-layered-v1"},{"schedule_sha256",fingerprint},{"status","success"},{"task_count",count},{"dependency_count",std::max(0,count-100)},{"resource_count",int(p.resources.size())},{"calendar_count",int(p.calendars.size())},{"split_task_count",profile=="effort"?(count+24)/25:0},{"profile",QString::fromStdString(p.calculationProfile)},{"project_finish",QString::fromStdString(finish)},{"repeats",repeats},{"calculation",samples(calculation)},{"peak_process_resident_bytes",peakBytes()},{"peak_memory_semantics","OS process high water mark; not an incremental allocation measurement"},{"os",QSysInfo::prettyProductName()},{"cpu_architecture",QSysInfo::currentCpuArchitecture()},{"logical_cpu_count",QThread::idealThreadCount()},{"qt",qVersion()},{"source_revision",JOBS_PERF_REVISION},{"build_configuration",JOBS_PERF_CONFIG}};
#ifdef _MSC_VER
    out["compiler"]=QString("MSVC %1").arg(_MSC_VER);
#else
    out["compiler"]=__VERSION__;
#endif
    out["built_product_source_sha256"]=JOBS_SOURCE_SHA256;
    if(!preparation.empty()){out["ui_preparation"]=samples(preparation);out["scroll"]=samples(scroll);out["maximum_gantt_items"]=double(maxItems);}
    QFile output(parser.value("output"));if(!output.open(QIODevice::WriteOnly)||output.write(QJsonDocument(out).toJson())<0)throw std::runtime_error("cannot write benchmark output");std::cout<<QJsonDocument(out).toJson(QJsonDocument::Compact).toStdString()<<'\n';
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
