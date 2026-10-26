#include "calculation/Scheduler.h"
#include "calculation/WorkCalendar.h"
#include "calculation/Pdm.h"
#include "calculation/Constraints.h"
#include "calculation/Effort.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <stdexcept>

namespace jobs {
ScheduleResult calculatePlan(const Project& p) {
    ScheduleResult result; result.documentId = p.documentId; result.inputRevision = p.revision; result.profile = p.calculationProfile;
    result.engineVersion=engineFor(p);
    try { validate(p); }
    catch (const std::exception& e) {
        result.status = CalculationStatus::Conflict;
        result.diagnostics.push_back({"INVALID_INPUT", e.what(), {}}); return result;
    }
    if (p.tasks.empty()) { result.diagnostics.push_back({"NO_TASKS", "계산할 작업이 없습니다.", {}}); return result; }
    const auto n = p.tasks.size();
    std::unordered_map<std::string, std::size_t> ids;
    for (std::size_t i = 0; i < n; ++i) ids.emplace(p.tasks[i].id, i);
    std::vector<std::vector<std::size_t>> successors(n), predecessors(n);
    std::vector<std::size_t> degree(n), order;
    bool missing = false, invalid = false;
    for (std::size_t i = 0; i < n; ++i) {
        const auto& task = p.tasks[i];
        if (!task.durationMinutes && !(isEffort(p)&&usesWork(task))) {
            missing = true; result.diagnostics.push_back({"MISSING_DURATION", "작업 기간이 미정입니다: " + task.name, {task.id}});
        }
        if(isEffort(p)&&usesWork(task)&&task.workUnitMinutes.value_or(0)>0&&std::none_of(p.assignments.begin(),p.assignments.end(),[&](const auto& a){return a.taskId==task.id;})) {
            missing=true;result.diagnostics.push_back({"MISSING_WORK_ASSIGNMENT","공수 작업에 자원을 배정하세요: "+task.name,{task.id}});
        }
        for (const auto& id : task.predecessors) {
            const auto found = ids.find(id);
            if (found == ids.end() || found->second == i) {
                invalid = true; result.diagnostics.push_back({"INVALID_DEPENDENCY", "존재하지 않거나 자기 자신인 선행 작업입니다: " + task.name, {task.id, id}});
            } else {
                predecessors[i].push_back(found->second); successors[found->second].push_back(i); ++degree[i];
            }
        }
    }
    std::deque<std::size_t> ready;
    for (std::size_t i = 0; i < n; ++i) if (degree[i] == 0) ready.push_back(i);
    while (!ready.empty()) {
        auto i = ready.front(); ready.pop_front(); order.push_back(i);
        for (auto next : successors[i]) if (--degree[next] == 0) ready.push_back(next);
    }
    if (order.size() != n) {
        // Follow predecessors inside the unprocessed graph to return an actual cycle, not downstream nodes.
        std::size_t current = 0;
        while (degree[current] == 0) ++current;
        std::unordered_map<std::size_t, std::size_t> seen;
        std::vector<std::size_t> chain;
        while (!seen.contains(current)) {
            seen[current] = chain.size(); chain.push_back(current);
            for (auto prev : predecessors[current]) if (degree[prev] > 0) { current = prev; break; }
        }
        std::vector<std::string> cycle;
        for (std::size_t j = seen[current]; j < chain.size(); ++j) cycle.push_back(p.tasks[chain[j]].id);
        cycle.push_back(p.tasks[current].id); std::reverse(cycle.begin(), cycle.end());
        result.diagnostics.push_back({"DEPENDENCY_CYCLE", "선행관계 순환이 있습니다. 진단의 작업 ID 경로를 확인하세요.", std::move(cycle)});
        invalid = true;
    }
    if (invalid || missing) {
        result.status = invalid ? CalculationStatus::Conflict : CalculationStatus::Incomplete; return result;
    }
    if(isEffort(p))return calculateEffort(p);
    if (p.calculationProfile == constraintProfile) return calculateConstrainedPlan(p,order);
    if (isPdm(p)) return calculatePdmPlan(p, order);
    std::vector<TaskSchedule> rows(n);
    std::int64_t finish = 0;
    for (auto i : order) {
        auto& row = rows[i]; row.taskId = p.tasks[i].id;
        for (auto prev : predecessors[i]) row.earlyStartMinutes = std::max(row.earlyStartMinutes, rows[prev].earlyFinishMinutes);
        row.earlyFinishMinutes = row.earlyStartMinutes + *p.tasks[i].durationMinutes;
        finish = std::max(finish, row.earlyFinishMinutes);
        for (auto prev : predecessors[i]) if (rows[prev].earlyFinishMinutes == row.earlyStartMinutes)
            row.drivingPredecessors.push_back(p.tasks[prev].id);
    }
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        auto i = *it; auto& row = rows[i]; row.lateFinishMinutes = finish;
        auto freeEnd = finish;
        for (auto next : successors[i]) {
            row.lateFinishMinutes = std::min(row.lateFinishMinutes, rows[next].lateStartMinutes);
            freeEnd = std::min(freeEnd, rows[next].earlyStartMinutes);
        }
        row.lateStartMinutes = row.lateFinishMinutes - *p.tasks[i].durationMinutes;
        row.totalFloatMinutes = row.lateStartMinutes - row.earlyStartMinutes;
        row.freeFloatMinutes = freeEnd - row.earlyFinishMinutes; row.critical = row.totalFloatMinutes == 0;
    }
    try {
        WorkCalendar calendar(p);
        for (std::size_t i = 0; i < n; ++i) {
            rows[i].start = calendar.boundary(rows[i].earlyStartMinutes, *p.tasks[i].durationMinutes == 0);
            rows[i].finish = calendar.boundary(rows[i].earlyFinishMinutes, true);
        }
        result.projectFinish = calendar.boundary(finish, true);
    } catch (const std::exception& e) {
        result.status = CalculationStatus::Conflict;
        result.diagnostics.push_back({"NO_AVAILABILITY_IN_HORIZON", e.what(), {}}); return result;
    }
    result.tasks = std::move(rows); result.status = CalculationStatus::Valid;
    result.diagnostics.push_back({"RESOURCE_NOT_CHECKED", "기간·FS 논리 일정입니다. 자원 가용량은 검증하지 않았습니다.", {}});
    return result;
}
}
