#include "calculation/Effort.h"
#include "calculation/Constraints.h"
#include <algorithm>
#include <map>
#include <stdexcept>
#include <numeric>
#include <tuple>
namespace jobs {
using Minute=std::int64_t;
TaskExecution::TaskExecution(const PlanInput& p,const Task& t):task_(t),clock_(p),calendar_(p,t) {
    const auto taskTimes=calendar_.civilIntervals();std::map<Minute,Minute> changes;
    for(const auto& a:p.assignments)if(a.taskId==t.id) {
        const auto r=std::find_if(p.resources.begin(),p.resources.end(),[&](const auto& v){return v.id==a.resourceId;});
        if(r==p.resources.end())throw std::runtime_error("배정 자원이 없습니다.");
        Task resourceTask;resourceTask.calendarId=r->calendarId;WorkCalendar resourceCalendar(p,resourceTask);
        const auto resourceTimes=resourceCalendar.civilIntervals();std::size_t i=0,j=0;
        while(i<taskTimes.size()&&j<resourceTimes.size()) {
            const auto s=std::max(taskTimes[i].first,resourceTimes[j].first),e=std::min(taskTimes[i].second,resourceTimes[j].second);
            if(s<e){availability_.push_back({a.resourceId,s,e,a.units});changes[s]+=a.units;changes[e]-=a.units;}
            if(taskTimes[i].second<resourceTimes[j].second)++i;else ++j;
        }
    }
    Minute units=0,prefix=0,last=0;
    for(const auto& [at,delta]:changes){if(units&&at>last){weighted_.push_back({last,at,units,prefix});prefix+=(at-last)*units;}units+=delta;last=at;}
}
Minute TaskExecution::integrated(Minute civil) const {
    auto it=std::upper_bound(weighted_.begin(),weighted_.end(),civil,[](Minute n,const auto& w){return n<w.start;});
    if(it==weighted_.begin())return 0;
    --it;return it->prefix+std::clamp(civil-it->start,Minute{0},it->finish-it->start)*it->units;
}
TaskSchedule TaskExecution::dates(Minute local) const {
    TaskSchedule row;row.taskId=task_.id;
    const auto amount=usesWork(task_)?task_.workUnitMinutes.value():task_.durationMinutes.value();
    row.start=calendar_.boundary(local,amount==0);row.finish=row.start;
    if(amount>0&&usesWork(task_)) {
        const auto first=clock_.offset(row.start);
        auto it=std::lower_bound(weighted_.begin(),weighted_.end(),first,[](const auto& w,Minute n){return w.finish<=n;});
        if(it==weighted_.end())throw std::runtime_error("공수 작업의 배정/자원·작업 달력 교집합에 가용 시간이 없습니다.");
        const auto begin=std::max(first,it->start),target=integrated(begin)+amount;
        row.start=clock_.boundary(begin,false);
        auto end=std::lower_bound(weighted_.begin(),weighted_.end(),target,[](const auto& w,Minute n){return w.prefix+(w.finish-w.start)*w.units<n;});
        if(end==weighted_.end())throw std::runtime_error("20년 계산 범위 안에 요구 공수를 완료할 가용 시간이 부족합니다.");
        const auto finish=end->start+(target-end->prefix+end->units-1)/end->units;
        row.finish=clock_.boundary(finish,true);
    } else if(amount>0) row.finish=calendar_.boundary(local+amount,true);
    row.earlyStartMinutes=clock_.offset(row.start);row.earlyFinishMinutes=clock_.offset(row.finish);return row;
}
Minute TaskExecution::required(const std::string& bound,bool finish) const {
    if(!finish)return (usesWork(task_)?task_.workUnitMinutes.value()==0:task_.durationMinutes.value()==0)?calendar_.finishAtLeast(bound):calendar_.startAtLeast(bound);
    // Finish-driven edges need an inverse of the work integral, not F - nominal D.
    Minute lo=0,hi=calendar_.maxOffset();
    while(lo<hi){auto mid=lo+(hi-lo)/2;bool reached=true;try{reached=dates(mid).finish>=bound;}catch(const std::exception&){}if(reached)hi=mid;else lo=mid+1;}
    return lo;
}
std::vector<WorkSpan> TaskExecution::spans(const TaskSchedule& row) const {
    if(row.completedActual)return {};
    const auto begin=clock_.offset(row.remainingStart.value_or(row.start)),end=clock_.offset(row.finish);
    std::vector<WorkSpan> out;
    for(const auto& a:availability_)if(a.start<end&&a.finish>begin)out.push_back({a.resourceId,std::max(begin,a.start),std::min(end,a.finish),a.units});
    if(usesWork(task_)&&end>begin) {
        const auto target=*task_.workUnitMinutes;
        const auto before=integrated(end-1)-integrated(begin),last=target-before,total=integrated(end)-integrated(end-1);
        if(last<0||last>total)throw std::runtime_error("공수 일정 결과와 입력 공수가 일치하지 않습니다.");
        if(last<total) {
            std::vector<WorkSpan> kept,tail;Minute allocated=0;
            for(const auto& a:out) {
                if(a.finish<end){kept.push_back(a);continue;}
                if(a.start<end-1)kept.push_back({a.resourceId,a.start,end-1,a.units});
                const int units=int(Minute(a.units)*last/total);allocated+=units;tail.push_back({a.resourceId,end-1,end,units});
            }
            std::sort(tail.begin(),tail.end(),[](const auto& a,const auto& b){return a.resourceId<b.resourceId;});
            for(auto& a:tail)if(allocated<last){++a.units;++allocated;}
            for(const auto& a:tail)if(a.units)kept.push_back(a);
            out=std::move(kept);
        }
    }
    std::sort(out.begin(),out.end(),[](const auto& a,const auto& b){return std::tie(a.resourceId,a.start)<std::tie(b.resourceId,b.start);});return out;
}
Minute TaskExecution::work(const TaskSchedule& row) const { Minute total=0;for(const auto& a:spans(row))total+=(a.finish-a.start)*a.units;return total; }
Minute TaskExecution::duration(const TaskSchedule& row) const { return calendar_.offset(row.finish)-calendar_.offset(row.remainingStart.value_or(row.start)); }
Task pendingTask(const Project& p,const Task& t) {
    auto pending=t;if(!p.statusAt)return pending;const auto* e=progressAsOf(p,t.id);if(!e)return pending;
    if(usesWork(t)) {
        if(e->actualFinish)pending.workUnitMinutes=0;
        else if(e->remainingWorkUnitMinutes)pending.workUnitMinutes=e->remainingWorkUnitMinutes;
        else throw std::runtime_error("공수 작업에는 명시적인 잔여 공수 보고가 필요합니다: "+t.name);
    } else {
        if(!e->actualFinish&&e->remainingMinutes==0&&e->remainingWorkUnitMinutes)throw std::runtime_error("기간 기준으로 변경한 작업에는 새 잔여 기간 보고가 필요합니다: "+t.name);
        pending.durationMinutes=e->remainingMinutes;
    }
    return pending;
}
std::vector<WorkSpan> executionSpans(const PlanInput& p,const Task& t,const TaskSchedule& row) { return TaskExecution(p,t).spans(row); }
}
