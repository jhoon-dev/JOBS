#include "calculation/Constraints.h"
#include <algorithm>
#include <map>
#include <unordered_map>
#include <stdexcept>
namespace jobs {
ConstraintWindow constraintWindow(const PlanInput& p,const Task& t,const WorkCalendar& cal,std::int64_t duration) {
    ConstraintWindow w;if(!t.constraintAt)return w;const auto& at=*t.constraintAt;const auto& type=t.constraintType;
    const bool start=type=="SNET"||type=="SNLT"||type=="MSO";
    const bool fixed=type=="MSO"||type=="MFO";
    const bool upper=type=="SNLT"||type=="FNLT";
    if(at<p.startDate+" 00:00") {if(fixed||upper)w.upper=-1;return w;}
    std::int64_t value;
    try {value=cal.offset(at);} catch(const std::exception&) {if(upper){w.upper=cal.maxOffset()-duration;return w;}throw;}
    if(fixed) {
        auto local=value-(start?0:duration);
        if(local<0||cal.boundary(value,!start||duration==0)!=at)throw std::runtime_error("고정 시각이 작업 달력의 유효한 시작/완료 경계가 아닙니다: "+at);
        w.lower=local;w.upper=local;
    } else if(upper) {
        auto local=value-(start?0:duration);
        if(local>=0 && cal.boundary(start?local:local+duration,!start||duration==0)>at)--local;
        w.upper=local;
    } else {
        w.lower=start?(duration?cal.startAtLeast(at):cal.finishAtLeast(at)):std::max(std::int64_t{0},cal.finishAtLeast(at)-duration);
    }
    return w;
}
std::optional<Diagnostic> timingViolation(const Task& t,const TaskSchedule& row) {
    if(!t.constraintAt)return {};
    const auto& type=t.constraintType;
    const bool start=type=="SNET"||type=="SNLT"||type=="MSO";const auto& actual=start?row.start:row.finish;const auto& bound=*t.constraintAt;
    const bool bad=type=="MSO"||type=="MFO"?actual!=bound:type=="SNET"||type=="FNET"?actual<bound:actual>bound;
    if(!bad)return {};
    auto ids=row.drivingPredecessors;ids.insert(ids.begin(),t.id);
    return Diagnostic{"CONSTRAINT_CONFLICT",t.name+" · "+constraintName(type)+" ("+type+") "+bound+" · 선행관계/달력/상태일을 반영한 "+(start?"시작 ":"완료 ")+actual+" · 기간·제약 날짜·선행 ID·작업 달력을 확인하세요.",std::move(ids)};
}
std::optional<std::int64_t> deadlineSlack(const PlanInput& p,const Task& t,const std::string& finish) {
    if(!t.deadlineAt)return {};
    try {WorkCalendar cal(p,t);const auto due=*t.deadlineAt<p.startDate+" 00:00"?0:cal.offset(*t.deadlineAt);return due-cal.offset(finish);}catch(const std::exception&){return {};}
}
std::vector<Diagnostic> deadlineDiagnostics(const PlanInput& p,const std::vector<TaskSchedule>& rows) {
    std::vector<Diagnostic> out;
    for(const auto& t:p.tasks)if(t.deadlineAt) {
        auto it=std::find_if(rows.begin(),rows.end(),[&](const auto& r){return r.taskId==t.id;});if(it==rows.end()||it->finish<=*t.deadlineAt)continue;
        const auto slack=deadlineSlack(p,t,it->finish);
        out.push_back({"DEADLINE_MISSED",t.name+" · 마감 "+*t.deadlineAt+" < "+(it->completedActual?"실제 완료 ":"완료 ")+it->finish+" · 마감 여유 "+(slack?std::to_string(*slack)+" 근무분":"계산 범위 밖")+" · 마감일은 경고 기준이며 날짜를 강제로 이동하지 않습니다.",{t.id}});
    }
    return out;
}
ScheduleResult calculateConstrainedPlan(const Project& p,const std::vector<std::size_t>& order) {
    ScheduleResult out;out.documentId=p.documentId;out.inputRevision=p.revision;out.profile=p.calculationProfile;out.engineVersion=engineFor(p);out.calculationPolicy="constrained-asap-alap-v1";
    std::string currentTask;
    try {
        WorkCalendar clock(p);std::map<std::string,WorkCalendar> calendars;std::unordered_map<std::string,std::size_t> ids;
        for(std::size_t i=0;i<p.tasks.size();++i){ids[p.tasks[i].id]=i;if(!calendars.contains(p.tasks[i].calendarId))calendars.emplace(p.tasks[i].calendarId,WorkCalendar(p,p.tasks[i]));}
        const auto n=p.tasks.size();std::vector<TaskSchedule> rows(n);std::vector<ConstraintWindow> bounds(n);std::vector<std::int64_t> early(n),late(n);
        std::vector<std::vector<std::pair<std::size_t,Dependency>>> next(n);std::string finish=p.startDate+" 00:00";
        std::size_t completed=0;
        for(auto i:order) {
            calculationProgress(CalculationStage::Forward,completed++,n);
            currentTask=p.tasks[i].id;
            const auto& t=p.tasks[i];const auto& cal=calendars.at(t.calendarId);auto& row=rows[i];row.taskId=t.id;
            try {bounds[i]=constraintWindow(p,t,cal,*t.durationMinutes);}catch(const std::exception& e){out.diagnostics.push_back({"CONSTRAINT_CALENDAR",t.name+" · "+constraintName(t.constraintType)+" · "+e.what(),{t.id}});continue;}
            early[i]=bounds[i].lower;
            for(const auto& edge:dependencies(t)){const auto prev=ids.at(edge.predecessorId);if(rows[prev].start.empty())continue;early[i]=std::max(early[i],pdmRequiredStart(cal,edge,rows[prev],*t.durationMinutes));next[prev].push_back({i,edge});}
            pdmDates(clock,cal,row,early[i],*t.durationMinutes);
            for(const auto& edge:dependencies(t))if(!rows[ids.at(edge.predecessorId)].start.empty() && pdmRequiredStart(cal,edge,rows[ids.at(edge.predecessorId)],*t.durationMinutes)==early[i])row.drivingPredecessors.push_back(edge.predecessorId);
            if(auto error=timingViolation(t,row))out.diagnostics.push_back(*error);
            finish=std::max(finish,row.finish);
        }
        if(!out.diagnostics.empty()){out.status=CalculationStatus::Conflict;return out;}
        auto latest=[&](std::size_t i,const std::vector<std::int64_t>& targets) {
            currentTask=p.tasks[i].id;
            const auto& t=p.tasks[i];const auto& cal=calendars.at(t.calendarId);auto cap=cal.offset(finish)-*t.durationMinutes;if(bounds[i].upper)cap=std::min(cap,*bounds[i].upper);
            auto lo=early[i],hi=std::max(early[i],cap);
            while(lo<hi){const auto mid=lo+(hi-lo+1)/2;TaskSchedule trial;pdmDates(clock,cal,trial,mid,*t.durationMinutes);bool fits=true;
                try{for(const auto& [j,edge]:next[i])if(pdmRequiredStart(calendars.at(p.tasks[j].calendarId),edge,trial,*p.tasks[j].durationMinutes)>targets[j]){fits=false;break;}}catch(const std::exception&){fits=false;}
                if(fits)lo=mid;else hi=mid-1;
            }return lo;
        };
        // Reverse order places ALAP chains as late as their already placed successors permit.
        for(auto it=order.rbegin();it!=order.rend();++it)if(p.tasks[*it].constraintType=="ALAP"){
            const auto i=*it;early[i]=latest(i,early);pdmDates(clock,calendars.at(p.tasks[i].calendarId),rows[i],early[i],*p.tasks[i].durationMinutes);
        }
        for(auto it=order.rbegin();it!=order.rend();++it){calculationProgress(CalculationStage::Backward,std::uint64_t(it-order.rbegin()),n);const auto i=*it;auto& row=rows[i];late[i]=latest(i,late);TaskSchedule trial;pdmDates(clock,calendars.at(p.tasks[i].calendarId),trial,late[i],*p.tasks[i].durationMinutes);row.lateStartMinutes=trial.earlyStartMinutes;row.lateFinishMinutes=trial.earlyFinishMinutes;row.totalFloatMinutes=late[i]-early[i];row.freeFloatMinutes=latest(i,early)-early[i];row.critical=row.totalFloatMinutes==0;
            row.drivingPredecessors.clear();for(const auto& edge:dependencies(p.tasks[i]))if(pdmRequiredStart(calendars.at(p.tasks[i].calendarId),edge,rows[ids.at(edge.predecessorId)],*p.tasks[i].durationMinutes)==early[i])row.drivingPredecessors.push_back(edge.predecessorId);
        }
        out.tasks=std::move(rows);out.projectFinish=finish;out.status=CalculationStatus::Valid;out.diagnostics=deadlineDiagnostics(p,out.tasks);
        out.diagnostics.push_back({"CONSTRAINT_POLICY","선행관계·작업 달력·날짜 제약을 함께 적용합니다. ALAP은 현재 프로젝트 완료일 안에서 후행 작업을 늦추지 않는 가장 늦은 배치입니다. 마감일은 경고/마감 여유이며 CPM 여유를 변경하지 않습니다.",{}});
    }catch(const std::exception& e){out.status=CalculationStatus::Conflict;out.tasks.clear();out.projectFinish.reset();out.diagnostics.push_back({"CONSTRAINT_HORIZON",e.what(),currentTask.empty()?std::vector<std::string>{}:std::vector<std::string>{currentTask}});}
    return out;
}
}
