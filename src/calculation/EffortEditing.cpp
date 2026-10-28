#include "calculation/Effort.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jobs {
namespace {
void scaleAssignments(Project& p,std::size_t index,std::int64_t target,std::int64_t duration,bool planOnly=false) {
    calculationCheckpoint();
    auto t=p.tasks.at(index);t.taskType="fixed-duration";t.workDriven=false;t.workUnitMinutes.reset();t.durationMinutes=duration;
    Project trial; if(planOnly){static_cast<PlanInput&>(trial)=p;trial.documentId=p.documentId;trial.revision=p.revision;}else trial=p;trial.tasks[index]=t;auto plan=calculatePlan(trial);std::int64_t local=0;TaskExecution model(p,t);
    if(plan.status!=CalculationStatus::Valid)throw std::runtime_error("배정률 역산에는 입력 기간을 적용한 유효한 전체 계획이 필요합니다. 선행관계·날짜 제약·기간 입력을 확인하세요.");
    for(const auto& row:plan.tasks)if(row.taskId==t.id)local=model.calendar().offset(row.start);
    const auto work=model.work(model.dates(local));
    if(target&&work==0)throw std::runtime_error("공수/기간으로 배정률을 조정하려면 양수 기간과 겹치는 자원 달력·배정이 필요합니다.");
    for(auto& a:p.assignments){calculationCheckpoint();if(a.taskId==t.id){const auto units=work?std::llround(static_cast<long double>(a.units)*target/work):0;if(units>100000)throw std::runtime_error("필요 배정률이 100,000%를 초과합니다.");a.units=int(target?std::max(1LL,units):0);}}
    std::erase_if(p.assignments,[&](const auto& a){return a.taskId==t.id&&!a.units;});
}
}
void changeDuration(Project& p,std::size_t index,std::optional<std::int64_t> duration,bool planOnly) {
    auto& t=p.tasks.at(index);
    if(isEffort(p)&&t.taskType=="fixed-work") {
        if(!duration||(*duration==0&&t.workUnitMinutes.value_or(0)>0))throw std::runtime_error("고정 공수의 기간 편집에는 양수 기간이 필요합니다.");
        scaleAssignments(p,index,t.workUnitMinutes.value(),*duration,planOnly);
    } else if(t.taskType=="fixed-units"){t.workDriven=false;t.workUnitMinutes.reset();}
    t.durationMinutes=duration;
}
void changeWork(Project& p,std::size_t index,std::int64_t work) {
    if(work<0||work>maxWork)throw std::runtime_error("입력 공수 범위를 초과했습니다.");
    p.calculationProfile=effortProfile;auto& t=p.tasks.at(index);
    if(t.taskType=="fixed-duration") {
        if(!t.durationMinutes)throw std::runtime_error("고정 기간 작업의 기간을 먼저 입력하세요.");
        scaleAssignments(p,index,work,*t.durationMinutes);
    } else {t.workUnitMinutes=work;t.workDriven=true;}
}
}
