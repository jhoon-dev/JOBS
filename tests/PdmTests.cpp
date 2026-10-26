#include "application/Document.h"
#include "calculation/Pdm.h"
#include "calculation/Resources.h"
#include "calculation/Costs.h"
#include "calculation/Tracking.h"
#include "calculation/Reports.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <iostream>
#include <random>
namespace {
void check(bool ok,const char* m){if(!ok)throw std::runtime_error(m);}
template<class F> void rejects(F f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"invalid input accepted");}
jobs::Project sample(){jobs::Project p;p.documentId="pdm";p.name="PDM";p.startDate="2026-10-26";p.calculationProfile=jobs::pdmProfile;p.tasks={{"A","A",480,{}},{"B","B",240,{"A"}}};return p;}
void dates(const jobs::ScheduleResult& r,std::size_t i,const std::string& start,const std::string& finish){check(r.status==jobs::CalculationStatus::Valid,"invalid schedule");if(r.tasks[i].start!=start||r.tasks[i].finish!=finish){std::cerr<<r.tasks[i].start<<" -> "<<r.tasks[i].finish<<'\n';throw std::runtime_error("unexpected PDM dates");}}
QByteArray read(const QString& p){QFile f(p);check(f.open(QIODevice::ReadOnly),"read");return f.readAll();}
void write(const QString& p,const QByteArray& b){QFile f(p);check(f.open(QIODevice::WriteOnly),"write");check(f.write(b)==b.size(),"write all");}
}
int main(int argc,char** argv){QCoreApplication app(argc,argv);try{
    auto p=sample();dates(jobs::calculatePlan(p),1,"2026-10-27 09:00","2026-10-27 14:00");
    p.tasks[1].dependencyRules={{"A","SS",120}};dates(jobs::calculatePlan(p),1,"2026-10-26 11:00","2026-10-26 16:00");
    p.tasks[1].dependencyRules={{"A","FF",0}};dates(jobs::calculatePlan(p),1,"2026-10-26 14:00","2026-10-26 18:00");
    auto mixed=sample();mixed.tasks[1].dependencyRules={{"A","SS",120}};mixed.tasks.push_back({"C","C",120,{"A","B"}});mixed.tasks[2].dependencyRules={{"A","FF",0},{"B","SF",480}};auto mixedResult=jobs::calculatePlan(mixed);dates(mixedResult,2,"2026-10-27 09:00","2026-10-27 11:00");check(mixedResult.tasks[0].critical&&mixedResult.tasks[1].critical&&mixedResult.tasks[2].critical,"SS/SF driving chain lost criticality");
    p.tasks[1].dependencyRules={{"A","SF",480}};dates(jobs::calculatePlan(p),1,"2026-10-26 14:00","2026-10-26 18:00");
    p.tasks[1].dependencyRules={{"A","FS",-120}};dates(jobs::calculatePlan(p),1,"2026-10-26 16:00","2026-10-27 11:00");
    auto milestone=sample();milestone.tasks[1].durationMinutes=0;dates(jobs::calculatePlan(milestone),1,"2026-10-26 18:00","2026-10-26 18:00");
    auto lastDate=sample();lastDate.startDate="9999-12-31";lastDate.tasks.resize(1);lastDate.tasks[0].durationMinutes=60;dates(jobs::calculatePlan(lastDate),0,"9999-12-31 09:00","9999-12-31 10:00");lastDate.calculationProfile="native-fs0-workminutes-v1";dates(jobs::calculatePlan(lastDate),0,"9999-12-31 09:00","9999-12-31 10:00");
    auto horizon=sample();jobs::TaskCalendar continuous;continuous.id="24h";continuous.name="매일";continuous.weekdays={1,2,3,4,5,6,7};continuous.periods={{0,1440}};horizon.calendars={continuous};horizon.tasks[0].calendarId="24h";horizon.tasks[1].calendarId="24h";horizon.tasks[0].durationMinutes=5000000;horizon.tasks[1].durationMinutes=1;horizon.tasks[1].dependencyRules={{"A","FS",5000000}};check(jobs::calculatePlan(horizon).status==jobs::CalculationStatus::Valid,"infeasible late trial invalidated feasible early schedule");
    p.tasks[1].dependencyRules={{"A","FS",180}};p.startDate="2026-10-30";p.holidays={"2026-11-02"};dates(jobs::calculatePlan(p),1,"2026-11-03 13:00","2026-11-03 17:00");
    // Independent Saturday calendar and lag measured in the successor's hours.
    jobs::TaskCalendar sat; sat.id="sat";sat.name="토요일";sat.weekdays={6};sat.periods={{600,840}};p.calendars={sat};p.tasks[1].calendarId="sat";p.tasks[1].dependencyRules={{"A","FS",60}};
    dates(jobs::calculatePlan(p),1,"2026-10-31 11:00","2026-11-07 11:00");
    p.tasks[1].dependencyRules={{"A","FF",0}};dates(jobs::calculatePlan(p),1,"2026-10-31 10:00","2026-10-31 14:00");
    p.calendars[0].holidays={"2026-10-31"};dates(jobs::calculatePlan(p),1,"2026-11-07 10:00","2026-11-07 14:00");
    auto invalid=p;invalid.tasks[1].dependencyRules[0].type="XX";check(jobs::calculatePlan(invalid).status==jobs::CalculationStatus::Conflict,"bad relationship");invalid=p;invalid.tasks[1].calendarId="missing";rejects([&]{jobs::validate(invalid);});
    invalid=p;invalid.calendars[0].periods={{540,720},{700,800}};rejects([&]{jobs::validate(invalid);});invalid=p;invalid.tasks[0].predecessors={"B"};check(jobs::calculatePlan(invalid).status==jobs::CalculationStatus::Conflict,"mixed cycle accepted");
    invalid=p;invalid.tasks[1].durationMinutes.reset();check(jobs::calculatePlan(invalid).status==jobs::CalculationStatus::Incomplete,"missing duration accepted");
    // Verify endpoints, backward feasibility and floats against a minute-grid oracle.
    std::mt19937 random(27);const std::string types[]={"FS","SS","FF","SF"};
    for(int run=0;run<48;++run){auto v=sample();v.tasks[0].durationMinutes=60+random()%300;v.tasks[1].durationMinutes=60+random()%240;auto type=types[run%4];auto lag=std::int64_t(random()%241)-120;if(type!="FS"||lag)v.tasks[1].dependencyRules={{"A",type,lag}};
        auto r=jobs::calculatePlan(v);check(r.status==jobs::CalculationStatus::Valid,"oracle schedule");jobs::WorkCalendar clock(v),a(v,v.tasks[0]),b(v,v.tasks[1]);const auto edge=jobs::dependencies(v.tasks[1])[0];
        const auto earliest=b.offset(r.tasks[1].start); const auto bound=b.shift(edge.type[0]=='F'?r.tasks[0].finish:r.tasks[0].start,edge.lagMinutes);
        std::int64_t oracle=0; for(;;++oracle) {const auto target=edge.type[1]=='S'?b.boundary(oracle,false):b.boundary(oracle+*v.tasks[1].durationMinutes,true);if(target>=bound)break;}check(earliest==oracle,"minute-grid earliest lower bound");
        for(std::size_t i=0;i<2;++i){jobs::WorkCalendar work(v,v.tasks[i]);const auto& row=r.tasks[i];const auto late=work.offset(clock.boundary(row.lateStartMinutes,false));check(late-work.offset(row.start)==row.totalFloatMinutes,"float not task work minutes");jobs::TaskSchedule trial;jobs::pdmDates(clock,work,trial,late,*v.tasks[i].durationMinutes);check(trial.finish<=*r.projectFinish,"late finish exceeds project");if(!i)check(jobs::pdmRequiredStart(b,edge,trial,*v.tasks[1].durationMinutes)<=b.offset(clock.boundary(r.tasks[1].lateStartMinutes,false)),"late relationship broken");}
    }
    QTemporaryDir dir;jobs::JsonProjectStore store;jobs::Document doc(store);doc.create("확장 일정","2026-10-30",{"A","B"});doc.setDuration(0,480);doc.setDuration(1,240);const auto aId=doc.project()->tasks[0].id,bId=doc.project()->tasks[1].id;
    doc.setPredecessors(1,{aId});doc.captureBaseline("기존 FS","PM","승인","2026-10-26T09:00:00Z");const auto oldBaseline=doc.project()->baselines.front();
    sat.id.clear();auto calendarId=doc.putCalendar(sat);doc.setTaskCalendar(1,calendarId);doc.setDependencies(1,{{aId,"SS",60}});const auto advanced=*doc.project();doc.undo();doc.redo();check(*doc.project()!=advanced && doc.project()->tasks==advanced.tasks,"undo/redo calendar relation");
    const auto report=jobs::compareToBaseline(*doc.project(),oldBaseline.id);check(!report.contextChanges.empty() && report.tasks[1].changes.size()>=2,"comparison dropped relation/calendar");jobs::verifyBaseline(oldBaseline);
    doc.captureBaseline("PDM 승인","PM","교대 달력","2026-10-26T09:01:00Z");doc.saveScenario("달력 대안","task-order",20,"2026-10-26T09:01:01Z",doc.project()->documentId,doc.project()->revision);check(doc.project()->scenarios.back().policy=="serial-no-split-pdm-v1"&&jobs::levelResources(jobs::scenarioProject(doc.project()->scenarios.back()),"task-order",20).leveled.status==jobs::CalculationStatus::Valid,"PDM scenario provenance/replay");auto path=dir.filePath("확장.jobs");doc.saveAs(path.toStdString());check(store.load(path.toStdString()).project==*doc.project(),"schema7 roundtrip");const auto bytes=read(path);check(QJsonDocument::fromJson(bytes).object()["schemaVersion"]==QJsonValue(8),"schema8 missing");
    doc.setDependencies(1,{{aId,"FF",-30}});jobs::JsonRecoveryStore recovery(dir.filePath("recovery"));auto session=recovery.newSession();session->checkpoint(doc.recoverySnapshot("2026-10-26T09:02:00Z"));session.reset();auto scan=recovery.scan();auto claim=recovery.claim(scan.candidates.front());check(claim->snapshot.project==*doc.project(),"PDM recovery dropped input");claim.reset();
    // schema6 original remains untouched; no reinterpretation of old snapshots.
    auto old=QJsonDocument::fromJson(bytes).object();
    jobs::Document oldDoc(store);oldDoc.create("legacy6","2026-10-26",{"A"});oldDoc.setDuration(0,480);auto seed=dir.filePath("seed.jobs");oldDoc.saveAs(seed.toStdString());old=QJsonDocument::fromJson(read(seed)).object();old["schemaVersion"]=6;auto legacyPath=dir.filePath("v6.jobs");write(legacyPath,QJsonDocument(old).toJson());const auto legacyBytes=read(legacyPath);jobs::Document loaded(store);loaded.open(legacyPath.toStdString());check(loaded.requiresSaveAs(),"v6 not preserved");loaded.saveAs(dir.filePath("migrated.jobs").toStdString());check(read(legacyPath)==legacyBytes,"v6 overwritten");
    // Real work intervals drive capacity and cost; lunch/weekends never consume units.
    auto money=sample();money.tasks[1].predecessors.clear();money.tasks[0].durationMinutes=480;money.tasks[1].durationMinutes=240;
    jobs::TaskCalendar afternoon;afternoon.id="pm";afternoon.name="오후";afternoon.periods={{780,1020}};money.calendars={afternoon};money.tasks[1].calendarId="pm";
    jobs::Resource r;r.id="R";r.name="공유";r.rates={{"2026-01-01",6000}};money.resources={r};money.assignments={{"A","R",100},{"B","R",100}};money.taskCosts={{"A",0},{"B",0}};
    auto schedule=jobs::calculatePlan(money);auto load=jobs::resourceLoad(money,schedule);check(load.days.size()==1&&load.days[0].demandUnitMinutes==72000&&load.days[0].overloadMinutes==240,"nonworking capacity wrong");
    auto costs=jobs::analyzeCosts(money,{},"budget-remaining-v1");check(costs.plan==72000,"nonworking charged");auto leveled=jobs::levelResources(money,"task-order",10);check(leveled.leveled.status==jobs::CalculationStatus::Valid&&leveled.after.status==jobs::CalculationStatus::Valid,"task-calendar leveling failed");dates(leveled.leveled,1,"2026-10-27 13:00","2026-10-27 17:00");
    auto evmProject=money;evmProject.taskCosts[1].fixedMinor=240;auto budget=jobs::prepareBaseline(evmProject,"달력 예산","PM","승인","2026-10-26T09:00:00Z");budget.id="calendar-budget";budget.version=1;++evmProject.revision;evmProject.baselines={budget};evmProject.statusAt="2026-10-26 15:00";auto evm=jobs::analyzeCosts(evmProject,budget.id,"budget-remaining-v1");check(evm.evm.bac==72240&&evm.evm.pv==42120,"frozen EVM ignored task working hours");
    auto night=money;night.tasks.resize(1);night.assignments.resize(1);night.taskCosts.resize(1);night.taskCosts[0].fixedMinor=120;night.tasks[0].durationMinutes=120;night.tasks[0].calendarId="night";jobs::TaskCalendar nightCal;nightCal.id="night";nightCal.name="자정 경계";nightCal.periods={{0,60},{1380,1440}};night.calendars={nightCal};auto nightCost=jobs::analyzeCosts(night,{},"budget-remaining-v1");check(nightCost.plan==12120&&nightCost.days[0].plan==12120,"midnight fixed-cost day lost one minute");
    auto rounding=night;rounding.tasks[0].durationMinutes=60;rounding.calendars[0].periods={{540,570},{600,630}};rounding.resources[0].rates[0].hourlyMinor=1;rounding.taskCosts[0].fixedMinor=0;check(jobs::analyzeCosts(rounding,{},"budget-remaining-v1").plan==1,"split work periods rounded more than once per assignment/day");
    money.statusAt="2026-10-26 12:00";auto forecast=jobs::calculate(money);dates(forecast,1,"2026-10-26 13:00","2026-10-26 17:00");
    for(const auto& type:types){auto v=money;v.tasks[1].predecessors={"A"};v.tasks[1].dependencyRules.clear();if(type!="FS")v.tasks[1].dependencyRules={{"A",type,60}};v.statusAt.reset();auto level=jobs::levelResources(v,"longest-first",20);check(level.leveled.status==jobs::CalculationStatus::Valid,"mixed PDM leveling");jobs::WorkCalendar work(v,v.tasks[1]);const auto edge=jobs::dependencies(v.tasks[1])[0];const auto& prev=level.leveled.tasks[0];const auto& after=level.leveled.tasks[1];check((type[1]=='S'?after.start:after.finish)>=work.shift(type[0]=='F'?prev.finish:prev.start,edge.lagMinutes),"leveling violated endpoint relation");
        v.statusAt="2026-10-26 12:00";jobs::ProgressEntry actual;actual.id="actual";actual.taskId="A";actual.statusAt=*v.statusAt;actual.recordedAt="2026-10-26T09:00:00Z";actual.reporter="PM";actual.reason="보고";actual.sourceDocumentId=v.documentId;actual.recordedRevision=v.revision;actual.planDurationMinutes=480;actual.actualStart="2026-10-26 09:00";actual.actualDurationMinutes=180;actual.remainingMinutes=300;v.progress={actual};auto f=jobs::calculate(v);check(f.status==jobs::CalculationStatus::Valid&&f.tasks[0].start==*actual.actualStart,"forecast changed actual start");check((type[1]=='S'?f.tasks[1].remainingStart.value():f.tasks[1].finish)>=work.shift(type[0]=='F'?f.tasks[0].finish:f.tasks[0].start,edge.lagMinutes),"forecast ignored PDM endpoint");}
    jobs::ReportOptions options;options.reportId="pdm-contract";options.generatedAt="2026-10-26T09:00:00Z";auto overview=jobs::buildReport(money,options);check(!overview.tables.empty(),"PDM report failed");
    oldDoc.renameTask(0,"old recovery");jobs::JsonRecoveryStore oldRecovery(dir.filePath("old-recovery"));auto oldSession=oldRecovery.newSession();const auto oldId=oldSession->id();oldSession->checkpoint(oldDoc.recoverySnapshot("2026-10-26T09:03:00Z"));oldSession.reset();const auto recordPath=oldRecovery.directory()+"/"+oldId+"/checkpoint-000000001.jobrecovery";auto envelope=QJsonDocument::fromJson(read(recordPath)).object();auto body=envelope["payload"].toObject();body["projectSchema"]=6;envelope["payload"]=body;envelope["sha256"]=QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(body).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());write(recordPath,QJsonDocument(envelope).toJson());auto oldScan=oldRecovery.scan();check(oldScan.candidates.size()==1&&oldScan.candidates.front().valid,"prior recovery projectSchema6 unreadable");auto oldClaim=oldRecovery.claim(oldScan.candidates.front());check(oldClaim->snapshot.project==*oldDoc.project(),"prior recovery changed old input");
    std::cout<<"PASS: PDM endpoints/leads/lags, minute-grid floats, independent calendars, validation/cycles, legacy/new baselines, schema7/recovery/undo, resources/leveling/cost/forecast/report\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
