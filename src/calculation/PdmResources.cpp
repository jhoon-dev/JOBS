#include "calculation/PdmResources.h"
#include "calculation/Pdm.h"
#include <algorithm>
#include <map>
#include <unordered_map>
namespace jobs {
LevelingResult levelPdmResources(const Project& p,const std::string& priority,int horizonWorkDays) {
    LevelingResult out; out.priority=priority; out.horizonWorkDays=horizonWorkDays; out.original=calculate(p); out.before=resourceLoad(p,out.original); out.leveled=out.original;
    out.leveled.calculationPolicy="serial-no-split-pdm-v1/"+priority;
    if (out.original.status!=CalculationStatus::Valid) return out;
    try {
        WorkCalendar clock(p); PlanInput standard=p; standard.calculationProfile="native-fs0-workminutes-v1"; WorkCalendar horizonCalendar(standard);
        const auto horizon=horizonCalendar.boundary(std::int64_t(horizonWorkDays)*480,true);
        std::map<std::string,WorkCalendar> calendars; std::unordered_map<std::string,std::size_t> ids;
        for (std::size_t i=0;i<p.tasks.size();++i) { ids[p.tasks[i].id]=i; if (!calendars.contains(p.tasks[i].calendarId)) calendars.emplace(p.tasks[i].calendarId,WorkCalendar(p,p.tasks[i])); }
        auto rows=out.original.tasks; std::vector<bool> done(p.tasks.size());
        std::map<std::string,std::map<std::int64_t,std::int64_t>> reserved;
        auto remaining=[&](std::size_t i) { const auto* e=out.original.isForecast?progressAsOf(p,p.tasks[i].id):nullptr; return e?e->remainingMinutes:*p.tasks[i].durationMinutes; };
        for (std::size_t count=0;count<p.tasks.size();++count) {
            std::optional<std::size_t> chosen;
            for (std::size_t i=0;i<p.tasks.size();++i) {
                if (done[i] || std::any_of(p.tasks[i].predecessors.begin(),p.tasks[i].predecessors.end(),[&](const auto& id){return !done[ids.at(id)];})) continue;
                if (!chosen || (priority=="shortest-first" && remaining(i)<remaining(*chosen)) || (priority=="longest-first" && remaining(i)>remaining(*chosen))) chosen=i;
            }
            if (!chosen) throw std::runtime_error("평준화 관계에 순환이 있습니다.");
            const auto i=*chosen; const auto& task=p.tasks[i]; auto& row=rows[i]; done[i]=true;
            if (row.completedActual) continue;
            const auto& cal=calendars.at(task.calendarId); const auto duration=remaining(i); const auto originalStart=row.remainingStart.value_or(row.start);
            auto local=duration?cal.startAtLeast(originalStart):cal.finishAtLeast(originalStart);
            for (const auto& d:dependencies(task)) local=std::max(local,pdmRequiredStart(cal,d,rows[ids.at(d.predecessorId)],duration));
            bool found=false, unknown=false; TaskSchedule trial; std::vector<std::pair<std::string,std::string>> segments;
            for (;;) {
                pdmDates(clock,cal,trial,local,duration); if (trial.finish>horizon) break;
                segments=cal.segments(trial.start,trial.finish); auto next=local;
                for (const auto& a:p.assignments) if (a.taskId==task.id) {
                    const auto& r=*std::find_if(p.resources.begin(),p.resources.end(),[&](const auto& v){return v.id==a.resourceId;}); const auto& events=reserved[r.id];
                    for (const auto& [begin,end]:segments) {
                        auto at=clock.offset(begin); const auto finish=clock.offset(end); std::int64_t used=0; auto ev=events.begin();
                        while (ev!=events.end() && ev->first<=at) { used+=ev->second; ++ev; }
                        while (at<finish) {
                            auto change=std::int64_t((at/1440+1)*1440); if (ev!=events.end()) change=std::min(change,ev->first);
                            const auto date=clock.boundary(at,false).substr(0,10); auto cap=r.maxUnits; for (const auto& d:r.exceptions) if (d.date==date) cap=d.units;
                            if (!cap || used+a.units>*cap) { unknown=unknown||!cap; next=std::max(next,std::max(local+1,cal.startAtLeast(clock.boundary(change,false)))); break; }
                            at=std::min(change,finish); while (ev!=events.end() && ev->first<=at) { used+=ev->second; ++ev; }
                        }
                        if (next!=local) break;
                    }
                }
                if (next==local) { found=true; break; } local=next;
            }
            if (!found) { out.leveled.status=unknown?CalculationStatus::Incomplete:CalculationStatus::Conflict; out.leveled.tasks.clear(); out.leveled.projectFinish.reset(); out.diagnostics.push_back({unknown?"LEVEL_UNKNOWN_CAPACITY":"LEVEL_HORIZON_EXHAUSTED","PDM 관계·작업 달력·자원 가용량을 만족하는 배치를 탐색 범위에 찾지 못했습니다. 부분 결과를 확정하지 않습니다.",{task.id}}); return out; }
            for (const auto& a:p.assignments) if (a.taskId==task.id) for (const auto& [begin,end]:segments) { reserved[a.resourceId][clock.offset(begin)]+=a.units; reserved[a.resourceId][clock.offset(end)]-=a.units; }
            const auto* entry=out.original.isForecast?progressAsOf(p,task.id):nullptr;
            if (entry && entry->actualStart) { trial.start=*entry->actualStart; trial.earlyStartMinutes=clock.offset(trial.start); }
            trial.taskId=task.id; if (out.original.isForecast) trial.remainingStart=cal.boundary(local,duration==0);
            row=trial;
            if (row.remainingStart.value_or(row.start)!=originalStart) out.diagnostics.push_back({"RESOURCE_DELAY","선행·자원 제약으로 잔여/계획 작업을 작업 달력의 다음 가용 구간으로 지연했습니다.",{task.id}});
        }
        out.leveled.tasks=std::move(rows); out.leveled.projectFinish.reset();
        for (const auto& row:out.leveled.tasks) if (!out.leveled.projectFinish || row.finish>*out.leveled.projectFinish) out.leveled.projectFinish=row.finish;
        out.leveled.status=CalculationStatus::Valid; out.after=resourceLoad(p,out.leveled);
        out.diagnostics.push_back({"LEVEL_POLICY","직렬 우선순위 · 작업 달력의 근무 구간만 자원 예약 · 비근무 중단은 허용, 자원에 따른 작업 분할 없음 · 여유/임계 미산정",{}});
    } catch (const std::exception& e) { out.leveled.status=CalculationStatus::Conflict; out.leveled.tasks.clear(); out.leveled.projectFinish.reset(); out.diagnostics.push_back({"LEVEL_HORIZON_EXHAUSTED",e.what(),{}}); }
    return out;
}
}
