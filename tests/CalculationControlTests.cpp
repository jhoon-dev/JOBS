#include "calculation/Scheduler.h"
#include "calculation/Effort.h"
#include <chrono>
#include <future>
#include <iostream>
#include <thread>
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
jobs::Project fixture(){jobs::Project p;p.documentId="cancel-contract";p.name="Cancel";p.startDate="2026-10-26";p.calculationProfile=jobs::effortProfile;
    jobs::Resource r;r.id="R";r.name="Engineer";p.resources={r};
    for(int i=0;i<20000;++i){jobs::Task t{"T"+std::to_string(i),"Work",60,{}};t.taskType="fixed-work";t.workDriven=true;t.workUnitMinutes=6000;p.tasks.push_back(t);p.assignments.push_back({t.id,"R",100});}return p;}
}
int main(){try{
    const auto p=fixture(),before=p;auto control=std::make_shared<jobs::CalculationControl>();control->cancel();auto result=jobs::calculate(p,control);
    check(result.status==jobs::CalculationStatus::Cancelled&&result.tasks.empty()&&!result.projectFinish,"pre-cancel exposed a schedule");
    // Observe a real worker inside calendar preparation and the backward pass.
    for(const auto stage:{jobs::CalculationStage::Calendars,jobs::CalculationStage::Backward}){
        control=std::make_shared<jobs::CalculationControl>();auto worker=std::async(std::launch::async,[&]{return jobs::calculate(p,control);});
        const auto limit=std::chrono::steady_clock::now()+std::chrono::seconds(30);bool observed=false;
        while(std::chrono::steady_clock::now()<limit){if(control->stage.load()==stage){observed=true;control->cancel();break;}if(worker.wait_for(std::chrono::milliseconds(0))==std::future_status::ready)break;std::this_thread::yield();}
        control->cancel();result=worker.get();check(observed,"requested phase was never observed");
        check(result.status==jobs::CalculationStatus::Cancelled&&result.tasks.empty()&&!result.projectFinish&&result.documentId==p.documentId&&result.inputRevision==p.revision,"midflight cancellation became conflict or partial success");
    }
    auto fresh=std::make_shared<jobs::CalculationControl>();result=jobs::calculate(p,fresh);
    check(result.status==jobs::CalculationStatus::Valid&&result.tasks.size()==20000&&result.projectFinish=="2026-10-26 10:00"&&fresh->stage.load()==jobs::CalculationStage::Finished,"restart reused cancellation or changed schedule");
    check(p==before,"calculation changed immutable input");
    // Same operation cache must distinguish foreign baseline/calendar contents.
    jobs::EvaluationScope scope;auto other=p;other.holidays={"2026-10-26"};auto moved=jobs::calculate(other);
    check(moved.projectFinish=="2026-10-27 10:00"&&jobs::calculate(p).projectFinish==result.projectFinish,"cache leaked across calendar semantics");
    std::cout<<"PASS: pre-cancel, mid-calendar/backward cancel, no partial result, immutable input, restart and calendar cache isolation\n";
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
