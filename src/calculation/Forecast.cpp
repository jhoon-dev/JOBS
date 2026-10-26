#include "calculation/Scheduler.h"
#include "calculation/WorkCalendar.h"
#include "calculation/Pdm.h"
#include <algorithm>
#include <deque>
#include <unordered_map>
namespace jobs {
ScheduleResult calculate(const Project& p) {
    auto plan = calculatePlan(p);
    if (!p.statusAt) return plan;
    plan.isForecast = true; plan.statusAt = p.statusAt; plan.calculationPolicy = "retained-remaining-v1";
    if (plan.status != CalculationStatus::Valid) return plan;
    if (p.calculationProfile == pdmProfile) return calculatePdmForecast(p, std::move(plan));
    ScheduleResult result = plan; result.tasks.clear(); result.projectFinish.reset(); result.diagnostics.clear();
    try {
        WorkCalendar calendar(p); const auto status = calendar.offset(*p.statusAt);
        const auto n = p.tasks.size(); std::unordered_map<std::string, std::size_t> ids;
        for (std::size_t i = 0; i < n; ++i) ids.emplace(p.tasks[i].id, i);
        std::vector<std::vector<std::size_t>> next(n);
        std::vector<std::size_t> degree(n);
        for (std::size_t i = 0; i < n; ++i) for (const auto& prev : p.tasks[i].predecessors) { next[ids.at(prev)].push_back(i); ++degree[i]; }
        std::deque<std::size_t> ready;
        for (std::size_t i = 0; i < n; ++i) if (!degree[i]) ready.push_back(i);
        std::vector<TaskSchedule> rows(n);
        for (; !ready.empty(); ready.pop_front()) {
            auto i = ready.front(); const auto& task = p.tasks[i]; auto& row = rows[i]; row.taskId = task.id;
            const auto* entry = progressAsOf(p, task.id);
            std::int64_t anchor = status;
            for (const auto& id : task.predecessors) anchor = std::max(anchor, rows[ids.at(id)].earlyFinishMinutes);
            if (entry && entry->actualFinish) {
                row.start = *entry->actualStart; row.finish = *entry->actualFinish; row.completedActual = true;
                row.earlyStartMinutes = calendar.offset(row.start); row.earlyFinishMinutes = calendar.offset(row.finish);
            } else {
                const auto remaining = entry ? entry->remainingMinutes : *task.durationMinutes;
                if (entry && entry->actualStart) {
                    row.start = *entry->actualStart; row.earlyStartMinutes = calendar.offset(row.start);
                    anchor = std::max(anchor, row.earlyStartMinutes);
                } else {
                    row.start = calendar.boundary(anchor, remaining == 0); row.earlyStartMinutes = anchor;
                }
                row.remainingStart = calendar.boundary(anchor, remaining == 0);
                row.earlyFinishMinutes = anchor + remaining;
                row.finish = calendar.boundary(row.earlyFinishMinutes, true);
                // At non-working status times a pending milestone cannot be placed before the status instant.
                if (remaining == 0 && row.finish < *p.statusAt) { row.finish = *p.statusAt; row.remainingStart = row.finish; if (!entry || !entry->actualStart) row.start = row.finish; }
                for (const auto& id : task.predecessors) if (rows[ids.at(id)].earlyFinishMinutes == anchor) row.drivingPredecessors.push_back(id);
            }
            for (auto j : next[i]) if (--degree[j] == 0) ready.push_back(j);
        }
        std::size_t unreported = 0;
        for (std::size_t i = 0; i < n; ++i) {
            const auto* entry = progressAsOf(p, p.tasks[i].id);
            if (!entry) ++unreported;
            if (entry && entry->actualStart) for (const auto& id : p.tasks[i].predecessors)
                if (*entry->actualStart < rows[ids.at(id)].finish)
                    result.diagnostics.push_back({"OUT_OF_SEQUENCE", "실제 시작이 선행 완료보다 빠릅니다. 실제 날짜는 보존하고 잔여에 FS 논리를 적용했습니다.", {id, p.tasks[i].id}});
            if (!result.projectFinish || rows[i].finish > *result.projectFinish) result.projectFinish = rows[i].finish;
        }
        result.tasks = std::move(rows); result.status = CalculationStatus::Valid;
        if (unreported) result.diagnostics.push_back({"UNREPORTED_REMAINING", "미보고 " + std::to_string(unreported) + "개 작업은 현재 계획 기간을 잔여 기간으로 사용했습니다.", {}});
        result.diagnostics.push_back({"FORECAST_POLICY", "상태일 이후 retained remaining logic 예측입니다. 실제 날짜는 고정하며 예측의 여유·임계 및 자원 가용량은 미산정입니다.", {}});
    } catch (const std::exception& e) {
        result.status = CalculationStatus::Conflict; result.tasks.clear(); result.projectFinish.reset();
        result.diagnostics.push_back({"FORECAST_HORIZON", e.what(), {}});
    }
    return result;
}
}
