#include "persistence/ProjectXml.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include "application/Document.h"
#include "calculation/Scheduler.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <functional>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool condition,const char* message){if(!condition)throw std::runtime_error(message);}
void rejects(const std::function<void()>& f){bool bad=false;try{f();}catch(const std::exception&){bad=true;}check(bad,"expected rejection");}
QByteArray read(const QString& path){QFile f(path);check(f.open(QIODevice::ReadOnly),"read file");return f.readAll();}
QByteArray changed(QByteArray source,const QByteArray& before,const QByteArray& after){check(source.contains(before),"mutation target missing");return source.replace(before,after);}
bool hasDiagnostic(const jobs::ProjectXmlExchange& e,const QString& fragment){for(const auto& d:e.report["diagnostics"].toArray())if(d.toObject()["path"].toString().contains(fragment))return true;return false;}
const jobs::TaskSchedule& scheduled(const jobs::ScheduleResult& r,const std::string& id){for(const auto& t:r.tasks)if(t.taskId==id)return t;throw std::runtime_error("schedule missing");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    const auto fixture=read(QString::fromUtf8(JOBS_XML_FIXTURE));
    auto imported=jobs::importProjectXml(fixture,"원본.xml");const auto& p=imported.project;
    check(p.name=="한글 계획 & 교환"&&p.startDate=="2026-10-26"&&p.tasks.size()==6&&p.wbsNodes.size()==1,"task/WBS identity");
    check(p.tasks[0].id=="xml-task-20"&&p.tasks[0].wbsId=="xml-wbs-10"&&p.tasks[0].durationMinutes==480,"UID based mapping");
    check(p.calendars.size()==2&&p.calendars[0].holidays==std::vector<std::string>{"2026-10-27"}&&p.calendars[1].holidays.size()==2,"calendar inheritance/holidays");
    check(p.resources[0].maxUnits==150&&p.resources[0].calendarId=="xml-cal-2"&&p.assignments[0].units==50&&p.assignments[1].units==125,"resource mapping");
    check(p.tasks[1].constraintType=="SNET"&&p.tasks[1].constraintAt=="2026-10-28 10:00"&&p.tasks[1].deadlineAt=="2026-10-29 18:00","constraint mapping");
    check(p.tasks[5].durationMinutes==0&&p.progress.empty()&&p.costEntries.empty()&&p.taskCosts.empty(),"no invented actual/cost");
    for(std::size_t i=1;i<5;++i){const auto d=jobs::dependencies(p.tasks[i]);const std::vector<std::string> types{"FS","SS","FF","SF"};check(d.size()==1&&d[0].type==types[i-1]&&d[0].lagMinutes==((i==2||i==3)?-60:60),"four links and tenths lag");}
    check(hasDiagnostic(imported,"ActualWork")&&hasDiagnostic(imported,"FixedCost")&&hasDiagnostic(imported,"StandardRate")&&hasDiagnostic(imported,"CustomFields"),"loss report completeness");
    auto placeholder=changed(fixture,"<Resources>","<Resources><Resource><UID>0</UID><Name/></Resource>");check(jobs::importProjectXml(placeholder,"placeholder.xml").project.resources.size()==1,"unassigned resource placeholder");
    auto placeholderReference=changed(placeholder,"<ResourceUID>99</ResourceUID>","<ResourceUID>0</ResourceUID>");rejects([&]{jobs::importProjectXml(placeholderReference,"placeholder-reference.xml");});
    auto schedule=jobs::calculate(p);check(schedule.status==jobs::CalculationStatus::Valid,"import scheduling");
    check(scheduled(schedule,"xml-task-20").start=="2026-10-26 09:00"&&scheduled(schedule,"xml-task-20").finish=="2026-10-26 18:00","independent A dates");
    check(scheduled(schedule,"xml-task-30").start=="2026-10-28 10:00"&&scheduled(schedule,"xml-task-30").finish=="2026-10-28 12:00","independent lag and holiday dates");
    check(scheduled(schedule,"xml-task-50").start=="2026-10-26 15:00"&&scheduled(schedule,"xml-task-50").finish=="2026-10-26 17:00","independent FF negative lag");
    QTemporaryDir temporary;check(temporary.isValid(),"temporary directory");
    auto folder=jobs::publishProjectXml(imported,temporary.filePath("import"));
    check(read(QDir(folder).filePath("original.xml"))==fixture,"exact source bytes");
    auto report=QJsonDocument::fromJson(read(QDir(folder).filePath("conversion.json"))).object();
    check(report["source"].toObject()["sha256"].toString().toLatin1()==QCryptographicHash::hash(fixture,QCryptographicHash::Sha256).toHex(),"source hash");
    auto compatibility=report["compatibility"].toObject();check(compatibility["jobsRecomputation"]=="unverified"&&compatibility["externalRoundTrip"]=="unverified","independent compatibility verdicts");
    jobs::JsonProjectStore store;jobs::Document doc(store);doc.open(QDir(folder).filePath("project.jobs").toUtf8().toStdString());check(*doc.project()==p&&!doc.dirty(),"native reopen");
    const auto saved=read(QDir(folder).filePath("project.jobs"));rejects([&]{jobs::publishProjectXml(imported,folder);});check(read(QDir(folder).filePath("project.jobs"))==saved,"existing destination preserved");
    rejects([&]{jobs::publishProjectXml(imported,temporary.filePath("missing/child"));});
    const auto before=*doc.project();const auto generation=doc.sessionGeneration();
    auto exported=jobs::exportProjectXml(p);auto back=jobs::importProjectXml(exported.xml,"roundtrip.xml");
    check(back.project.tasks.size()==p.tasks.size()&&back.project.wbsNodes.size()==1&&back.project.assignments.size()==2,"supported subset roundtrip counts");
    const auto again=jobs::calculate(back.project);check(again.status==jobs::CalculationStatus::Valid,"roundtrip schedule valid");
    for(std::size_t i=0;i<p.tasks.size();++i){const auto& a=p.tasks[i];const auto& b=back.project.tasks[i];check(a.name==b.name&&a.durationMinutes==b.durationMinutes&&a.constraintType==b.constraintType&&a.constraintAt==b.constraintAt&&a.deadlineAt==b.deadlineAt,"roundtrip input");const auto& x=scheduled(schedule,a.id);const auto& y=scheduled(again,b.id);check(x.start==y.start&&x.finish==y.finish&&x.totalFloatMinutes==y.totalFloatMinutes,"roundtrip date semantics");}
    check(*doc.project()==before&&doc.sessionGeneration()==generation&&!doc.dirty()&&!doc.canUndo(),"export does not edit document");
    auto output=jobs::publishProjectXml(exported,temporary.filePath("export"));check(store.load(QDir(output).filePath("source.jobs").toStdString()).project==p,"export source snapshot");
    auto unknown=p;unknown.tasks[0].durationMinutes.reset();unknown.resources[0].maxUnits.reset();
    auto un=jobs::exportProjectXml(unknown);auto unBack=jobs::importProjectXml(un.xml,"unknown.xml");check(!unBack.project.tasks[0].durationMinutes&&!unBack.project.resources[0].maxUnits&&hasDiagnostic(un,"Duration")&&hasDiagnostic(un,"MaxUnits"),"unknown not zero");
    auto work=p;work.tasks[0].taskType="fixed-work";work.tasks[0].workDriven=true;work.tasks[0].workUnitMinutes=24000;work.tasks[0].breaks={{"2026-10-26 10:00","2026-10-26 11:00"}};auto projection=jobs::exportProjectXml(work);check(hasDiagnostic(projection,"xml-task-20")&&jobs::importProjectXml(projection.xml,"work.xml").project.tasks[0].taskType=="fixed-duration","effort loss explicit");
    // Native export has no WBS: canonical order, a default calendar, and unknown duration survive.
    jobs::Project plain;plain.documentId="plain";plain.name="기본";plain.startDate="2026-10-26";plain.tasks={{"first","설계",60,{}},{"second","미정",std::nullopt,{"first"}}};
    auto plainBack=jobs::importProjectXml(jobs::exportProjectXml(plain).xml,"plain.xml");check(plainBack.project.tasks.size()==2&&!plainBack.project.tasks[1].durationMinutes,"legacy default calendar export");
    auto rejectChange=[&](const QByteArray& from,const QByteArray& to){auto b=changed(fixture,from,to);rejects([&]{jobs::importProjectXml(b,"bad.xml");});};
    rejectChange("<UID>30</UID>","<UID>20</UID>");rejectChange("<UID>2</UID>","<UID>1</UID>");
    rejectChange("<UID>102</UID>","<UID>101</UID>");rejectChange("<ResourceUID>99</ResourceUID>","<ResourceUID>98</ResourceUID>");
    rejectChange("<TaskUID>20</TaskUID>","<TaskUID>10</TaskUID>");rejectChange("<PredecessorUID>20</PredecessorUID>","<PredecessorUID>10</PredecessorUID>");
    rejectChange("<UID>30</UID>","<UID>30</UID><Manual>1</Manual>");rejectChange("<PredecessorUID>20</PredecessorUID>","<PredecessorUID>30</PredecessorUID>");
    rejectChange("<LinkLag>600</LinkLag>","<LinkLag>601</LinkLag>");rejectChange("<LagFormat>3</LagFormat>","<LagFormat>4</LagFormat>");
    rejectChange("<LagFormat>3</LagFormat>","");
    rejectChange("<Duration>PT8H0M0S</Duration>","<Duration>PT0H0M1S</Duration>");rejectChange("<Duration>PT8H0M0S</Duration>","<Duration>PT8H0M0S</Duration><DurationFormat>8</DurationFormat>");
    rejectChange("<Units>0.50</Units>","<Units>0.505</Units>");rejectChange("<Units>0.50</Units>","<Units>0</Units>");
    rejectChange("<StartDate>2026-10-26T09:00:00</StartDate>","<StartDate>2026-10-26T09:00:00Z</StartDate>");
    rejectChange("<BaseCalendarUID>1</BaseCalendarUID>","<BaseCalendarUID>2</BaseCalendarUID>");
    rejectChange("<BaseCalendarUID>1</BaseCalendarUID>","<BaseCalendarUID>3</BaseCalendarUID>");
    rejectChange("<Exceptions>","<WorkWeeks/><Exceptions>");rejectChange("<Period>1</Period>","<Period>2</Period>");
    rejectChange("<Name>상속 달력</Name>","<Name>상속 달력</Name><WeekDays><WeekDay><DayType>2</DayType><DayWorking>1</DayWorking><WorkingTimes><WorkingTime><FromTime>10:00:00</FromTime><ToTime>11:00:00</ToTime></WorkingTime></WorkingTimes></WeekDay></WeekDays>");
    rejectChange("<ScheduleFromStart>1</ScheduleFromStart>","<ScheduleFromStart>0</ScheduleFromStart>");
    rejectChange("<UID>20</UID>","<UID>20</UID><IsSubproject>1</IsSubproject>");
    rejectChange("<UID>20</UID>","<UID>20</UID><Project/>");
    rejectChange("<CurrencyCode>KRW</CurrencyCode>","<CurrencyCode>GBP</CurrencyCode>");
    rejectChange("<CalendarUID>2</CalendarUID>","<CalendarUID>3</CalendarUID>");
    rejectChange("<UID>102</UID>","");
    rejectChange("<UID>20</UID>","<UID>20</UID><PredecessorLink><PredecessorUID>30</PredecessorUID></PredecessorLink>");
    rejectChange("<UID>99</UID>","<UID>99</UID><Type>0</Type>");
    auto dtd=changed(fixture,"<Project xmlns=","<!DOCTYPE Project [<!ENTITY a 'expanded'>]><Project xmlns=");rejects([&]{jobs::importProjectXml(dtd,"entity.xml");});
    auto foreign=changed(fixture,"<CustomFields>","<CustomFields xmlns='urn:foreign'>");rejects([&]{jobs::importProjectXml(foreign,"foreign.xml");});
    rejects([&]{jobs::importProjectXml(fixture.left(fixture.size()-15),"truncated.xml");});rejects([&]{jobs::importProjectXml(QByteArray(16*1024*1024+1,' '),"large.xml");});
    auto deep=fixture;QByteArray open,close;for(int i=0;i<33;++i){open+="<X>";close+="</X>";}deep.replace("<CustomFields>","<CustomFields>"+open).replace("</CustomFields>",close+"</CustomFields>");rejects([&]{jobs::importProjectXml(deep,"depth.xml");});
    auto many=fixture;QByteArray rows;for(int i=0;i<10001;++i)rows+="<Task><UID>"+QByteArray::number(1000+i)+"</UID></Task>";many.replace("</Tasks>",rows+"</Tasks>");rejects([&]{jobs::importProjectXml(many,"rows.xml");});
    auto elements=fixture;elements.replace("</CustomFields>",QByteArray("<X/>").repeated(300001)+"</CustomFields>");rejects([&]{jobs::importProjectXml(elements,"elements.xml");});
    auto expanded=fixture;expanded.replace("2026-10-27T00:00:00","2000-01-01T00:00:00");expanded.replace("2026-10-27T23:59:00","2024-01-01T23:59:00");QByteArray inherited;for(int i=3;i<16;++i)inherited+="<Calendar><UID>"+QByteArray::number(i)+"</UID><IsBaseCalendar>0</IsBaseCalendar><BaseCalendarUID>1</BaseCalendarUID></Calendar>";expanded.replace("</Calendars>",inherited+"</Calendars>");rejects([&]{jobs::importProjectXml(expanded,"expanded.xml");});
    check(*doc.project()==before,"rejection never mutates current document");
    jobs::Document tracked(store);tracked.create("승인·실적 원본","2026-10-26",{"보고 작업"});tracked.setDuration(0,60);tracked.saveAs(temporary.filePath("tracked.jobs").toStdString());
    tracked.captureBaseline("승인 기준선","검증자","XML 원본 보존","2026-10-26T00:00:00Z");tracked.setStatusAt("2026-10-26 09:30");
    jobs::ProgressEntry progress;progress.taskId=tracked.project()->tasks[0].id;progress.actualStart="2026-10-26 09:00";progress.actualDurationMinutes=30;progress.remainingMinutes=30;progress.reporter="현장";progress.reason="계측";progress.recordedAt="2026-10-26T00:30:00Z";tracked.recordProgress(progress);
    jobs::CostEntry cost;cost.taskId=progress.taskId;cost.actualMinor=1234;cost.physicalBasisPoints=5000;cost.reporter="원가";cost.reason="계측";cost.recordedAt="2026-10-26T00:30:00Z";tracked.recordCost(cost);
    auto historySnapshot=*tracked.project();auto historyExport=jobs::exportProjectXml(historySnapshot);auto historyFolder=jobs::publishProjectXml(historyExport,temporary.filePath("history-export"));
    check(hasDiagnostic(historyExport,"history-costs")&&store.load(QDir(historyFolder).filePath("source.jobs").toStdString()).project==historySnapshot&&*tracked.project()==historySnapshot,"source snapshot lost approved/actual/cost ledger");
    check(!historyExport.xml.contains("<Baseline>")&&!historyExport.xml.contains("<ActualStart>")&&!historyExport.xml.contains("<ActualCost>"),"unmapped histories invented in XML");
    // Stable exports for an independent Python XML reader, not produced by the importer under test.
    QFile out("project-xml-export.xml");check(out.open(QIODevice::WriteOnly)&&out.write(exported.xml)==exported.xml.size(),"write independent XML fixture");out.close();
    QFile reportOut("project-xml-export.json");check(reportOut.open(QIODevice::WriteOnly),"write independent report");reportOut.write(QJsonDocument(exported.report).toJson());
    std::cout<<"Project XML contract, source preservation, roundtrip and rejection tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}return 0;}
