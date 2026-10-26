#include "calculation/Effort.h"
#include "calculation/Constraints.h"
#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>
namespace jobs {
namespace {
using Minute=std::int64_t;
std::string edgeBound(const TaskExecution& model,const Dependency& d,const TaskSchedule& from) {
    return model.calendar().shift(d.type[0]=='F'?from.finish:from.start,d.lagMinutes);
}
bool edgeFits(const TaskExecution& model,const Dependency& d,const TaskSchedule& from,const TaskSchedule& to) {
    return edgeBound(model,d,from)<=(d.type[1]=='F'?to.finish:to.remainingStart.value_or(to.start));
}
}
ScheduleResult calculateEffort(const Project& p,bool forecast,const std::string& priority,int horizonDays) {
    ScheduleResult out;out.documentId=p.documentId;out.inputRevision=p.revision;out.profile=p.calculationProfile;out.engineVersion=engineFor(p);
    out.isForecast=forecast;out.statusAt=forecast?p.statusAt:std::nullopt;
    out.calculationPolicy=priority.empty()?(forecast?"retained-remaining-effort-v1":"effort-constrained-v1"):levelingPolicyFor(p)+"/"+priority;
    std::string current;
    try {
        WorkCalendar clock(p);const auto n=p.tasks.size();std::map<std::string,std::size_t> ids;
        const auto source=priority.empty()?ScheduleResult{}:calculate(p);
        std::vector<Task> pending;std::vector<std::unique_ptr<TaskExecution>> models;
        for(std::size_t i=0;i<n;++i) {
            const auto& t=p.tasks[i];current=t.id;ids[t.id]=i;
            try{pending.push_back(forecast?pendingTask(p,t):t);}catch(const std::exception& e){out.status=CalculationStatus::Incomplete;out.diagnostics.push_back({usesWork(t)?"MISSING_REMAINING_WORK":"MISSING_REMAINING_DURATION",e.what(),{t.id}});return out;}
            models.push_back(std::make_unique<TaskExecution>(p,pending.back()));
        }
        std::vector<TaskSchedule> rows(n);std::vector<Minute> early(n);std::vector<std::size_t> order;std::vector<bool> done(n);
        std::vector<std::vector<std::pair<std::size_t,Dependency>>> successors(n);
        for(std::size_t i=0;i<n;++i)for(const auto& d:dependencies(p.tasks[i]))successors[ids.at(d.predecessorId)].push_back({i,d});
        std::map<std::string,std::map<Minute,Minute>> reserved;
        std::string finish=p.startDate+" 00:00",horizon;
        if(!priority.empty()){PlanInput standard=p;standard.calculationProfile="native-fs0-workminutes-v1";horizon=WorkCalendar(standard).boundary(Minute(horizonDays)*480,true);}
        auto lower=[&](std::size_t i) {
            const auto& t=p.tasks[i];const auto& model=*models[i];Minute local=0;
            if(forecast)local=model.required(std::max(*p.statusAt,p.startDate+" 00:00"),false);
            if(!priority.empty())for(const auto& row:source.tasks)if(row.taskId==t.id&&!row.completedActual)local=std::max(local,model.required(row.remainingStart.value_or(row.start),false));
            const auto* actual=forecast?progressAsOf(p,t.id):nullptr;
            const bool startConstraint=t.constraintType=="SNET"||t.constraintType=="MSO";
            const bool finishConstraint=t.constraintType=="FNET"||t.constraintType=="MFO";
            if(t.constraintAt&&*t.constraintAt>=p.startDate+" 00:00"&&(finishConstraint||(startConstraint&&(!actual||!actual->actualStart)))) local=std::max(local,model.required(*t.constraintAt,finishConstraint));
            for(const auto& d:dependencies(t))local=std::max(local,model.required(edgeBound(model,d,rows[ids.at(d.predecessorId)]),d.type[1]=='F'));
            return local;
        };
        for(std::size_t count=0;count<n;++count) {
            std::optional<std::size_t> chosen;
            for(std::size_t i=0;i<n;++i)if(!done[i]&&std::all_of(p.tasks[i].predecessors.begin(),p.tasks[i].predecessors.end(),[&](const auto& id){return done[ids.at(id)];})) {
                if(!chosen)chosen=i;
                else if(!priority.empty()&&priority!="task-order") {
                    auto length=[&](std::size_t j){const auto r=models[j]->dates(0);return r.earlyFinishMinutes-r.earlyStartMinutes;};
                    if((priority=="shortest-first"&&length(i)<length(*chosen))||(priority=="longest-first"&&length(i)>length(*chosen)))chosen=i;
                }
            }
            if(!chosen)throw std::runtime_error("공수 일정의 선행관계 순환 또는 참조 오류입니다.");
            const auto i=*chosen;const auto& t=p.tasks[i];current=t.id;const auto& model=*models[i];const auto& cal=model.calendar();
            const auto* actual=forecast?progressAsOf(p,t.id):nullptr;
            auto& row=rows[i];row.taskId=t.id;done[i]=true;order.push_back(i);
            if(actual&&actual->actualFinish){row.start=*actual->actualStart;row.finish=*actual->actualFinish;row.completedActual=true;row.earlyStartMinutes=clock.offset(row.start);row.earlyFinishMinutes=clock.offset(row.finish);}
            else {
                auto local=lower(i);bool unknown=false;
                for(;;) {
                    row=model.dates(local);const auto plannedStart=row.start;
                    for(const auto& d:dependencies(t))if(model.dates(model.required(edgeBound(model,d,rows[ids.at(d.predecessorId)]),d.type[1]=='F')).start==plannedStart)row.drivingPredecessors.push_back(d.predecessorId);
                    if(forecast){row.remainingStart=row.start;if(actual&&actual->actualStart){row.start=*actual->actualStart;row.earlyStartMinutes=clock.offset(row.start);}}
                    if(auto error=timingViolation(t,row)){out.diagnostics.push_back(*error);out.status=CalculationStatus::Conflict;if(forecast)out.tasks=rows;return out;}
                    if(priority.empty())break;
                    if(row.finish>horizon){out.status=unknown?CalculationStatus::Incomplete:CalculationStatus::Conflict;out.diagnostics.push_back({unknown?"LEVEL_UNKNOWN_CAPACITY":"LEVEL_HORIZON_EXHAUSTED","자원·공수·분할·날짜 제약을 만족하는 전체 배치를 탐색 범위에서 찾지 못했습니다.",{t.id}});return out;}
                    const auto spans=model.spans(row);auto next=local;
                    for(const auto& span:spans) {
                        const auto r=std::find_if(p.resources.begin(),p.resources.end(),[&](const auto& v){return v.id==span.resourceId;});const auto& events=reserved[span.resourceId];
                        auto at=span.start;Minute used=0;auto ev=events.begin();while(ev!=events.end()&&ev->first<=at){used+=ev->second;++ev;}
                        while(at<span.finish){auto change=Minute((at/1440+1)*1440);if(ev!=events.end())change=std::min(change,ev->first);
                            auto cap=r->maxUnits;const auto date=clock.boundary(at,false).substr(0,10);for(const auto& d:r->exceptions)if(d.date==date)cap=d.units;
                            if(!cap||used+span.units>*cap){unknown=unknown||!cap;next=std::max(next,std::max(local+1,cal.startAtLeast(clock.boundary(change,false))));break;}
                            at=std::min(change,span.finish);while(ev!=events.end()&&ev->first<=at){used+=ev->second;++ev;}
                        }
                    }
                    if(next==local){for(const auto& s:spans){reserved[s.resourceId][s.start]+=s.units;reserved[s.resourceId][s.finish]-=s.units;}break;}
                    local=next;
                }
                early[i]=cal.offset(row.remainingStart.value_or(row.start));
                if(!priority.empty())for(const auto& before:source.tasks)if(before.taskId==t.id&&row.remainingStart.value_or(row.start)!=before.remainingStart.value_or(before.start))out.diagnostics.push_back({"RESOURCE_DELAY","선행/자원 가용량을 만족하도록 공수·수동 중단을 유지해 지연 배치했습니다.",{t.id}});
            }
            if(auto error=timingViolation(t,row)){out.diagnostics.push_back(*error);out.status=CalculationStatus::Conflict;if(forecast)out.tasks=rows;return out;}
            if(actual&&actual->actualStart)for(const auto& d:dependencies(t)) {
                auto historical=row;historical.remainingStart.reset();
                if((d.type[1]=='S'||actual->actualFinish)&&!edgeFits(model,d,rows[ids.at(d.predecessorId)],historical))out.diagnostics.push_back({"OUT_OF_SEQUENCE","실제 날짜가 선행관계보다 빠릅니다. 실제는 보존하고 잔여 공수에 선행 논리를 적용했습니다.",{d.predecessorId,t.id}});
            }
            finish=std::max(finish,row.finish);
        }
        if(!forecast&&priority.empty()) {
            auto latest=[&](std::size_t i,const std::vector<TaskSchedule>& targets) {
                const auto& model=*models[i];const auto& cal=model.calendar();auto lo=early[i],hi=std::max(lo,cal.offset(finish));
                while(lo<hi){const auto mid=lo+(hi-lo+1)/2;bool fits=false;
                    try{auto trial=model.dates(mid);fits=trial.finish<=finish&&!timingViolation(p.tasks[i],trial);
                        if(fits)for(const auto& [j,d]:successors[i])if(!edgeFits(*models[j],d,trial,targets[j])){fits=false;break;}
                    }catch(const std::exception&){}if(fits)lo=mid;else hi=mid-1;
                }return lo;
            };
            for(auto it=order.rbegin();it!=order.rend();++it)if(p.tasks[*it].constraintType=="ALAP"){const auto i=*it;early[i]=latest(i,rows);rows[i]=models[i]->dates(early[i]);}
            auto late=rows;
            for(auto it=order.rbegin();it!=order.rend();++it){const auto i=*it;auto local=latest(i,late);late[i]=models[i]->dates(local);auto& r=rows[i];r.lateStartMinutes=late[i].earlyStartMinutes;r.lateFinishMinutes=late[i].earlyFinishMinutes;r.totalFloatMinutes=std::max(Minute{0},models[i]->calendar().offset(late[i].start)-early[i]);const auto free=latest(i,rows);r.freeFloatMinutes=std::max(Minute{0},models[i]->calendar().offset(models[i]->dates(free).start)-early[i]);r.critical=r.totalFloatMinutes==0;}
            for(std::size_t i=0;i<n;++i){rows[i].drivingPredecessors.clear();for(const auto& d:dependencies(p.tasks[i]))if(models[i]->dates(models[i]->required(edgeBound(*models[i],d,rows[ids.at(d.predecessorId)]),d.type[1]=='F')).start==rows[i].start)rows[i].drivingPredecessors.push_back(d.predecessorId);}
        }
        out.tasks=std::move(rows);out.projectFinish=finish;out.status=CalculationStatus::Valid;auto deadlines=deadlineDiagnostics(p,out.tasks);out.diagnostics.insert(out.diagnostics.end(),deadlines.begin(),deadlines.end());
        out.diagnostics.push_back({"EFFORT_POLICY","공수 = 작업·자원 달력 교집합의 배정률 적분. 수동 중단은 공수/부하/원가에서 제외합니다. 1분 완료 경계의 마지막 분은 자원 ID 순으로 잔여를 배분하여 입력 공수를 정확히 보존합니다. 단가 반올림은 자원별/일별 1회입니다.",{}});
        if(forecast||!priority.empty())out.diagnostics.push_back({"EFFORT_FORECAST_LEVEL_POLICY","실제 날짜는 보존하며 잔여만 배치합니다. 예측·평준화의 여유/임계 및 ALAP 재배치는 미산정입니다. 평준화는 수동 분할을 유지하는 직렬 지연 방식입니다.",{}});
    }catch(const std::exception& e){out.status=CalculationStatus::Conflict;out.tasks.clear();out.projectFinish.reset();out.diagnostics.push_back({"EFFORT_AVAILABILITY",e.what(),current.empty()?std::vector<std::string>{}:std::vector<std::string>{current}});}
    return out;
}
LevelingResult levelEffort(const Project& p,const std::string& priority,int horizon) {
    LevelingResult out;out.priority=priority;out.horizonWorkDays=horizon;out.original=calculate(p);out.before=resourceLoad(p,out.original);out.leveled=out.original;
    if(out.original.status!=CalculationStatus::Valid)return out;
    out.leveled=calculateEffort(p,p.statusAt.has_value(),priority,horizon);out.diagnostics=out.leveled.diagnostics;out.after=resourceLoad(p,out.leveled);return out;
}
}
