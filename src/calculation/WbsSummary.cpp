#include "calculation/Scheduler.h"
#include "domain/Wbs.h"
#include "calculation/WorkCalendar.h"
#include <unordered_map>
namespace jobs {
WbsSummary summarizeWbs(const Project& p, const ScheduleResult* result, const std::string& id) {
    EvaluationScope evaluation;
    WbsSummary summary;
    const bool valid = result && result->status == CalculationStatus::Valid && result->documentId == p.documentId &&
        result->inputRevision == p.revision && result->profile == p.calculationProfile;
    std::unordered_map<std::string, const TaskSchedule*> dates;
    if (valid) for (const auto& row : result->tasks) dates.emplace(row.taskId, &row);
    bool complete = valid;
    std::unordered_map<std::string,const TaskSchedule*> durationDates;
    if (valid) {
        const auto& durationRows = isEffort(p) && result->isForecast ? result->plannedTasks : result->tasks;
        for (const auto& row : durationRows) durationDates[row.taskId] = &row;
    }
    for (auto index : wbsTaskRows(p, id)) {
        const auto& task = p.tasks[index]; ++summary.taskCount;
        if(isEffort(p)&&usesWork(task)) {
            if(durationDates.contains(task.id)){const auto& r=*durationDates.at(task.id);WorkCalendar cal(p,task);summary.knownDurationMinutes+=cal.offset(r.finish)-cal.offset(r.start);}else ++summary.missingDurations;
        } else if (task.durationMinutes) summary.knownDurationMinutes += *task.durationMinutes; else ++summary.missingDurations;
        auto it = dates.find(task.id);
        if (it == dates.end()) { complete = false; continue; }
        const auto& row = *it->second;
        if (!summary.start || row.start < *summary.start) summary.start = row.start;
        if (!summary.finish || row.finish > *summary.finish) summary.finish = row.finish;
    }
    if (!complete) { summary.start.reset(); summary.finish.reset(); }
    return summary;
}
}
