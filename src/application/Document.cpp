#include "domain/Diagnostics.h"
#include "application/Document.h"
#include "calculation/WorkCalendar.h"
#include "calculation/Effort.h"
#include <stdexcept>
#include <algorithm>

namespace jobs {
bool Document::isEditable() const {
    return project_ && !readOnly_;
}
void Document::requireEditable() const {
    if (!isEditable())
        throw AppError(ErrorCode::DOCUMENT_NOT_EDITABLE, {});
}
void Document::create(std::string name, std::string date, const std::vector<std::string> &tasks) {
    Project p;
    p.documentId = store_.newId();
    p.name = std::move(name);
    p.startDate = std::move(date);
    for (const auto &taskName : tasks)
        p.tasks.push_back({store_.newId(), taskName, std::nullopt, {}});
    validate(p);
    project_ = std::move(p);
    path_.clear();
    fingerprint_.clear();
    lease_.reset();
    dirty_ = true;
    readOnly_ = false;
    calculation_.reset();
    ++calculationGeneration_;
    requiresSaveAs_ = false;
    savedState_.reset();
    dirtyState_ = {};
    taskChange_.reset();
    recoveryOrigin_.reset();
    clearHistory();
    ++sessionGeneration_;
    notify(Event::Replaced);
}
void Document::open(const std::string &path, bool readOnly) {
    auto normalized = store_.normalizePath(path);
    if (store_.sameFile(normalized, path_) && (lease_ || readOnly)) {
        auto loaded = store_.load(normalized);
        project_ = std::move(loaded.project);
        fingerprint_ = std::move(loaded.fingerprint);
        requiresSaveAs_ = loaded.requiresSaveAs;
        calculation_.reset();
        ++calculationGeneration_;
        if (readOnly)
            lease_.reset();
        dirty_ = false;
        readOnly_ = readOnly;
        savedState_ = Project(*project_);
        dirtyState_ = {0, false, true};
        dirtyComparisons_ = 0;
        recoveryOrigin_.reset();
        clearHistory();
        ++sessionGeneration_;
        notify(Event::Replaced);
        return;
    }
    auto lease = readOnly ? nullptr : store_.lock(normalized);
    auto loaded = store_.load(normalized);
    project_ = std::move(loaded.project);
    path_ = std::move(normalized);
    fingerprint_ = std::move(loaded.fingerprint);
    lease_ = std::move(lease);
    requiresSaveAs_ = loaded.requiresSaveAs;
    calculation_.reset();
    ++calculationGeneration_;
    dirty_ = false;
    readOnly_ = readOnly;
    savedState_ = Project(*project_);
    dirtyState_ = {0, false, true};
    dirtyComparisons_ = 0;
    recoveryOrigin_.reset();
    clearHistory();
    ++sessionGeneration_;
    notify(Event::Replaced);
}
void Document::renameTask(std::size_t row, std::string name) {
    requireEditable();
    const auto &original = project_->tasks.at(row);
    if (original.name == name)
        return;
    auto before = original;
    auto &task = project_.write().tasks.at(row);
    if (task.name == name)
        return;
    validateTaskName(name);
    if (project_->revision >= 9007199254740991ULL)
        throw AppError(ErrorCode::PROJECT_ID_REVISION_INVALID, {});
    TaskNameDelta delta{row, task.id, task.name, name};
    const auto weight = sizeof(HistoryStep) + delta.taskId.size() + delta.before.size() + delta.after.size();
    pushHistory({std::move(delta), "작업 이름 변경", weight + std::string("작업 이름 변경").size()});
    // The document is already valid; changing only the display name cannot affect
    // references, calendars, booked facts or any other domain invariant.
    task.name.swap(name);
    ++project_.write().revision;
    updateTaskDirty(row, before);
    taskChange_ = TaskChange{project_->revision - 1, project_->revision, row, true};
    staleCalculation();
    notify(Event::Edited);
}
void Document::addTask(std::string name, std::string wbsId) {
    requireEditable();
    auto p = *project_;
    p.tasks.push_back({store_.newId(), std::move(name), std::nullopt, {}, std::move(wbsId)});
    edit(std::move(p), "작업 추가");
}
std::string Document::addWbs(std::string name, std::string parentId) {
    requireEditable();
    auto p = *project_;
    auto id = store_.newId();
    p.wbsNodes.push_back({id, std::move(name), std::move(parentId)});
    edit(std::move(p), "WBS 추가");
    return id;
}
void Document::renameWbs(const std::string &id, std::string name) {
    requireEditable();
    auto p = *project_;
    auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto &n) { return n.id == id; });
    if (it == p.wbsNodes.end())
        throw AppError(ErrorCode::WBS_NOT_FOUND, {});
    it->name = std::move(name);
    edit(std::move(p), "WBS 이름 변경");
}
void Document::moveWbs(const std::string &id, std::string parentId) {
    requireEditable();
    auto p = *project_;
    auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto &n) { return n.id == id; });
    if (it == p.wbsNodes.end())
        throw AppError(ErrorCode::WBS_NOT_FOUND, {});
    it->parentId = std::move(parentId);
    edit(std::move(p), "WBS 이동");
}
void Document::removeWbs(const std::string &id) {
    requireEditable();
    auto p = *project_;
    for (const auto &n : p.wbsNodes)
        if (n.parentId == id)
            throw AppError(ErrorCode::WBS_HAS_CHILDREN, {});
    for (const auto &t : p.tasks)
        if (t.wbsId == id)
            throw AppError(ErrorCode::WBS_HAS_TASKS, {});
    const auto count = std::erase_if(p.wbsNodes, [&](const auto &n) { return n.id == id; });
    if (!count)
        throw AppError(ErrorCode::WBS_NOT_FOUND, {});
    edit(std::move(p), "빈 WBS 삭제");
}
void Document::assignWbs(std::size_t row, std::string id) {
    requireEditable();
    auto p = *project_;
    p.tasks.at(row).wbsId = std::move(id);
    edit(std::move(p), "작업 WBS 변경");
}
void Document::setDuration(std::size_t row, std::optional<std::int64_t> minutes) {
    requireEditable();
    if (project_->tasks.at(row).taskType == TaskType::FixedWork) {
        auto p = *project_;
        changeDuration(p, row, minutes);
        edit(std::move(p), "작업 기간 변경");
        return;
    }
    // For a valid project these fields only affect the task's duration/work basis.
    // Reports retain their own captured plan duration; IDs/references stay fixed.
    if (minutes && (*minutes < 0 || *minutes > 5000000))
        throw AppError(ErrorCode::TASK_DURATION_INVALID, {});
    auto before = project_->tasks.at(row), after = before;
    after.durationMinutes = minutes;
    if (after.taskType == TaskType::FixedUnits) {
        after.workDriven = false;
        after.workUnitMinutes.reset();
    }
    if (before == after)
        return;
    if (project_->revision >= 9007199254740991ULL)
        throw AppError(ErrorCode::PROJECT_ID_REVISION_INVALID, {});
    auto values = std::make_shared<const std::pair<Task, Task>>(before, after);
    const auto weight = sizeof(HistoryStep) + sizeof(*values) + inputExtraBytes(before) + inputExtraBytes(after) + 32;
    // Detach before changing history so allocation failure is not a partial edit.
    auto &p = project_.write();
    pushHistory({HistoryStep::TaskValue{row, std::move(values)}, "작업 기간 변경", weight});
    p.tasks[row] = std::move(after);
    ++p.revision;
    updateTaskDirty(row, before);
    taskChange_ = TaskChange{p.revision - 1, p.revision, row, false};
    staleCalculation();
    notify(Event::Edited);
}
void Document::setTaskEffort(std::size_t row, TaskType type, bool workInput, std::optional<std::int64_t> duration,
                             std::optional<std::int64_t> work, std::vector<TaskBreak> breaks) {
    requireEditable();
    auto p = *project_;
    p.calculationProfile = effortProfile;
    auto &t = p.tasks.at(row);
    t.taskType = std::move(type);
    t.durationMinutes = duration;
    t.breaks = std::move(breaks);
    t.workDriven = false;
    t.workUnitMinutes.reset();
    if (t.taskType == TaskType::FixedWork || workInput) {
        if (!work)
            throw AppError(ErrorCode::WORK_INPUT_REQUIRED, {});
        changeWork(p, row, *work);
        if (t.taskType == TaskType::FixedWork && !workInput)
            changeDuration(p, row, duration);
    }
    edit(std::move(p), "작업 유형·공수·분할 변경");
}
void Document::setPredecessors(std::size_t row, std::vector<std::string> ids) {
    requireEditable();
    std::vector<Dependency> edges;
    for (auto &id : ids)
        edges.push_back({std::move(id), DependencyType::FS, 0});
    setDependencies(row, std::move(edges));
}
void Document::setDependencies(std::size_t row, std::vector<Dependency> edges) {
    requireEditable();
    auto p = *project_;
    auto &t = p.tasks.at(row);
    t.predecessors.clear();
    t.dependencyRules.clear();
    for (const auto &d : edges) {
        t.predecessors.push_back(d.predecessorId);
        if (d.type != DependencyType::FS || d.lagMinutes)
            t.dependencyRules.push_back(d);
    }
    if (!t.dependencyRules.empty() && !isPdm(p))
        p.calculationProfile = pdmProfile;
    edit(std::move(p), "선행관계·지연 변경");
}
std::string Document::putCalendar(TaskCalendar c) {
    requireEditable();
    auto p = *project_;
    if (c.id.empty())
        c.id = store_.newId();
    auto it = std::find_if(p.calendars.begin(), p.calendars.end(), [&](const auto &v) { return v.id == c.id; });
    if (it == p.calendars.end())
        p.calendars.push_back(c);
    else
        *it = c;
    if (!isPdm(p))
        p.calculationProfile = pdmProfile;
    edit(std::move(p), "작업 달력 등록·변경");
    return c.id;
}
void Document::removeCalendar(const std::string &id) {
    requireEditable();
    auto p = *project_;
    for (const auto &t : p.tasks)
        if (t.calendarId == id)
            throw AppError(ErrorCode::CALENDAR_HAS_TASKS, {});
    for (const auto &r : p.resources)
        if (r.calendarId == id)
            throw AppError(ErrorCode::CALENDAR_HAS_RESOURCES, {});
    if (!std::erase_if(p.calendars, [&](const auto &c) { return c.id == id; }))
        throw AppError(ErrorCode::CALENDAR_NOT_FOUND, {});
    edit(std::move(p), "작업 달력 삭제");
}
void Document::setTaskCalendar(std::size_t row, std::string id) {
    requireEditable();
    auto p = *project_;
    p.tasks.at(row).calendarId = std::move(id);
    if (!p.tasks.at(row).calendarId.empty() && !isPdm(p))
        p.calculationProfile = pdmProfile;
    edit(std::move(p), "작업 달력 배정");
}
void Document::setTaskTiming(std::size_t row, ConstraintType type, std::optional<std::string> at,
                             std::optional<std::string> deadline) {
    requireEditable();
    auto p = *project_;
    auto &t = p.tasks.at(row);
    if (t.constraintType == type && t.constraintAt == at && t.deadlineAt == deadline)
        return;
    t.constraintType = std::move(type);
    t.constraintAt = std::move(at);
    t.deadlineAt = std::move(deadline);
    if (!isEffort(p))
        p.calculationProfile = constraintProfile;
    edit(std::move(p), "작업 제약·마감일 변경");
}
void Document::editGantt(const std::string &id, std::uint64_t revision, const std::string &taskId, bool resize,
                         const std::string &target) {
    if (!isEditable() || project_->documentId != id || project_->revision != revision || !calculation_ ||
        calculation_->status != CalculationStatus::Valid || calculation_->inputRevision != revision ||
        project_->statusAt)
        throw AppError(ErrorCode::GANTT_REQUIRES_CURRENT_PLAN, {});
    if (!validStamp(target))
        throw AppError(ErrorCode::GANTT_TIMESTAMP_INVALID, {});
    auto p = *project_;
    auto it = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto &t) { return t.id == taskId; });
    auto row = std::find_if(calculation_->tasks.begin(), calculation_->tasks.end(),
                            [&](const auto &t) { return t.taskId == taskId; });
    if (it == p.tasks.end() || row == calculation_->tasks.end() || (!it->durationMinutes && !usesWork(*it)))
        throw AppError(ErrorCode::TASK_NOT_FOUND, {});
    if (std::any_of(p.progress.begin(), p.progress.end(),
                    [&](const auto &e) { return e.taskId == taskId && e.actualStart.has_value(); }))
        throw AppError(ErrorCode::GANTT_HAS_ACTUAL_PROGRESS, {});
    WorkCalendar cal(p, *it);
    if (resize) {
        const auto duration = cal.offset(target) - cal.offset(row->start);
        if (duration <= 0 || duration > 5000000 || row->start == row->finish)
            throw AppError(ErrorCode::GANTT_DURATION_INVALID, {});
        if (duration == cal.offset(row->finish) - cal.offset(row->start))
            return;
        changeDuration(p, std::size_t(it - p.tasks.begin()), duration);
    } else {
        const bool zero = usesWork(*it) ? it->workUnitMinutes.value_or(0) == 0 : it->durationMinutes.value_or(0) == 0;
        const auto local = zero ? cal.finishAtLeast(target) : cal.startAtLeast(target);
        const auto trial = isEffort(p) ? TaskExecution(p, *it).dates(local) : TaskSchedule{};
        const auto start = isEffort(p) ? trial.start : cal.boundary(local, zero);
        if (start == row->start)
            return;
        const auto finish = isEffort(p) ? trial.finish
                            : !zero     ? cal.boundary(local + *it->durationMinutes, true)
                                        : start;
        if (it->constraintType == ConstraintType::ASAP || it->constraintType == ConstraintType::ALAP)
            it->constraintType = ConstraintType::SNET;
        it->constraintAt = it->constraintType == ConstraintType::FNET || it->constraintType == ConstraintType::FNLT ||
                                   it->constraintType == ConstraintType::MFO
                               ? finish
                               : start;
    }
    if (!isEffort(p))
        p.calculationProfile = constraintProfile;
    edit(std::move(p), resize ? "간트 기간 변경" : "간트 날짜 이동");
}
void Document::setHolidays(std::vector<std::string> dates) {
    requireEditable();
    auto p = *project_;
    p.holidays = std::move(dates);
    edit(std::move(p), "휴일 변경");
}
void Document::setStatusAt(std::optional<std::string> stamp) {
    requireEditable();
    auto p = *project_;
    p.statusAt = std::move(stamp);
    edit(std::move(p), "상태일 변경");
}
std::string Document::appendBaseline(Baseline b) {
    requireEditable();
    if (b.sourceDocumentId != project_->documentId || b.inputRevision != project_->revision ||
        b.input != static_cast<const PlanInput &>(*project_))
        throw AppError(ErrorCode::BASELINE_SOURCE_CHANGED, {});
    verifyBaseline(b);
    auto p = *project_;
    b.id = store_.newId();
    b.version = p.baselines.size() + 1;
    auto id = b.id;
    p.baselines.push_back(std::move(b));
    edit(std::move(p));
    return id;
}
std::string Document::captureBaseline(std::string name, std::string author, std::string reason, std::string utc) {
    requireEditable();
    return appendBaseline(
        prepareBaseline(*project_, std::move(name), std::move(author), std::move(reason), std::move(utc)));
}
void Document::recordProgress(ProgressEntry entry) {
    if (!isEditable() || !project_->statusAt)
        throw AppError(ErrorCode::STATUS_DATE_REQUIRED, {});
    auto p = *project_;
    entry.id = store_.newId();
    entry.statusAt = *p.statusAt;
    entry.recordedRevision = p.revision + 1;
    entry.sourceDocumentId = p.documentId;
    const auto task = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto &t) { return t.id == entry.taskId; });
    if (task == p.tasks.end())
        throw AppError(ErrorCode::PROGRESS_TASK_NOT_FOUND, {});
    entry.planDurationMinutes = task->durationMinutes;
    if (isEffort(p) && usesWork(*task)) {
        const auto plan = calculatePlan(p);
        entry.planDurationMinutes.reset();
        if (plan.status == CalculationStatus::Valid)
            for (const auto &row : plan.tasks)
                if (row.taskId == task->id) {
                    const auto duration = TaskExecution(p, *task).duration(row);
                    if (duration <= 5000000)
                        entry.planDurationMinutes = duration;
                }
    }
    entry.previousEntryId.clear();
    for (auto it = p.progress.rbegin(); it != p.progress.rend(); ++it)
        if (it->taskId == entry.taskId) {
            entry.previousEntryId = it->id;
            break;
        }
    p.progress.push_back(std::move(entry));
    edit(std::move(p));
}
void Document::staleCalculation() {
    bulkView_.reset();
    if (calculation_ && calculation_->status != CalculationStatus::Stale) {
        calculation_->status = CalculationStatus::Stale;
        ++calculationGeneration_;
    }
}
void Document::setCurrency(std::string currency) {
    requireEditable();
    if (currency == project_->currency)
        return;
    auto monetary = [](const PlanInput &p) {
        return std::any_of(p.resources.begin(), p.resources.end(), [](const auto &r) { return !r.rates.empty(); }) ||
               !p.taskCosts.empty();
    };
    bool used = monetary(*project_) || !project_->costEntries.empty();
    for (const auto &b : project_->baselines)
        used = used || monetary(b.input);
    for (const auto &s : project_->scenarios)
        used = used || monetary(s.input);
    if (used)
        throw AppError(ErrorCode::CURRENCY_HAS_HISTORY, {});
    auto p = *project_;
    p.currency = std::move(currency);
    edit(std::move(p), "프로젝트 통화 변경");
}
void Document::setTaskCost(TaskCost cost) {
    requireEditable();
    auto p = *project_;
    auto it =
        std::find_if(p.taskCosts.begin(), p.taskCosts.end(), [&](const auto &c) { return c.taskId == cost.taskId; });
    if (it == p.taskCosts.end())
        p.taskCosts.push_back(std::move(cost));
    else
        *it = std::move(cost);
    edit(std::move(p), "작업 예산 변경");
}
void Document::recordCost(CostEntry entry) {
    if (!isEditable() || !project_->statusAt)
        throw AppError(ErrorCode::COST_STATUS_DATE_REQUIRED, {});
    auto p = *project_;
    entry.id = store_.newId();
    entry.statusAt = *p.statusAt;
    entry.currency = p.currency;
    entry.recordedRevision = p.revision + 1;
    entry.sourceDocumentId = p.documentId;
    entry.previousEntryId.clear();
    for (auto it = p.costEntries.rbegin(); it != p.costEntries.rend(); ++it)
        if (it->taskId == entry.taskId) {
            entry.previousEntryId = it->id;
            break;
        }
    p.costEntries.push_back(std::move(entry));
    edit(std::move(p));
}
std::string Document::putResource(Resource r) {
    requireEditable();
    auto p = *project_;
    if (!r.calendarId.empty())
        p.calculationProfile = effortProfile;
    if (r.id.empty()) {
        r.id = store_.newId();
        p.resources.push_back(r);
    } else {
        auto it =
            std::find_if(p.resources.begin(), p.resources.end(), [&](const auto &item) { return item.id == r.id; });
        if (it == p.resources.end())
            throw AppError(ErrorCode::RESOURCE_NOT_FOUND, {});
        *it = r;
    }
    auto id = r.id;
    edit(std::move(p), "자원 등록·변경");
    return id;
}
void Document::removeResource(const std::string &id) {
    requireEditable();
    auto p = *project_;
    for (const auto &a : p.assignments)
        if (a.resourceId == id)
            throw AppError(ErrorCode::RESOURCE_HAS_ASSIGNMENTS, {});
    if (!std::erase_if(p.resources, [&](const auto &r) { return r.id == id; }))
        throw AppError(ErrorCode::RESOURCE_NOT_FOUND, {});
    edit(std::move(p), "자원 삭제");
}
void Document::setAssignments(const std::string &id, std::vector<Assignment> assignments) {
    requireEditable();
    auto p = *project_;
    if (std::none_of(p.tasks.begin(), p.tasks.end(), [&](const auto &t) { return t.id == id; }))
        throw AppError(ErrorCode::TASK_NOT_FOUND, {}, {}, "TASK_NOT_FOUND_2");
    for (auto &a : assignments) {
        if (a.taskId != id)
            throw AppError(ErrorCode::ASSIGNMENT_TASK_MISMATCH, {});
    }
    std::vector<Assignment> previous;
    for (const auto &a : p.assignments)
        if (a.taskId == id)
            previous.push_back(a);
    auto sorted = [](std::vector<Assignment> values) {
        std::sort(values.begin(), values.end(),
                  [](const auto &a, const auto &b) { return a.resourceId < b.resourceId; });
        return values;
    };
    if (sorted(previous) == sorted(assignments))
        return;
    if (isEffort(p))
        for (auto &t : p.tasks)
            if (t.id == id && t.taskType == TaskType::FixedUnits && !t.workDriven) {
                const auto plan = calculatePlan(p);
                if (plan.status == CalculationStatus::Valid)
                    for (const auto &r : plan.tasks)
                        if (r.taskId == id) {
                            TaskExecution model(p, t);
                            const auto work = model.work(r);
                            if (work > 0) {
                                t.workUnitMinutes = work;
                                t.workDriven = true;
                            }
                        }
            }
    std::erase_if(p.assignments, [&](const auto &a) { return a.taskId == id; });
    p.assignments.insert(p.assignments.end(), assignments.begin(), assignments.end());
    edit(std::move(p), "작업 자원 배정");
}
std::string Document::saveScenario(std::string name, std::string priority, int horizon, std::string utc,
                                   const std::string &sourceId, std::uint64_t sourceRevision) {
    requireEditable();
    if (sourceId != project_->documentId || sourceRevision != project_->revision)
        throw AppError(ErrorCode::SCENARIO_SOURCE_CHANGED, {});
    auto p = *project_;
    LevelingScenario s;
    s.id = store_.newId();
    s.name = std::move(name);
    s.createdAt = std::move(utc);
    s.sourceDocumentId = p.documentId;
    s.inputRevision = p.revision;
    s.priority = std::move(priority);
    s.horizonWorkDays = horizon;
    s.input = static_cast<const PlanInput &>(p);
    s.statusAt = p.statusAt;
    s.progress = p.progress;
    s.policy = levelingPolicyFor(p);
    auto id = s.id;
    p.scenarios.push_back(std::move(s));
    edit(std::move(p));
    return id;
}
void Document::edit(Project p, std::string label) {
    if (p == *project_)
        return;
    ++p.revision;
    validate(p);
    const bool immutable = p.baselines != project_->baselines || p.progress != project_->progress ||
                           p.costEntries != project_->costEntries || p.scenarios != project_->scenarios;
    if (immutable)
        clearHistory(
            "승인 기준선·제출 실적/원가·저장 시나리오는 이력으로 보존합니다. 실적/원가는 정정 보고를 사용하세요.");
    else {
        InputDelta delta(*project_, p);
        const auto weight = sizeof(HistoryStep) + delta.bytes() + label.size();
        pushHistory({std::make_shared<const InputDelta>(std::move(delta)), std::move(label), weight});
    }
    project_ = std::move(p);
    taskChange_.reset();
    updateDirty();
    staleCalculation();
    notify(Event::Edited);
}
void Document::applyCalculation(ScheduleResult result) {
    if (!project_)
        return;
    if (result.status == CalculationStatus::Cancelled) {
        result.tasks.clear();
        result.plannedTasks.clear();
        result.projectFinish.reset();
    }
    if (result.documentId != project_->documentId || result.inputRevision != project_->revision ||
        result.profile != project_->calculationProfile || result.statusAt != project_->statusAt)
        result.status = CalculationStatus::Stale;
    calculation_ = std::move(result);
    ++calculationGeneration_;
}
void Document::save() {
    if (!isEditable() || path_.empty() || requiresSaveAs_)
        throw AppError(ErrorCode::LEGACY_REQUIRES_SAVE_AS, {});
    auto fingerprint = store_.save(path_, *project_, fingerprint_);
    fingerprint_ = std::move(fingerprint);
    dirty_ = false;
    savedState_ = Project(*project_);
    dirtyState_ = {0, false, true};
    dirtyComparisons_ = 0;
    recoveryOrigin_.reset();
    notify(Event::Saved);
}
void Document::saveAs(const std::string &path) {
    if (!project_)
        throw AppError(ErrorCode::DOCUMENT_NOT_OPEN, {});
    auto normalized = store_.normalizePath(path);
    if (store_.sameFile(normalized, path_)) {
        throw AppError(ErrorCode::SAVE_AS_REQUIRES_NEW_PATH, {});
    }
    auto lease = store_.lock(normalized);
    auto p = *project_;
    const bool independent = !path_.empty();
    if (!path_.empty()) {
        p.derivedFrom = p.documentId;
        p.documentId = store_.newId();
    }
    auto fingerprint = store_.save(normalized, p, {});
    project_ = std::move(p);
    path_ = std::move(normalized);
    fingerprint_ = std::move(fingerprint);
    lease_ = std::move(lease);
    dirty_ = false;
    readOnly_ = false;
    requiresSaveAs_ = false;
    if (calculation_ && calculation_->documentId != project_->documentId)
        staleCalculation();
    savedState_ = Project(*project_);
    dirtyState_ = {0, false, true};
    dirtyComparisons_ = 0;
    recoveryOrigin_.reset();
    if (independent)
        clearHistory();
    ++sessionGeneration_;
    notify(Event::Saved);
}
void Document::clearHistory(std::string notice) {
    bulkView_.reset();
    history_.clear();
    historyCursor_ = historyWeight_ = 0;
    historyNotice_ = std::move(notice);
}
std::string Document::undoLabel() const {
    return canUndo() ? history_[historyCursor_ - 1].label : std::string{};
}
std::string Document::redoLabel() const {
    return canRedo() ? history_[historyCursor_].label : std::string{};
}
void Document::updateDirty() {
    dirtyState_ = project_ ? compareSavedInput(*project_, savedState_) : DirtyState{};
    dirty_ = project_ && dirtyState_.dirty();
    dirtyComparisons_ = project_ ? project_->tasks.size() : 0;
}
void Document::updateTaskDirty(std::size_t row, const Task &before) {
    dirtyComparisons_ = 0;
    if (savedState_ && dirtyState_.aligned) {
        const auto &saved = savedState_->tasks.at(row);
        const bool was = before != saved, now = project_->tasks.at(row) != saved;
        if (was != now) {
            if (now)
                ++dirtyState_.differentTasks;
            else
                --dirtyState_.differentTasks;
        }
        dirtyComparisons_ = 2;
    }
    dirty_ = dirtyState_.dirty();
}
void Document::pushHistory(HistoryStep step) {
    constexpr std::size_t maxWeight = 32 * 1024 * 1024;
    if (step.weight > maxWeight) {
        clearHistory("이번 변경은 실행 취소 이력의 추정 입력 32 MiB 한도를 초과했습니다.");
        return;
    }
    history_.reserve(history_.size() + 1);
    while (history_.size() > historyCursor_) {
        historyWeight_ -= history_.back().weight;
        history_.pop_back();
    }
    historyWeight_ += step.weight;
    history_.push_back(std::move(step));
    while (history_.size() > 100 || historyWeight_ > maxWeight) {
        historyWeight_ -= history_.front().weight;
        history_.erase(history_.begin());
        historyNotice_ = "최근 100단계 / 추정 입력 32 MiB 범위의 실행 취소 이력입니다.";
    }
    historyCursor_ = history_.size();
}
void Document::applyHistory(const HistoryStep &step, bool forward) {
    if (const auto *delta = std::get_if<TaskNameDelta>(&step.change)) {
        if (project_->revision >= 9007199254740991ULL)
            throw AppError(ErrorCode::PROJECT_ID_REVISION_INVALID, {});
        auto before = project_->tasks.at(delta->row);
        auto &task = project_.write().tasks.at(delta->row);
        if (task.id != delta->taskId)
            throw std::logic_error("실행 취소 작업 ID가 일치하지 않습니다.");
        auto name = forward ? delta->after : delta->before;
        task.name.swap(name);
        ++project_.write().revision;
        updateTaskDirty(delta->row, before);
        taskChange_ = TaskChange{project_->revision - 1, project_->revision, delta->row, true};
    } else if (const auto *value = std::get_if<HistoryStep::TaskValue>(&step.change)) {
        if (project_->revision >= 9007199254740991ULL)
            throw AppError(ErrorCode::PROJECT_ID_REVISION_INVALID, {});
        auto before = project_->tasks.at(value->row);
        auto after = forward ? value->values->second : value->values->first;
        if (before.id != after.id)
            throw std::logic_error("실행 취소 작업 ID가 일치하지 않습니다.");
        auto &p = project_.write();
        p.tasks[value->row] = std::move(after);
        ++p.revision;
        updateTaskDirty(value->row, before);
        taskChange_ = TaskChange{p.revision - 1, p.revision, value->row, false};
    } else {
        if (project_->revision >= 9007199254740991ULL)
            throw AppError(ErrorCode::PROJECT_ID_REVISION_INVALID, {});
        auto p = *project_;
        std::get<std::shared_ptr<const InputDelta>>(step.change)->apply(p, forward);
        ++p.revision;
        if (step.planOnly)
            validatePlan(p);
        else
            validate(p);
        project_ = std::move(p);
        taskChange_.reset();
        updateDirty();
    }
    staleCalculation();
}
bool Document::historyNeedsWorker(bool redo) const {
    return redo ? canRedo() && history_[historyCursor_].planOnly : canUndo() && history_[historyCursor_ - 1].planOnly;
}
void Document::undo() {
    if (!canUndo())
        throw AppError(ErrorCode::NOTHING_TO_UNDO, {});
    applyHistory(history_[historyCursor_ - 1], false);
    --historyCursor_;
    notify(Event::Edited);
}
void Document::redo() {
    if (!canRedo())
        throw AppError(ErrorCode::NOTHING_TO_REDO, {});
    applyHistory(history_[historyCursor_], true);
    ++historyCursor_;
    notify(Event::Edited);
}
std::uint64_t Document::subscribe(std::function<void(Event)> listener) {
    auto token = ++listenerToken_;
    listeners_.emplace(token, std::move(listener));
    return token;
}
void Document::unsubscribe(std::uint64_t token) {
    listeners_.erase(token);
}
void Document::notify(Event event) noexcept {
    // Observers schedule work only; a failed observer cannot roll back a committed edit.
    try {
        std::vector<std::uint64_t> tokens;
        for (const auto &[id, listener] : listeners_) {
            (void)listener;
            tokens.push_back(id);
        }
        for (auto id : tokens) {
            auto found = listeners_.find(id);
            if (found == listeners_.end())
                continue;
            auto listener = found->second;
            try {
                listener(event);
            } catch (...) {
            }
        }
    } catch (...) {
    }
}
RecoverySnapshot Document::recoverySnapshot(std::string utc) const {
    return recoverySnapshotRequest(std::move(utc))();
}
std::function<RecoverySnapshot()> Document::recoverySnapshotRequest(std::string utc) const {
    if (!isEditable() || !dirty_ || !validUtcStamp(utc))
        throw AppError(ErrorCode::RECOVERY_SNAPSHOT_INVALID, {});
    return
        [input = snapshot(), path = path_, fingerprint = fingerprint_, captured = std::move(utc), readOnly = readOnly_,
         saveAs = requiresSaveAs_, saved = savedState_ ? std::optional{savedState_->revision} : std::nullopt] {
            return RecoverySnapshot{*input, path, fingerprint, captured, readOnly, saveAs, saved};
        };
}
void Document::recover(const RecoverySnapshot &snapshot) {
    validate(snapshot.project);
    if (!validUtcStamp(snapshot.capturedAt))
        throw AppError(ErrorCode::RECOVERY_TIMESTAMP_INVALID, {});
    auto p = snapshot.project;
    p.derivedFrom = p.documentId;
    p.documentId = store_.newId();
    validate(p);
    RecoveryOrigin origin{snapshot.project.documentId, snapshot.sourcePath, snapshot.capturedAt,
                          snapshot.project.revision, snapshot.lastSavedRevision};
    project_ = std::move(p);
    path_.clear();
    fingerprint_.clear();
    lease_.reset();
    savedState_.reset();
    calculation_.reset();
    ++calculationGeneration_;
    dirty_ = true;
    dirtyState_ = {};
    taskChange_.reset();
    readOnly_ = false;
    requiresSaveAs_ = true;
    recoveryOrigin_ = std::move(origin);
    clearHistory("복구한 입력에서 새 편집 이력을 시작합니다. 복구 전 실행 취소 이력은 포함하지 않습니다.");
    ++sessionGeneration_;
    notify(Event::Replaced);
}
} // namespace jobs
