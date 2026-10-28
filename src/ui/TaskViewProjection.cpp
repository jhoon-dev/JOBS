#include "ui/TaskViewProjection.h"
#include "domain/Wbs.h"
#include "calculation/WorkCalendar.h"
#include <algorithm>
#include <unordered_set>
namespace jobs {
TaskViewProjection projectTaskView(const Project& p,const ScheduleResult* result,const TaskViewSpec& spec) {
    TaskViewProjection out; out.rows=wbsTaskRows(p,spec.wbs);out.scopeCount=out.rows.size();
    const auto query=spec.query.trimmed();
    const bool valid=result&&result->status==CalculationStatus::Valid;
    const bool plan=valid&&!result->isForecast;
    std::unordered_map<std::string,const TaskSchedule*> dates;
    std::unordered_set<std::string> assigned;
    if(spec.filter||spec.sort>=3){
        if(valid)for(const auto& row:result->tasks)dates[row.taskId]=&row;
        if(spec.filter==3)for(const auto& a:p.assignments)assigned.insert(a.taskId);
    }
    std::erase_if(out.rows,[&](std::size_t i){
        calculationCheckpoint();const auto& t=p.tasks[i];
        if(!query.isEmpty()&&!QString::fromUtf8(t.name).contains(query,Qt::CaseInsensitive)&&!QString::fromUtf8(t.id).contains(query,Qt::CaseInsensitive))return true;
        if(spec.filter==1)return !plan||!dates.contains(t.id)||!dates.at(t.id)->critical;
        if(spec.filter==2)return !(usesWork(t)?t.workUnitMinutes==0:t.durationMinutes==0)&&!(dates.contains(t.id)&&dates.at(t.id)->start==dates.at(t.id)->finish);
        return spec.filter==3&&assigned.contains(t.id);
    });
    if(spec.sort){
        struct Key{QString name;std::optional<std::int64_t> duration;std::optional<std::string> start;};
        std::vector<Key> keys(p.tasks.size());
        for(auto i:out.rows){
            calculationCheckpoint();const auto& t=p.tasks[i];auto& k=keys[i];k.name=QString::fromUtf8(t.name);k.duration=t.durationMinutes;
            if(dates.contains(t.id)){const auto& r=*dates.at(t.id);k.start=r.start;if(usesWork(t)&&plan){WorkCalendar cal(p,t);k.duration=cal.offset(r.finish)-cal.offset(r.start);}}
        }
        std::size_t comparisons=0;
        std::stable_sort(out.rows.begin(),out.rows.end(),[&](std::size_t a,std::size_t b){
            if(++comparisons%1024==0)calculationCheckpoint();
            const auto& x=keys[a];const auto& y=keys[b];const int sort=spec.sort;
            if(sort<=2){const auto c=QString::compare(x.name,y.name,Qt::CaseInsensitive);return sort==1?c<0:c>0;}
            if(sort<=4){if(bool(x.duration)!=bool(y.duration))return bool(x.duration);if(!x.duration)return false;return sort==3?*x.duration<*y.duration:*x.duration>*y.duration;}
            if(bool(x.start)!=bool(y.start))return bool(x.start);
            if(!x.start)return false;
            return sort==5?*x.start<*y.start:*x.start>*y.start;
        });
    }
    return out;
}
namespace {
void appendRange(std::vector<std::pair<int,int>>& ranges,int row){
    if(!ranges.empty()&&ranges.back().second==row-1)ranges.back().second=row;
    else ranges.emplace_back(row,row);
}
}
void prepareTaskView(BulkView& view,const Project& before,const Project& after,const TaskViewSpec& spec,
                     std::vector<std::size_t> previousRows,std::string token,std::uint64_t generation) {
    auto projected=projectTaskView(after,nullptr,spec);
    view.projected=true;view.viewToken=std::move(token);view.sourceCalculationGeneration=generation;
    view.scopeTaskCount=projected.scopeCount;view.summary=summarizeWbs(after,nullptr,spec.wbs);
    view.previousRows=std::move(previousRows);view.rows=std::move(projected.rows);view.positions.clear();
    std::unordered_set<std::string> oldIds,newIds;
    std::vector<std::string> survivingOld;
    for(auto row:view.previousRows){calculationCheckpoint();oldIds.insert(before.tasks.at(row).id);}
    for(auto row:view.rows)newIds.insert(after.tasks[row].id);
    for(int row=0;row<int(view.previousRows.size());++row){
        calculationCheckpoint();const auto& id=before.tasks[view.previousRows[std::size_t(row)]].id;
        if(!newIds.contains(id))appendRange(view.removals,row);else survivingOld.push_back(id);
    }
    std::unordered_set<std::size_t> changed;
    for(const auto& [first,last]:view.changedRanges)for(int row=first;row<=last;++row)changed.insert(std::size_t(row));
    view.changedRanges.clear();
    for(int row=0;row<int(view.rows.size());++row){
        calculationCheckpoint();const auto source=view.rows[std::size_t(row)];const auto& id=after.tasks[source].id;
        view.positions.emplace(source,row);
        if(!oldIds.contains(id))appendRange(view.insertions,row);
        else{
            const auto position=view.survivorRows.size();
            if(survivingOld[position]!=id)view.reordered=true;
            view.survivorPositions.emplace(id,int(position));view.survivorRows.push_back(source);
        }
        if(changed.contains(source))appendRange(view.changedRanges,row);
    }
    for(const auto& id:survivingOld)view.survivorOldOrderRows.push_back(std::size_t(view.numbers.at(id)-1));
    calculationCheckpoint();
}
}
