#include "domain/Diagnostics.h"
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
ScheduleResult calculatePlan(const Project &p) {
    EvaluationScope evaluation;
    calculationCheckpoint();
    ScheduleResult result;
    result.documentId = p.documentId;
    result.inputRevision = p.revision;
    result.profile = p.calculationProfile;
    result.engineVersion = engineFor(p);
    try {
        validate(p);
    } catch (const std::exception &e) {
        result.status = CalculationStatus::Conflict;
        result.diagnostics.push_back(diagnosticFromException(e, "INVALID_INPUT", {}));
        return result;
    }
    if (p.tasks.empty()) {
        result.diagnostics.push_back({"NO_TASKS", {}, {}, "NO_TASKS"});
        return result;
    }
    const auto n = p.tasks.size();
    calculationProgress(CalculationStage::Network, 0, n);
    std::unordered_map<std::string, std::size_t> ids;
    for (std::size_t i = 0; i < n; ++i)
        ids.emplace(p.tasks[i].id, i);
    std::vector<std::vector<std::size_t>> successors(n), predecessors(n);
    std::vector<std::size_t> degree(n), order;
    std::unordered_set<std::string> assignedTasks;
    for (const auto &a : p.assignments)
        assignedTasks.insert(a.taskId);
    bool missing = false, invalid = false;
    for (std::size_t i = 0; i < n; ++i) {
        calculationCheckpoint();
        const auto &task = p.tasks[i];
        if (!task.durationMinutes && !(isEffort(p) && usesWork(task))) {
            missing = true;
            result.diagnostics.push_back(
                {"MISSING_DURATION", {diagnosticArgument(task.name)}, {task.id}, "MISSING_DURATION"});
        }
        if (isEffort(p) && usesWork(task) && task.workUnitMinutes.value_or(0) > 0 && !assignedTasks.contains(task.id)) {
            missing = true;
            result.diagnostics.push_back(
                {"MISSING_WORK_ASSIGNMENT", {diagnosticArgument(task.name)}, {task.id}, "MISSING_WORK_ASSIGNMENT"});
        }
        for (const auto &id : task.predecessors) {
            const auto found = ids.find(id);
            if (found == ids.end() || found->second == i) {
                invalid = true;
                result.diagnostics.push_back(
                    {"INVALID_DEPENDENCY", {diagnosticArgument(task.name)}, {task.id, id}, "INVALID_DEPENDENCY"});
            } else {
                predecessors[i].push_back(found->second);
                successors[found->second].push_back(i);
                ++degree[i];
            }
        }
    }
    std::deque<std::size_t> ready;
    for (std::size_t i = 0; i < n; ++i)
        if (degree[i] == 0)
            ready.push_back(i);
    while (!ready.empty()) {
        calculationCheckpoint();
        auto i = ready.front();
        ready.pop_front();
        order.push_back(i);
        for (auto next : successors[i])
            if (--degree[next] == 0)
                ready.push_back(next);
    }
    if (order.size() != n) {
        // Follow predecessors inside the unprocessed graph to return an actual cycle, not downstream nodes.
        std::size_t current = 0;
        while (degree[current] == 0)
            ++current;
        std::unordered_map<std::size_t, std::size_t> seen;
        std::vector<std::size_t> chain;
        while (!seen.contains(current)) {
            seen[current] = chain.size();
            chain.push_back(current);
            for (auto prev : predecessors[current])
                if (degree[prev] > 0) {
                    current = prev;
                    break;
                }
        }
        std::vector<std::string> cycle;
        for (std::size_t j = seen[current]; j < chain.size(); ++j)
            cycle.push_back(p.tasks[chain[j]].id);
        cycle.push_back(p.tasks[current].id);
        std::reverse(cycle.begin(), cycle.end());
        result.diagnostics.push_back({"DEPENDENCY_CYCLE", {}, std::move(cycle), "DEPENDENCY_CYCLE_2"});
        invalid = true;
    }
    if (invalid || missing) {
        result.status = invalid ? CalculationStatus::Conflict : CalculationStatus::Incomplete;
        return result;
    }
    if (isEffort(p))
        return calculateEffort(p);
    if (p.calculationProfile == constraintProfile)
        return calculateConstrainedPlan(p, order);
    if (isPdm(p))
        return calculatePdmPlan(p, order);
    std::vector<TaskSchedule> rows(n);
    std::int64_t finish = 0;
    std::size_t completed = 0;
    for (auto i : order) {
        calculationProgress(CalculationStage::Forward, completed++, n);
        auto &row = rows[i];
        row.taskId = p.tasks[i].id;
        for (auto prev : predecessors[i])
            row.earlyStartMinutes = std::max(row.earlyStartMinutes, rows[prev].earlyFinishMinutes);
        row.earlyFinishMinutes = row.earlyStartMinutes + *p.tasks[i].durationMinutes;
        finish = std::max(finish, row.earlyFinishMinutes);
        for (auto prev : predecessors[i])
            if (rows[prev].earlyFinishMinutes == row.earlyStartMinutes)
                row.drivingPredecessors.push_back(p.tasks[prev].id);
    }
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        calculationProgress(CalculationStage::Backward, std::size_t(it - order.rbegin()), n);
        auto i = *it;
        auto &row = rows[i];
        row.lateFinishMinutes = finish;
        auto freeEnd = finish;
        for (auto next : successors[i]) {
            row.lateFinishMinutes = std::min(row.lateFinishMinutes, rows[next].lateStartMinutes);
            freeEnd = std::min(freeEnd, rows[next].earlyStartMinutes);
        }
        row.lateStartMinutes = row.lateFinishMinutes - *p.tasks[i].durationMinutes;
        row.totalFloatMinutes = row.lateStartMinutes - row.earlyStartMinutes;
        row.freeFloatMinutes = freeEnd - row.earlyFinishMinutes;
        row.critical = row.totalFloatMinutes == 0;
    }
    try {
        WorkCalendar calendar(p);
        for (std::size_t i = 0; i < n; ++i) {
            rows[i].start = calendar.boundary(rows[i].earlyStartMinutes, *p.tasks[i].durationMinutes == 0);
            rows[i].finish = calendar.boundary(rows[i].earlyFinishMinutes, true);
        }
        result.projectFinish = calendar.boundary(finish, true);
    } catch (const std::exception &e) {
        result.status = CalculationStatus::Conflict;
        result.diagnostics.push_back(diagnosticFromException(e, "NO_AVAILABILITY_IN_HORIZON", {}));
        return result;
    }
    result.tasks = std::move(rows);
    result.status = CalculationStatus::Valid;
    result.diagnostics.push_back({"RESOURCE_NOT_CHECKED", {}, {}, "RESOURCE_NOT_CHECKED"});
    return result;
}
} // namespace jobs
