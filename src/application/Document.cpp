#include "application/Document.h"
#include "calculation/WorkCalendar.h"
#include "calculation/Effort.h"
#include <stdexcept>
#include <algorithm>

namespace jobs {
void Document::create(std::string name, std::string date, const std::vector<std::string>& tasks) {
    Project p;
    p.documentId = store_.newId(); p.name = std::move(name); p.startDate = std::move(date);
    for (const auto& taskName : tasks) p.tasks.push_back({store_.newId(), taskName, std::nullopt, {}});
    validate(p);
    project_ = std::move(p); path_.clear(); fingerprint_.clear(); lease_.reset();
    dirty_ = true; readOnly_ = false;
    calculation_.reset(); requiresSaveAs_ = false;
    savedState_.reset(); recoveryOrigin_.reset(); clearHistory(); ++sessionGeneration_; notify(Event::Replaced);
}
void Document::open(const std::string& path, bool readOnly) {
    auto normalized = store_.normalizePath(path);
    if (normalized == path_ && (lease_ || readOnly)) {
        auto loaded = store_.load(normalized);
        project_ = std::move(loaded.project); fingerprint_ = std::move(loaded.fingerprint);
        requiresSaveAs_ = loaded.requiresSaveAs; calculation_.reset();
        if (readOnly) lease_.reset();
        dirty_ = false; readOnly_ = readOnly; savedState_ = project_; recoveryOrigin_.reset(); clearHistory(); ++sessionGeneration_; notify(Event::Replaced); return;
    }
    auto lease = readOnly ? nullptr : store_.lock(normalized);
    auto loaded = store_.load(normalized);
    project_ = std::move(loaded.project); path_ = std::move(normalized);
    fingerprint_ = std::move(loaded.fingerprint); lease_ = std::move(lease);
    requiresSaveAs_ = loaded.requiresSaveAs; calculation_.reset();
    dirty_ = false; readOnly_ = readOnly;
    savedState_ = project_; recoveryOrigin_.reset(); clearHistory(); ++sessionGeneration_; notify(Event::Replaced);
}
void Document::renameTask(std::size_t row, std::string name) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).name = std::move(name);
    edit(std::move(p), "작업 이름 변경");
}
void Document::addTask(std::string name, std::string wbsId) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.push_back({store_.newId(), std::move(name), std::nullopt, {}, std::move(wbsId)});
    edit(std::move(p), "작업 추가");
}
std::string Document::addWbs(std::string name, std::string parentId) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; auto id = store_.newId();
    p.wbsNodes.push_back({id, std::move(name), std::move(parentId)}); edit(std::move(p), "WBS 추가"); return id;
}
void Document::renameWbs(const std::string& id, std::string name) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto& n) { return n.id == id; });
    if (it == p.wbsNodes.end()) throw std::runtime_error("WBS가 존재하지 않습니다.");
    it->name = std::move(name); edit(std::move(p), "WBS 이름 변경");
}
void Document::moveWbs(const std::string& id, std::string parentId) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto& n) { return n.id == id; });
    if (it == p.wbsNodes.end()) throw std::runtime_error("WBS가 존재하지 않습니다.");
    it->parentId = std::move(parentId); edit(std::move(p), "WBS 이동");
}
void Document::removeWbs(const std::string& id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    for (const auto& n : p.wbsNodes) if (n.parentId == id) throw std::runtime_error("하위 WBS를 먼저 이동하거나 제거하세요.");
    for (const auto& t : p.tasks) if (t.wbsId == id) throw std::runtime_error("소속 작업을 먼저 다른 WBS로 이동하세요.");
    const auto count = std::erase_if(p.wbsNodes, [&](const auto& n) { return n.id == id; });
    if (!count) throw std::runtime_error("WBS가 존재하지 않습니다.");
    edit(std::move(p), "빈 WBS 삭제");
}
void Document::assignWbs(std::size_t row, std::string id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).wbsId = std::move(id); edit(std::move(p), "작업 WBS 변경");
}
void Document::setDuration(std::size_t row, std::optional<std::int64_t> minutes) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; changeDuration(p,row,minutes); edit(std::move(p), "작업 기간 변경");
}
void Document::setTaskEffort(std::size_t row,std::string type,bool workInput,std::optional<std::int64_t> duration,std::optional<std::int64_t> work,std::vector<TaskBreak> breaks) {
    if(!project_||readOnly_)throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p=*project_;p.calculationProfile=effortProfile;auto& t=p.tasks.at(row);t.taskType=std::move(type);t.durationMinutes=duration;t.breaks=std::move(breaks);t.workDriven=false;t.workUnitMinutes.reset();
    if(t.taskType=="fixed-work"||workInput){if(!work)throw std::runtime_error("공수 입력이 필요합니다.");changeWork(p,row,*work);if(t.taskType=="fixed-work"&&!workInput)changeDuration(p,row,duration);}
    edit(std::move(p),"작업 유형·공수·분할 변경");
}
void Document::setPredecessors(std::size_t row, std::vector<std::string> ids) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    std::vector<Dependency> edges; for (auto& id : ids) edges.push_back({std::move(id), "FS", 0}); setDependencies(row, std::move(edges));
}
void Document::setDependencies(std::size_t row, std::vector<Dependency> edges) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; auto& t = p.tasks.at(row); t.predecessors.clear(); t.dependencyRules.clear();
    for (const auto& d : edges) { t.predecessors.push_back(d.predecessorId); if (d.type != "FS" || d.lagMinutes) t.dependencyRules.push_back(d); }
    if (!t.dependencyRules.empty() && !isPdm(p)) p.calculationProfile = pdmProfile;
    edit(std::move(p), "선행관계·지연 변경");
}
std::string Document::putCalendar(TaskCalendar c) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; if (c.id.empty()) c.id = store_.newId();
    auto it = std::find_if(p.calendars.begin(), p.calendars.end(), [&](const auto& v) { return v.id == c.id; });
    if (it == p.calendars.end()) p.calendars.push_back(c); else *it = c;
    if(!isPdm(p)) p.calculationProfile = pdmProfile;
    edit(std::move(p), "작업 달력 등록·변경"); return c.id;
}
void Document::removeCalendar(const std::string& id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; for (const auto& t : p.tasks) if (t.calendarId == id) throw std::runtime_error("작업의 달력을 먼저 변경하세요.");
    for(const auto& r:p.resources)if(r.calendarId==id)throw std::runtime_error("자원의 달력을 먼저 변경하세요.");
    if (!std::erase_if(p.calendars, [&](const auto& c) { return c.id == id; })) throw std::runtime_error("달력이 존재하지 않습니다.");
    edit(std::move(p), "작업 달력 삭제");
}
void Document::setTaskCalendar(std::size_t row, std::string id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).calendarId = std::move(id); if (!p.tasks.at(row).calendarId.empty() && !isPdm(p)) p.calculationProfile = pdmProfile;
    edit(std::move(p), "작업 달력 배정");
}
void Document::setTaskTiming(std::size_t row,std::string type,std::optional<std::string> at,std::optional<std::string> deadline) {
    if(!project_||readOnly_)throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p=*project_;auto& t=p.tasks.at(row);if(t.constraintType==type&&t.constraintAt==at&&t.deadlineAt==deadline)return;
    t.constraintType=std::move(type);t.constraintAt=std::move(at);t.deadlineAt=std::move(deadline);if(!isEffort(p))p.calculationProfile=constraintProfile;edit(std::move(p),"작업 제약·마감일 변경");
}
void Document::editGantt(const std::string& id,std::uint64_t revision,const std::string& taskId,bool resize,const std::string& target) {
    if(!project_||readOnly_||project_->documentId!=id||project_->revision!=revision||!calculation_||calculation_->status!=CalculationStatus::Valid||calculation_->inputRevision!=revision||project_->statusAt)throw std::runtime_error("최신 계획 일정에서만 드래그할 수 있습니다. 실제/잔여 예측은 실적·잔여 입력을 사용하세요.");
    if(!validStamp(target))throw std::runtime_error("드래그 시각이 잘못되었습니다.");
    auto p=*project_;auto it=std::find_if(p.tasks.begin(),p.tasks.end(),[&](const auto& t){return t.id==taskId;});
    auto row=std::find_if(calculation_->tasks.begin(),calculation_->tasks.end(),[&](const auto& t){return t.taskId==taskId;});
    if(it==p.tasks.end()||row==calculation_->tasks.end()||(!it->durationMinutes&&!usesWork(*it)))throw std::runtime_error("편집할 작업이 없습니다.");
    if(std::any_of(p.progress.begin(),p.progress.end(),[&](const auto& e){return e.taskId==taskId&&e.actualStart.has_value();}))throw std::runtime_error("실제 시작 이력이 있는 작업은 간트 드래그 대신 실적/잔여를 편집하세요.");
    WorkCalendar cal(p,*it);
    if(resize) {
        const auto duration=cal.offset(target)-cal.offset(row->start);
        if(duration<=0||duration>5000000||row->start==row->finish)throw std::runtime_error("오른쪽 끝은 시작 뒤의 양수 근무 기간이어야 합니다. 마일스톤은 기간 표에서 편집하세요.");
        if(duration==cal.offset(row->finish)-cal.offset(row->start))return;
        changeDuration(p,std::size_t(it-p.tasks.begin()),duration);
    } else {
        const bool zero=usesWork(*it)?it->workUnitMinutes.value_or(0)==0:it->durationMinutes.value_or(0)==0;
        const auto local=zero?cal.finishAtLeast(target):cal.startAtLeast(target);
        const auto trial=isEffort(p)?TaskExecution(p,*it).dates(local):TaskSchedule{};
        const auto start=isEffort(p)?trial.start:cal.boundary(local,zero);if(start==row->start)return;
        const auto finish=isEffort(p)?trial.finish:!zero?cal.boundary(local+*it->durationMinutes,true):start;
        if(it->constraintType=="ASAP"||it->constraintType=="ALAP")it->constraintType="SNET";
        it->constraintAt=it->constraintType=="FNET"||it->constraintType=="FNLT"||it->constraintType=="MFO"?finish:start;
    }
    if(!isEffort(p))p.calculationProfile=constraintProfile;
    edit(std::move(p),resize?"간트 기간 변경":"간트 날짜 이동");
}
void Document::setHolidays(std::vector<std::string> dates) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.holidays = std::move(dates); edit(std::move(p), "휴일 변경");
}
void Document::setStatusAt(std::optional<std::string> stamp) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.statusAt = std::move(stamp); edit(std::move(p), "상태일 변경");
}
std::string Document::appendBaseline(Baseline b) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    if (b.sourceDocumentId != project_->documentId || b.inputRevision != project_->revision || b.input != static_cast<const PlanInput&>(*project_))
        throw std::runtime_error("기준선 준비 중 문서가 변경되었습니다. 최신 계획으로 다시 저장하세요.");
    verifyBaseline(b);
    auto p = *project_; b.id = store_.newId(); b.version = p.baselines.size() + 1; auto id = b.id;
    p.baselines.push_back(std::move(b)); edit(std::move(p)); return id;
}
std::string Document::captureBaseline(std::string name, std::string author, std::string reason, std::string utc) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    return appendBaseline(prepareBaseline(*project_, std::move(name), std::move(author), std::move(reason), std::move(utc)));
}
void Document::recordProgress(ProgressEntry entry) {
    if (!project_ || readOnly_ || !project_->statusAt) throw std::runtime_error("실적 보고 전에 상태일을 설정하세요.");
    auto p = *project_; entry.id = store_.newId(); entry.statusAt = *p.statusAt; entry.recordedRevision = p.revision + 1;
    entry.sourceDocumentId = p.documentId;
    const auto task = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto& t) { return t.id == entry.taskId; });
    if (task == p.tasks.end()) throw std::runtime_error("보고할 작업이 존재하지 않습니다.");
    entry.planDurationMinutes = task->durationMinutes;
    if(isEffort(p)&&usesWork(*task)){const auto plan=calculatePlan(p);entry.planDurationMinutes.reset();if(plan.status==CalculationStatus::Valid)for(const auto& row:plan.tasks)if(row.taskId==task->id){const auto duration=TaskExecution(p,*task).duration(row);if(duration<=5000000)entry.planDurationMinutes=duration;}}
    entry.previousEntryId.clear();
    for (auto it = p.progress.rbegin(); it != p.progress.rend(); ++it) if (it->taskId == entry.taskId) { entry.previousEntryId = it->id; break; }
    p.progress.push_back(std::move(entry)); edit(std::move(p));
}
void Document::staleCalculation() {
    if (calculation_) calculation_->status = CalculationStatus::Stale;
}
void Document::setCurrency(std::string currency) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    if (currency == project_->currency) return;
    auto monetary = [](const PlanInput& p) {
        return std::any_of(p.resources.begin(), p.resources.end(), [](const auto& r) { return !r.rates.empty(); }) || !p.taskCosts.empty();
    };
    bool used = monetary(*project_) || !project_->costEntries.empty();
    for (const auto& b : project_->baselines) used = used || monetary(b.input);
    for (const auto& s : project_->scenarios) used = used || monetary(s.input);
    if (used) throw std::runtime_error("단가·예산·원가 이력이 있는 문서의 통화를 재해석할 수 없습니다. 새 프로젝트에서 통화를 먼저 설정하세요.");
    auto p = *project_; p.currency = std::move(currency); edit(std::move(p), "프로젝트 통화 변경");
}
void Document::setTaskCost(TaskCost cost) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; auto it = std::find_if(p.taskCosts.begin(), p.taskCosts.end(), [&](const auto& c) { return c.taskId == cost.taskId; });
    if (it == p.taskCosts.end()) p.taskCosts.push_back(std::move(cost)); else *it = std::move(cost);
    edit(std::move(p), "작업 예산 변경");
}
void Document::recordCost(CostEntry entry) {
    if (!project_ || readOnly_ || !project_->statusAt) throw std::runtime_error("원가 보고 전에 편집 가능한 프로젝트와 상태일이 필요합니다.");
    auto p = *project_; entry.id = store_.newId(); entry.statusAt = *p.statusAt; entry.currency = p.currency;
    entry.recordedRevision = p.revision + 1; entry.sourceDocumentId = p.documentId; entry.previousEntryId.clear();
    for (auto it = p.costEntries.rbegin(); it != p.costEntries.rend(); ++it) if (it->taskId == entry.taskId) { entry.previousEntryId = it->id; break; }
    p.costEntries.push_back(std::move(entry)); edit(std::move(p));
}
std::string Document::putResource(Resource r) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    if(!r.calendarId.empty())p.calculationProfile=effortProfile;
    if (r.id.empty()) { r.id = store_.newId(); p.resources.push_back(r); }
    else {
        auto it = std::find_if(p.resources.begin(), p.resources.end(), [&](const auto& item) { return item.id == r.id; });
        if (it == p.resources.end()) throw std::runtime_error("자원이 존재하지 않습니다.");
        *it = r;
    }
    auto id = r.id; edit(std::move(p), "자원 등록·변경"); return id;
}
void Document::removeResource(const std::string& id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    for (const auto& a : p.assignments) if (a.resourceId == id) throw std::runtime_error("작업 배정을 먼저 해제하세요.");
    if (!std::erase_if(p.resources, [&](const auto& r) { return r.id == id; })) throw std::runtime_error("자원이 존재하지 않습니다.");
    edit(std::move(p), "자원 삭제");
}
void Document::setAssignments(const std::string& id, std::vector<Assignment> assignments) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    if (std::none_of(p.tasks.begin(), p.tasks.end(), [&](const auto& t) { return t.id == id; })) throw std::runtime_error("작업이 존재하지 않습니다.");
    for (auto& a : assignments) { if (a.taskId != id) throw std::runtime_error("다른 작업의 배정이 포함되었습니다."); }
    std::vector<Assignment> previous;for(const auto& a:p.assignments)if(a.taskId==id)previous.push_back(a);
    auto sorted=[](std::vector<Assignment> values){std::sort(values.begin(),values.end(),[](const auto& a,const auto& b){return a.resourceId<b.resourceId;});return values;};
    if(sorted(previous)==sorted(assignments))return;
    if(isEffort(p))for(auto& t:p.tasks)if(t.id==id&&t.taskType=="fixed-units"&&!t.workDriven) {
        const auto plan=calculatePlan(p);
        if(plan.status==CalculationStatus::Valid)for(const auto& r:plan.tasks)if(r.taskId==id){TaskExecution model(p,t);const auto work=model.work(r);if(work>0){t.workUnitMinutes=work;t.workDriven=true;}}
    }
    std::erase_if(p.assignments, [&](const auto& a) { return a.taskId == id; });
    p.assignments.insert(p.assignments.end(), assignments.begin(), assignments.end()); edit(std::move(p), "작업 자원 배정");
}
std::string Document::saveScenario(std::string name, std::string priority, int horizon, std::string utc,
                                  const std::string& sourceId, std::uint64_t sourceRevision) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    if (sourceId != project_->documentId || sourceRevision != project_->revision)
        throw std::runtime_error("비교 중 문서가 변경되었습니다. 최신 입력으로 다시 계산하세요.");
    auto p = *project_; LevelingScenario s;
    s.id = store_.newId(); s.name = std::move(name); s.createdAt = std::move(utc); s.sourceDocumentId = p.documentId; s.inputRevision = p.revision;
    s.priority = std::move(priority); s.horizonWorkDays = horizon;
    s.input = static_cast<const PlanInput&>(p); s.statusAt = p.statusAt; s.progress = p.progress;
    s.policy=levelingPolicyFor(p);
    auto id = s.id; p.scenarios.push_back(std::move(s)); edit(std::move(p)); return id;
}
void Document::edit(Project p, std::string label) {
    if (p == *project_) return;
    ++p.revision; validate(p);
    const bool immutable = p.baselines != project_->baselines || p.progress != project_->progress || p.costEntries != project_->costEntries || p.scenarios != project_->scenarios;
    if (immutable) clearHistory("승인 기준선·제출 실적/원가·저장 시나리오는 이력으로 보존합니다. 실적/원가는 정정 보고를 사용하세요.");
    else {
        HistoryStep step{{static_cast<const PlanInput&>(*project_), project_->statusAt}, {static_cast<const PlanInput&>(p), p.statusAt}, std::move(label), 0};
        auto weight = [](const InputState& s) {
            const auto& v = s.plan; std::size_t n = sizeof(InputState) + v.name.size() + v.startDate.size() + v.calendar.size() + v.calculationProfile.size() + v.currency.size() + v.costProfile.size();
            if (s.statusAt) n += s.statusAt->size();
            for (const auto& t : v.tasks) { n += sizeof(Task) + t.id.size() + t.name.size() + t.wbsId.size(); for (const auto& id : t.predecessors) n += sizeof(std::string) + id.size(); }
            for (const auto& w : v.wbsNodes) n += sizeof(WbsNode) + w.id.size() + w.name.size() + w.parentId.size();
            for (const auto& h : v.holidays) n += sizeof(std::string) + h.size();
            for (const auto& r : v.resources) { n += sizeof(Resource) + r.id.size() + r.name.size() + r.type.size()+r.calendarId.size(); for (const auto& d : r.exceptions) n += sizeof(CapacityDay) + d.date.size(); for (const auto& rate : r.rates) n += sizeof(Resource::Rate) + rate.effectiveDate.size(); }
            for (const auto& a : v.assignments) n += sizeof(Assignment) + a.taskId.size() + a.resourceId.size();
            for (const auto& c : v.taskCosts) n += sizeof(TaskCost) + c.taskId.size() + c.accrual.size() + c.credit.size();
            for (const auto& t : v.tasks) { n += t.calendarId.size(); for (const auto& d : t.dependencyRules) n += sizeof(Dependency) + d.predecessorId.size() + d.type.size(); }
            for (const auto& t : v.tasks) n+=t.constraintType.size()+(t.constraintAt?t.constraintAt->size():0)+(t.deadlineAt?t.deadlineAt->size():0);
            for(const auto& t:v.tasks){n+=t.taskType.size();for(const auto& b:t.breaks)n+=sizeof(TaskBreak)+b.start.size()+b.finish.size();}
            for (const auto& c : v.calendars) { n += sizeof(TaskCalendar) + c.id.size() + c.name.size() + c.weekdays.size() * sizeof(int) + c.periods.size() * sizeof(WorkPeriod); for (const auto& d : c.holidays) n += sizeof(std::string) + d.size(); }
            return n;
        };
        step.weight = weight(step.before) + weight(step.after) + step.label.size();
        while (history_.size() > historyCursor_) { historyWeight_ -= history_.back().weight; history_.pop_back(); }
        constexpr std::size_t maxWeight = 32 * 1024 * 1024;
        if (step.weight > maxWeight) clearHistory("이번 변경은 실행 취소 이력의 추정 입력 32 MiB 한도를 초과했습니다.");
        else {
            historyWeight_ += step.weight; history_.push_back(std::move(step));
            while (history_.size() > 100 || historyWeight_ > maxWeight) { historyWeight_ -= history_.front().weight; history_.erase(history_.begin()); historyNotice_ = "최근 100단계 / 추정 입력 32 MiB 범위의 실행 취소 이력입니다."; }
            historyCursor_ = history_.size();
        }
    }
    project_ = std::move(p); updateDirty(); staleCalculation(); notify(Event::Edited);
}
void Document::applyCalculation(ScheduleResult result) {
    if (!project_) return;
    if(result.status==CalculationStatus::Cancelled){result.tasks.clear();result.projectFinish.reset();}
    if (result.documentId != project_->documentId || result.inputRevision != project_->revision || result.profile != project_->calculationProfile || result.statusAt != project_->statusAt)
        result.status = CalculationStatus::Stale;
    calculation_ = std::move(result);
}
void Document::save() {
    if (!project_ || path_.empty() || readOnly_ || requiresSaveAs_) throw std::runtime_error("다른 이름으로 저장하세요. 이전 형식의 원본은 보존합니다.");
    auto fingerprint = store_.save(path_, *project_, fingerprint_);
    fingerprint_ = std::move(fingerprint); dirty_ = false;
    savedState_ = project_; recoveryOrigin_.reset(); notify(Event::Saved);
}
void Document::saveAs(const std::string& path) {
    if (!project_) throw std::runtime_error("열린 프로젝트가 없습니다.");
    auto normalized = store_.normalizePath(path);
    if (normalized == path_) {
        throw std::runtime_error("독립 사본은 현재 파일과 다른 경로에 저장하세요.");
    }
    auto lease = store_.lock(normalized);
    auto p = *project_;
    const bool independent = !path_.empty();
    if (!path_.empty()) { p.derivedFrom = p.documentId; p.documentId = store_.newId(); }
    auto fingerprint = store_.save(normalized, p, {});
    project_ = std::move(p); path_ = std::move(normalized); fingerprint_ = std::move(fingerprint);
    lease_ = std::move(lease); dirty_ = false; readOnly_ = false;
    requiresSaveAs_ = false;
    if (calculation_ && calculation_->documentId != project_->documentId) staleCalculation();
    savedState_ = project_; recoveryOrigin_.reset(); if (independent) clearHistory(); ++sessionGeneration_; notify(Event::Saved);
}
void Document::clearHistory(std::string notice) { history_.clear(); historyCursor_ = historyWeight_ = 0; historyNotice_ = std::move(notice); }
std::string Document::undoLabel() const { return canUndo() ? history_[historyCursor_ - 1].label : std::string{}; }
std::string Document::redoLabel() const { return canRedo() ? history_[historyCursor_].label : std::string{}; }
void Document::updateDirty() {
    if (!project_ || !savedState_) { dirty_ = project_.has_value(); return; }
    const auto& a = *project_; const auto& b = *savedState_;
    dirty_ = static_cast<const PlanInput&>(a) != static_cast<const PlanInput&>(b) || a.statusAt != b.statusAt || a.documentId != b.documentId || a.derivedFrom != b.derivedFrom || a.baselines != b.baselines || a.progress != b.progress || a.scenarios != b.scenarios || a.costEntries != b.costEntries;
}
void Document::applyHistory(const InputState& state) {
    auto p = *project_; static_cast<PlanInput&>(p) = state.plan; p.statusAt = state.statusAt; ++p.revision; validate(p);
    project_ = std::move(p); updateDirty(); staleCalculation();
}
void Document::undo() { if (!canUndo()) throw std::runtime_error("실행 취소할 편집이 없습니다."); applyHistory(history_[historyCursor_ - 1].before); --historyCursor_; notify(Event::Edited); }
void Document::redo() { if (!canRedo()) throw std::runtime_error("다시 실행할 편집이 없습니다."); applyHistory(history_[historyCursor_].after); ++historyCursor_; notify(Event::Edited); }
std::uint64_t Document::subscribe(std::function<void(Event)> listener) { auto token = ++listenerToken_; listeners_.emplace(token, std::move(listener)); return token; }
void Document::unsubscribe(std::uint64_t token) { listeners_.erase(token); }
void Document::notify(Event event) noexcept {
    // Observers schedule work only; a failed observer cannot roll back a committed edit.
    try { auto listeners = listeners_; for (const auto& [id, listener] : listeners) { (void)id; try { listener(event); } catch (...) {} } } catch (...) {}
}
RecoverySnapshot Document::recoverySnapshot(std::string utc) const {
    if (!project_ || readOnly_ || !dirty_ || !validUtcStamp(utc)) throw std::runtime_error("자동 복구 저장할 편집 가능한 미저장 문서/시각이 없습니다.");
    return {*project_, path_, fingerprint_, std::move(utc), readOnly_, requiresSaveAs_, savedState_ ? std::optional{savedState_->revision} : std::nullopt};
}
void Document::recover(const RecoverySnapshot& snapshot) {
    validate(snapshot.project); if (!validUtcStamp(snapshot.capturedAt)) throw std::runtime_error("복구 시각이 잘못되었습니다.");
    auto p = snapshot.project; p.derivedFrom = p.documentId; p.documentId = store_.newId(); validate(p);
    RecoveryOrigin origin{snapshot.project.documentId, snapshot.sourcePath, snapshot.capturedAt, snapshot.project.revision, snapshot.lastSavedRevision};
    project_ = std::move(p); path_.clear(); fingerprint_.clear(); lease_.reset(); savedState_.reset(); calculation_.reset();
    dirty_ = true; readOnly_ = false; requiresSaveAs_ = true; recoveryOrigin_ = std::move(origin);
    clearHistory("복구한 입력에서 새 편집 이력을 시작합니다. 복구 전 실행 취소 이력은 포함하지 않습니다."); ++sessionGeneration_; notify(Event::Replaced);
}
}
