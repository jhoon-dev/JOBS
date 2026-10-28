#include "calculation/SharedResources.h"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <map>
#include <set>

namespace jobs {
namespace {
using Minute = std::int64_t;
using namespace std::chrono;
Minute civil(const std::string &stamp) {
    if (!validStamp(stamp))
        throw AppError(ErrorCode::CIVIL_TIMESTAMP_INVALID);
    const auto date = year{std::stoi(stamp.substr(0, 4))} / unsigned(std::stoi(stamp.substr(5, 2))) /
                      unsigned(std::stoi(stamp.substr(8, 2)));
    return sys_days{date}.time_since_epoch().count() * 1440 + std::stoi(stamp.substr(11, 2)) * 60 +
           std::stoi(stamp.substr(14, 2));
}
Minute dayOf(Minute at) {
    auto day = at / 1440;
    return at % 1440 < 0 ? day - 1 : day;
}
std::string dateText(Minute day) {
    const year_month_day date{sys_days{days{day}}};
    char text[16];
    std::snprintf(text, sizeof(text), "%04d-%02u-%02u", int(date.year()), unsigned(date.month()), unsigned(date.day()));
    return text;
}
struct Occupancy {
    Minute start, finish;
    int units;
    ProjectTaskRef task;
};
} // namespace
SharedResourceResult calculateSharedResources(const Workspace &workspace,
                                              const std::vector<std::shared_ptr<const Project>> &snapshots,
                                              bool forecast, const std::shared_ptr<CalculationControl> &control) {
    EvaluationScope evaluation(control);
    calculationCheckpoint();
    validateWorkspace(workspace);
    SharedResourceResult out;
    out.forecast = forecast;
    out.status = CalculationStatus::Valid;
    const auto addDiagnostic = [&](Diagnostic issue) {
        if (out.diagnostics.size() < 1000)
            out.diagnostics.push_back(std::move(issue));
        else
            ++out.omittedDiagnostics;
    };
    std::map<std::string, std::shared_ptr<const Project>> projects;
    for (const auto &project : snapshots) {
        if (!project || !projects.emplace(project->documentId, project).second)
            throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {project ? project->documentId : "null"});
    }
    std::map<std::string, const SharedResource *> pools;
    for (const auto &resource : workspace.resources)
        pools[resource.id] = &resource;
    std::map<std::pair<std::string, std::string>, std::string> bindings;
    for (const auto &binding : workspace.bindings)
        bindings[{binding.projectId, binding.resourceId}] = binding.sharedId;
    std::map<std::string, std::vector<Occupancy>> occupied;
    for (const auto &reference : workspace.projects) {
        EvaluationScope projectScope(control, std::make_shared<EvaluationCache>());
        calculationCheckpoint();
        const auto found = projects.find(reference.documentId);
        if (found == projects.end()) {
            out.status = CalculationStatus::Incomplete;
            Diagnostic issue{
                "WORKSPACE_PROJECT_NOT_OPEN", {reference.name.empty() ? reference.documentId : reference.name}, {}};
            issue.projectId = reference.documentId;
            addDiagnostic(std::move(issue));
            out.projects.push_back({reference.documentId, reference.name});
            continue;
        }
        const auto &project = *found->second;
        const auto schedule = forecast ? calculate(project) : calculatePlan(project);
        out.projects.push_back({project.documentId, project.name, project.revision, schedule.status,
                                schedule.projectFinish, project.tasks.size()});
        for (auto diagnostic : schedule.diagnostics) {
            diagnostic.projectId = project.documentId;
            addDiagnostic(std::move(diagnostic));
        }
        if (schedule.status != CalculationStatus::Valid) {
            out.status = CalculationStatus::Incomplete;
            continue;
        }
        ++out.includedProjects;
        std::map<std::string, std::string> localTypes;
        for (const auto &local : project.resources) {
            localTypes[local.id] = local.type;
            if (!bindings.contains({project.documentId, local.id}))
                ++out.unmappedResources;
        }
        std::set<std::string> validBindings;
        for (const auto &binding : workspace.bindings)
            if (binding.projectId == project.documentId) {
                if (!localTypes.contains(binding.resourceId) ||
                    localTypes.at(binding.resourceId) != pools.at(binding.sharedId)->type) {
                    Diagnostic issue{"WORKSPACE_RESOURCE_MISSING", {binding.resourceId}, {}};
                    issue.projectId = project.documentId;
                    issue.resourceIds = {binding.resourceId};
                    addDiagnostic(std::move(issue));
                    out.status = CalculationStatus::Incomplete;
                } else
                    validBindings.insert(binding.resourceId);
            }
        if (validBindings.empty())
            continue;
        for (const auto &span : resourceOccupancies(project, schedule, 500000 - out.occupancyCount, &validBindings)) {
            if (!validBindings.contains(span.resourceId))
                continue;
            occupied[bindings.at({project.documentId, span.resourceId})].push_back(
                {civil(span.start), civil(span.finish), span.units, {project.documentId, span.taskId}});
            if (++out.occupancyCount > 500000)
                throw AppError(ErrorCode::WORKSPACE_LIMIT);
        }
    }
    if (workspace.projects.empty()) {
        out.status = CalculationStatus::Incomplete;
        addDiagnostic({"WORKSPACE_NO_PROJECTS", {}, {}});
    }
    if (workspace.resources.empty() || workspace.bindings.empty()) {
        out.status = CalculationStatus::Incomplete;
        addDiagnostic({"WORKSPACE_NO_RESOURCES", {}, {}});
    }
    if (out.unmappedResources)
        addDiagnostic({"WORKSPACE_UNMAPPED_RESOURCES", {std::to_string(out.unmappedResources)}, {}});
    bool overload = false, unknown = false;
    std::size_t taskReferences = 0;
    for (const auto &[id, spans] : occupied) {
        calculationCheckpoint();
        const auto &resource = *pools.at(id);
        std::map<Minute, Minute> events;
        std::map<Minute, std::set<ProjectTaskRef>> tasks;
        for (const auto &span : spans) {
            calculationCheckpoint();
            events[span.start] += span.units;
            events[span.finish] -= span.units;
            for (auto day = dayOf(span.start); day <= dayOf(span.finish - 1); ++day) {
                if (tasks[day].insert(span.task).second && ++taskReferences > 1000000)
                    throw AppError(ErrorCode::WORKSPACE_LIMIT);
            }
        }
        std::map<std::string, std::optional<int>> exceptions;
        for (const auto &item : resource.exceptions)
            exceptions[item.date] = item.units;
        const std::set<std::string> holidays(resource.calendar.holidays.begin(), resource.calendar.holidays.end());
        std::map<Minute, SharedResourceDay> rows;
        auto event = events.begin();
        Minute at = event->first, used = 0;
        const auto end = events.rbegin()->first;
        while (at < end) {
            calculationCheckpoint();
            while (event != events.end() && event->first <= at) {
                used += event->second;
                ++event;
            }
            if (used == 0 && event != events.end()) {
                at = event->first;
                continue;
            }
            const auto day = dayOf(at), minute = at - day * 1440;
            const auto date = dateText(day);
            const auto weekday = std::chrono::weekday{sys_days{days{day}}}.iso_encoding();
            const bool workingDay = !holidays.contains(date) &&
                                    std::find(resource.calendar.weekdays.begin(), resource.calendar.weekdays.end(),
                                              int(weekday)) != resource.calendar.weekdays.end();
            bool working = false;
            auto next = (day + 1) * 1440;
            if (workingDay)
                for (const auto &period : resource.calendar.periods) {
                    if (minute < period.begin)
                        next = std::min(next, day * 1440 + period.begin);
                    else if (minute < period.end) {
                        working = true;
                        next = std::min(next, day * 1440 + period.end);
                    }
                }
            if (event != events.end())
                next = std::min(next, event->first);
            next = std::min(next, end);
            if (next <= at)
                throw AppError(ErrorCode::INTERNAL_ERROR, {"non-increasing shared resource interval"});
            if (used > 0) {
                auto &row = rows[day];
                row.sharedId = id;
                row.date = date;
                row.capacity = exceptions.contains(date) ? exceptions.at(date) : resource.maxUnits;
                const auto capacity = working ? row.capacity : std::optional<int>{0};
                row.peakUnits = std::max(row.peakUnits, used);
                row.demandUnitMinutes += used * (next - at);
                if (!capacity) {
                    row.unknownCapacityMinutes += next - at;
                    unknown = true;
                } else if (used > *capacity) {
                    row.overloadMinutes += next - at;
                    row.excessUnitMinutes += (used - *capacity) * (next - at);
                    overload = true;
                }
            }
            at = next;
        }
        for (auto &[day, row] : rows) {
            row.tasks.assign(tasks.at(day).begin(), tasks.at(day).end());
            out.days.push_back(std::move(row));
            if (out.days.size() > 100000)
                throw AppError(ErrorCode::WORKSPACE_LIMIT);
        }
    }
    if (unknown) {
        out.status = CalculationStatus::Incomplete;
        addDiagnostic({"WORKSPACE_UNKNOWN_CAPACITY", {}, {}});
    }
    if (overload) {
        if (out.status == CalculationStatus::Valid)
            out.status = CalculationStatus::Conflict;
        addDiagnostic({"WORKSPACE_OVERLOAD", {}, {}});
    }
    return out;
}
} // namespace jobs
