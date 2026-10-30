#include "domain/Diagnostics.h"
#include "formatting/PlanTokens.h"
#include "calculation/Tracking.h"
#include "calculation/WorkCalendar.h"
#include <algorithm>
#include <stdexcept>
#include <unordered_map>
namespace jobs {
namespace {
Project baselineProject(const Baseline &b) {
    Project p;
    static_cast<PlanInput &>(p) = b.input;
    p.documentId = b.sourceDocumentId;
    p.revision = b.inputRevision;
    return p;
}
std::optional<std::int64_t> delta(const WorkCalendar &calendar, const std::optional<std::string> &old,
                                  const std::optional<std::string> &now) {
    if (!old || !now)
        return std::nullopt;
    try {
        return calendar.offset(*now) - calendar.offset(*old);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}
std::vector<std::string> sorted(std::vector<std::string> input) {
    std::sort(input.begin(), input.end());
    return input;
}
std::vector<std::string> assignments(const PlanInput &p, const std::string &taskId, bool names) {
    std::vector<std::string> result;
    for (const auto &a : p.assignments)
        if (a.taskId == taskId) {
            std::string label;
            if (names)
                for (const auto &r : p.resources)
                    if (r.id == a.resourceId)
                        label = r.name + " ";
            result.push_back(label + "[" + a.resourceId + "] " + std::to_string(a.units) + "%");
        }
    return sorted(std::move(result));
}
std::string wbsPath(const PlanInput &p, std::string id) {
    std::unordered_map<std::string, const WbsNode *> nodes;
    for (const auto &n : p.wbsNodes)
        nodes.emplace(n.id, &n);
    std::vector<std::string> path;
    while (!id.empty()) {
        path.push_back(nodes.at(id)->name);
        id = nodes.at(id)->parentId;
    }
    std::string result;
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        if (!result.empty())
            result += " / ";
        result += *it;
    }
    return result.empty() ? "프로젝트 루트/미분류" : result;
}
} // namespace
Baseline prepareBaseline(const Project &p, std::string name, std::string author, std::string reason, std::string utc) {
    auto plan = calculatePlan(p);
    if (plan.status != CalculationStatus::Valid)
        throw AppError(ErrorCode::BASELINE_REQUIRES_VALID_PLAN, {});
    Baseline b;
    b.name = std::move(name);
    b.approvedBy = std::move(author);
    b.reason = std::move(reason);
    b.createdAt = std::move(utc);
    b.sourceDocumentId = p.documentId;
    b.inputRevision = p.revision;
    b.input = static_cast<const PlanInput &>(p);
    b.engineVersion = plan.engineVersion;
    b.projectFinish = *plan.projectFinish;
    b.schedule = std::move(plan.tasks);
    return b;
}
void verifyBaseline(const Baseline &b) {
    auto expected = calculatePlan(baselineProject(b), b.engineVersion);
    if (expected.status != CalculationStatus::Valid || expected.tasks != b.schedule ||
        expected.projectFinish != b.projectFinish || expected.engineVersion != b.engineVersion)
        throw AppError(ErrorCode::BASELINE_RESULT_MISMATCH, {});
}
ComparisonReport compareToBaseline(const Project &p, const std::string &baselineId) {
    ComparisonReport report;
    report.documentId = p.documentId;
    report.revision = p.revision;
    report.plan = calculatePlan(p);
    report.forecast = calculate(p);
    report.totalTasks = p.tasks.size();
    const Baseline *baseline = nullptr;
    for (const auto &b : p.baselines)
        if (b.id == baselineId)
            baseline = &b;
    if (!baselineId.empty() && !baseline)
        throw AppError(ErrorCode::BASELINE_NOT_FOUND, {}, {}, "BASELINE_NOT_FOUND_2");
    std::optional<WorkCalendar> calendar;
    std::unordered_map<std::string, const Task *> before;
    std::unordered_map<std::string, const TaskSchedule *> baselineDates, planDates, forecastDates;
    if (baseline) {
        report.baselineId = baseline->id;
        report.baselineFinish = baseline->projectFinish;
        calendar.emplace(baseline->input, Task{});
        for (const auto &t : baseline->input.tasks)
            before.emplace(t.id, &t);
        for (const auto &row : baseline->schedule)
            baselineDates.emplace(row.taskId, &row);
        report.planFinishVariance = delta(*calendar, report.baselineFinish, report.plan.projectFinish);
        report.forecastFinishVariance = delta(*calendar, report.baselineFinish, report.forecast.projectFinish);
        if (baseline->input.startDate != p.startDate)
            report.contextChanges.push_back("프로젝트 시작일 변경");
        if (baseline->input.calendar != p.calendar || sorted(baseline->input.holidays) != sorted(p.holidays) ||
            baseline->input.calendars != p.calendars)
            report.contextChanges.push_back("달력/휴일 변경");
        if (baseline->input.calculationProfile != p.calculationProfile)
            report.contextChanges.push_back("계산 정책 변경");
        if (baseline->engineVersion != report.plan.engineVersion)
            report.contextChanges.push_back("계산 엔진 변경: " + baseline->engineVersion + " → " +
                                            report.plan.engineVersion);
        if (baseline->input.wbsNodes != p.wbsNodes)
            report.contextChanges.push_back("프로젝트 WBS 정의 변경");
        if (baseline->input.resources != p.resources)
            report.contextChanges.push_back("자원 정의/날짜별 가용량 변경");
        if (baseline->input.currency != p.currency || baseline->input.costProfile != p.costProfile)
            report.contextChanges.push_back("통화/원가 정책 변경");
        if (baseline->input.taskCosts != p.taskCosts)
            report.contextChanges.push_back("작업 고정비/비용 발생/EV 측정법 변경");
    }
    if (report.plan.status == CalculationStatus::Valid)
        for (const auto &row : report.plan.tasks)
            planDates.emplace(row.taskId, &row);
    if (report.forecast.status == CalculationStatus::Valid)
        for (const auto &row : report.forecast.tasks)
            forecastDates.emplace(row.taskId, &row);
    for (const auto &t : p.tasks) {
        TaskComparison row;
        row.taskId = t.id;
        row.name = t.name;
        row.planDuration = t.durationMinutes;
        row.planPredecessors = dependencyLabels(t);
        row.planWbsPath = wbsPath(p, t.wbsId);
        if (isEffort(p) && usesWork(t)) {
            row.planDuration.reset();
            if (planDates.contains(t.id)) {
                WorkCalendar cal(p, t);
                row.planDuration = cal.offset(planDates.at(t.id)->finish) - cal.offset(planDates.at(t.id)->start);
            }
        }
        row.planAssignments = assignments(p, t.id, true);
        if (baselineDates.contains(t.id)) {
            row.baselineStart = baselineDates.at(t.id)->start;
            row.baselineFinish = baselineDates.at(t.id)->finish;
        }
        if (planDates.contains(t.id)) {
            row.planStart = planDates.at(t.id)->start;
            row.planFinish = planDates.at(t.id)->finish;
        }
        if (forecastDates.contains(t.id)) {
            row.forecastStart = forecastDates.at(t.id)->start;
            row.forecastFinish = forecastDates.at(t.id)->finish;
        }
        const auto *entry = progressAsOf(p, t.id);
        if (entry) {
            row.reported = true;
            ++report.reportedTasks;
            row.actualStart = entry->actualStart;
            row.actualFinish = entry->actualFinish;
            row.actualMinutes = entry->actualDurationMinutes;
            row.remainingMinutes = entry->remainingMinutes;
            if (entry->actualFinish)
                row.durationProgressPercent = 100.0;
            else if (!usesWork(t) && !(entry->remainingMinutes == 0 && entry->remainingWorkUnitMinutes) &&
                     entry->actualDurationMinutes + entry->remainingMinutes > 0)
                row.durationProgressPercent = 100.0 * double(entry->actualDurationMinutes) /
                                              double(entry->actualDurationMinutes + entry->remainingMinutes);
        } else
            row.remainingMinutes = t.durationMinutes;
        if (usesWork(t) && (!entry || !entry->actualFinish))
            row.remainingMinutes.reset();
        if (baseline) {
            if (!before.contains(t.id))
                row.changes.push_back("추가된 작업");
            else {
                const auto &old = *before.at(t.id);
                row.baselineName = old.name;
                row.baselineDuration = old.durationMinutes;
                if (isEffort(baseline->input) && usesWork(old)) {
                    WorkCalendar cal(baseline->input, old);
                    row.baselineDuration = cal.offset(*row.baselineFinish) - cal.offset(*row.baselineStart);
                }
                row.baselinePredecessors = dependencyLabels(old);
                row.baselineWbsPath = wbsPath(baseline->input, old.wbsId);
                row.baselineAssignments = assignments(baseline->input, t.id, true);
                if (old.name != t.name)
                    row.changes.push_back("이름 변경");
                if (old.durationMinutes != t.durationMinutes)
                    row.changes.push_back("계획 기간 변경");
                if (sorted(dependencyLabels(old)) != sorted(dependencyLabels(t)))
                    row.changes.push_back("선행관계 변경");
                if (old.calendarId != t.calendarId)
                    row.changes.push_back("작업 달력 배정 변경");
                if (old.constraintType != t.constraintType || old.constraintAt != t.constraintAt)
                    row.changes.push_back("작업 날짜 제약 변경");
                if (old.deadlineAt != t.deadlineAt)
                    row.changes.push_back("마감일 변경");
                if (old.taskType != t.taskType || old.workDriven != t.workDriven ||
                    old.workUnitMinutes != t.workUnitMinutes)
                    row.changes.push_back("작업 유형·공수·계산 기준 변경");
                if (old.breaks != t.breaks)
                    row.changes.push_back("작업 분할 중단 구간 변경");
                if (old.wbsId != t.wbsId)
                    row.changes.push_back("WBS 소속 변경");
                if (assignments(baseline->input, t.id, false) != assignments(p, t.id, false))
                    row.changes.push_back("자원 배정 변경");
            }
            if (before.contains(t.id)) {
                WorkCalendar taskCalendar(baseline->input, *before.at(t.id));
                row.planFinishVariance = delta(taskCalendar, row.baselineFinish, row.planFinish);
                row.forecastFinishVariance = delta(taskCalendar, row.baselineFinish, row.forecastFinish);
            }
            before.erase(t.id);
        }
        report.tasks.push_back(std::move(row));
    }
    if (baseline)
        for (const auto &old : baseline->input.tasks)
            if (before.contains(old.id)) {
                TaskComparison row;
                row.taskId = old.id;
                row.name = old.name;
                row.baselineStart = baselineDates.at(old.id)->start;
                row.baselineName = old.name;
                row.baselineDuration = old.durationMinutes;
                row.baselinePredecessors = dependencyLabels(old);
                row.baselineWbsPath = wbsPath(baseline->input, old.wbsId);
                row.baselineAssignments = assignments(baseline->input, old.id, true);
                row.baselineFinish = baselineDates.at(old.id)->finish;
                if (isEffort(baseline->input) && usesWork(old)) {
                    WorkCalendar cal(baseline->input, old);
                    row.baselineDuration = cal.offset(*row.baselineFinish) - cal.offset(*row.baselineStart);
                }
                row.changes.push_back("현재 계획에서 삭제된 작업");
                report.tasks.push_back(std::move(row));
            }
    return report;
}
} // namespace jobs
