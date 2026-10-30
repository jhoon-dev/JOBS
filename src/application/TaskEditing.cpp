#include "domain/Diagnostics.h"
#include "application/Document.h"
#include "calculation/Effort.h"
#include <algorithm>
#include <deque>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
namespace jobs {
namespace {
std::unordered_set<std::string> selected(const Project &p, const std::vector<std::string> &ids) {
    if (ids.empty() || ids.size() > 10000)
        throw AppError(ErrorCode::TASK_SELECTION_LIMIT, {});
    std::unordered_set<std::string> available, result;
    for (const auto &t : p.tasks) {
        calculationCheckpoint();
        available.insert(t.id);
    }
    for (const auto &id : ids)
        if (!available.contains(id) || !result.insert(id).second)
            throw AppError(ErrorCode::TASK_SELECTION_STALE, {});
    return result;
}
void validateGraph(const Project &p) {
    std::unordered_map<std::string, std::size_t> ids;
    for (std::size_t i = 0; i < p.tasks.size(); ++i)
        ids[p.tasks[i].id] = i;
    std::vector<std::vector<std::size_t>> next(p.tasks.size());
    std::vector<std::size_t> degree(p.tasks.size());
    std::deque<std::size_t> ready;
    for (std::size_t i = 0; i < p.tasks.size(); ++i) {
        calculationCheckpoint();
        for (const auto &prev : p.tasks[i].predecessors) {
            if (!ids.contains(prev) || ids.at(prev) == i)
                throw AppError(ErrorCode::PREDECESSOR_REFERENCE_INVALID, {diagnosticArgument(p.tasks[i].name)});
            next[ids.at(prev)].push_back(i);
            ++degree[i];
        }
        if (!degree[i])
            ready.push_back(i);
    }
    std::size_t count = 0;
    while (!ready.empty()) {
        calculationCheckpoint();
        const auto i = ready.front();
        ready.pop_front();
        ++count;
        for (auto j : next[i])
            if (!--degree[j])
                ready.push_back(j);
    }
    if (count != p.tasks.size())
        throw AppError(ErrorCode::DEPENDENCY_CYCLE, {});
}
} // namespace
std::vector<std::string> Document::insertTasks(const std::vector<std::string> &names, const std::string &beforeId,
                                               const std::string &wbsId) {
    requireEditable();
    if (names.empty() || names.size() > 10000)
        throw AppError(ErrorCode::TASK_INSERT_LIMIT, {});
    auto p = *project_;
    auto position = p.tasks.end();
    if (!beforeId.empty()) {
        position = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto &t) { return t.id == beforeId; });
        if (position == p.tasks.end())
            throw AppError(ErrorCode::INSERT_REFERENCE_STALE, {});
    }
    std::vector<Task> added;
    std::vector<std::string> ids;
    for (const auto &name : names) {
        calculationCheckpoint();
        auto id = store_.newId();
        ids.push_back(id);
        added.push_back({id, name, std::nullopt, {}, wbsId});
    }
    p.tasks.insert(position, added.begin(), added.end());
    editPlan(std::move(p), "작업 일괄 삽입");
    return ids;
}
TaskRemoval Document::previewRemoveTasks(const std::vector<std::string> &ids) const {
    if (!project_)
        throw AppError(ErrorCode::DOCUMENT_NOT_OPEN, {});
    return previewRemoveTasks(*project_, ids);
}
TaskRemoval Document::previewRemoveTasks(const Project &project, const std::vector<std::string> &ids) {
    const auto targets = selected(project, ids);
    TaskRemoval out;
    out.tasks = targets.size();
    std::unordered_set<std::string> protectedIds;
    for (const auto &e : project.progress)
        if (targets.contains(e.taskId))
            protectedIds.insert(e.taskId);
    for (const auto &e : project.costEntries)
        if (targets.contains(e.taskId))
            protectedIds.insert(e.taskId);
    for (const auto &t : project.tasks) {
        calculationCheckpoint();
        if (protectedIds.contains(t.id))
            out.protectedTaskIds.push_back(t.id);
        for (const auto &prev : t.predecessors)
            if (targets.contains(t.id) || targets.contains(prev))
                ++out.links;
    }
    for (const auto &a : project.assignments)
        if (targets.contains(a.taskId))
            ++out.assignments;
    for (const auto &c : project.taskCosts)
        if (targets.contains(c.taskId))
            ++out.costs;
    return out;
}
void Document::removeTasks(const std::vector<std::string> &ids) {
    requireEditable();
    const auto summary = previewRemoveTasks(ids);
    if (!summary.protectedTaskIds.empty())
        throw AppError(ErrorCode::TASK_HAS_FACTS, {});
    auto p = *project_;
    const auto targets = selected(p, ids);
    std::erase_if(p.tasks, [&](const auto &t) { return targets.contains(t.id); });
    for (auto &t : p.tasks) {
        std::erase_if(t.predecessors, [&](const auto &id) { return targets.contains(id); });
        std::erase_if(t.dependencyRules, [&](const auto &d) { return targets.contains(d.predecessorId); });
    }
    std::erase_if(p.assignments, [&](const auto &a) { return targets.contains(a.taskId); });
    std::erase_if(p.taskCosts, [&](const auto &c) { return targets.contains(c.taskId); });
    editPlan(std::move(p), "선택 작업 일괄 삭제");
}
std::vector<std::string> Document::duplicateTasks(const std::vector<std::string> &ids) {
    requireEditable();
    auto p = *project_;
    const auto targets = selected(p, ids);
    std::unordered_map<std::string, std::string> copies;
    std::size_t insertion = 0;
    std::vector<std::string> created;
    for (std::size_t i = 0; i < p.tasks.size(); ++i)
        if (targets.contains(p.tasks[i].id)) {
            auto id = store_.newId();
            copies[p.tasks[i].id] = id;
            created.push_back(id);
            insertion = i + 1;
        }
    std::vector<Task> added;
    for (const auto &t : p.tasks)
        if (targets.contains(t.id)) {
            auto copy = t;
            copy.id = copies.at(t.id);
            copy.name += " (복사)";
            for (auto &prev : copy.predecessors)
                if (copies.contains(prev))
                    prev = copies.at(prev);
            for (auto &edge : copy.dependencyRules)
                if (copies.contains(edge.predecessorId))
                    edge.predecessorId = copies.at(edge.predecessorId);
            added.push_back(std::move(copy));
        }
    p.tasks.insert(p.tasks.begin() + std::ptrdiff_t(insertion), added.begin(), added.end());
    const auto assignments = p.assignments;
    for (auto a : assignments)
        if (copies.contains(a.taskId)) {
            a.taskId = copies.at(a.taskId);
            p.assignments.push_back(std::move(a));
        }
    const auto costs = p.taskCosts;
    for (auto c : costs)
        if (copies.contains(c.taskId)) {
            c.taskId = copies.at(c.taskId);
            p.taskCosts.push_back(std::move(c));
        }
    editPlan(std::move(p), "선택 작업 일괄 복제");
    return created;
}
void Document::editTasks(const std::vector<TaskEdit> &changes) {
    requireEditable();
    if (changes.empty() || changes.size() > 10000)
        throw AppError(ErrorCode::TASK_EDIT_LIMIT, {});
    auto p = *project_;
    std::unordered_map<std::string, std::size_t> indices;
    for (std::size_t i = 0; i < p.tasks.size(); ++i)
        indices[p.tasks[i].id] = i;
    std::unordered_set<std::string> seen;
    std::map<std::size_t, const TaskEdit *> ordered;
    bool graphChanged = false;
    for (const auto &e : changes) {
        calculationCheckpoint();
        if (!indices.contains(e.taskId) || !seen.insert(e.taskId).second)
            throw AppError(ErrorCode::TASK_EDIT_TARGET_INVALID, {});
        const auto i = indices.at(e.taskId);
        auto &t = p.tasks[i];
        ordered[i] = &e;
        if (e.name)
            t.name = *e.name;
        if (e.edges) {
            graphChanged = true;
            t.predecessors.clear();
            t.dependencyRules.clear();
            for (const auto &edge : *e.edges) {
                t.predecessors.push_back(edge.predecessorId);
                if (edge.type != DependencyType::FS || edge.lagMinutes)
                    t.dependencyRules.push_back(edge);
            }
            if (!t.dependencyRules.empty() && !isPdm(p))
                p.calculationProfile = pdmProfile;
        }
    }
    // Build the complete proposal before an inverse calculation requires a valid plan.
    for (const auto &[i, e] : ordered)
        if (e->duration) {
            if (p.tasks[i].taskType == TaskType::FixedWork)
                p.tasks[i].durationMinutes = *e->duration;
            else
                changeDuration(p, i, *e->duration);
        }
    validatePlan(p);
    if (graphChanged)
        validateGraph(p);
    EvaluationScope evaluation;
    std::vector<std::size_t> inverse;
    for (const auto &[i, e] : ordered)
        if (e->duration && p.tasks[i].taskType == TaskType::FixedWork)
            inverse.push_back(i);
    changeFixedWorkDurations(p, inverse);
    editPlan(std::move(p), "표 일괄 붙여넣기", inverse.empty());
}
void Document::editPlan(Project p, std::string label, bool validated) {
    // Only the closed task-editing methods call this path. Historical records,
    // document identity, currency and status metadata cannot be modified here.
    if (static_cast<const PlanInput &>(p) == static_cast<const PlanInput &>(*project_))
        return;
    if (p.revision >= 9007199254740991ULL)
        throw AppError(ErrorCode::PROJECT_REVISION_LIMIT, {});
    if (!validated)
        validatePlan(p);
    std::unordered_set<std::string> ids;
    for (const auto &task : p.tasks) {
        calculationCheckpoint();
        ids.insert(task.id);
    }
    for (const auto &entry : p.progress)
        if (!ids.contains(entry.taskId))
            throw std::logic_error("실적 작업을 삭제할 수 없습니다.");
    for (const auto &entry : p.costEntries)
        if (!ids.contains(entry.taskId))
            throw std::logic_error("원가 작업을 삭제할 수 없습니다.");
    ++p.revision;
    InputDelta delta(*project_, p);
    const auto weight = sizeof(HistoryStep) + delta.bytes() + label.size();
    auto history = HistoryStep{std::make_shared<const InputDelta>(std::move(delta)), std::move(label), weight, true};
    // No cancellation points after the first mutation of the live document.
    calculationCheckpoint();
    pushHistory(std::move(history));
    project_ = std::move(p);
    taskChange_.reset();
    updateDirty();
    staleCalculation();
    notify(Event::Edited);
}

} // namespace jobs
