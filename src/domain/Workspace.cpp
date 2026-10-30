#include "domain/Workspace.h"
#include <set>

namespace jobs {
namespace {
bool text(const std::string &value, std::size_t maximum, bool empty = false) {
    return (empty || !value.empty()) && value.size() <= maximum &&
           value.find_first_of(std::string("\0\r\n", 3)) == std::string::npos;
}
} // namespace
void validateWorkspace(const Workspace &workspace, bool requirePaths) {
    if (!text(workspace.id, 200))
        throw AppError(ErrorCode::WORKSPACE_INVALID, {"id"});
    if (workspace.projects.size() > 100 || workspace.resources.size() > 1000 || workspace.bindings.size() > 10000)
        throw AppError(ErrorCode::WORKSPACE_LIMIT);
    std::set<std::string> projects, paths, resources;
    for (const auto &ref : workspace.projects) {
        if (!text(ref.documentId, 200) || !text(ref.path, 4096, !requirePaths) ||
            ref.name.find('\0') != std::string::npos)
            throw AppError(ErrorCode::WORKSPACE_INVALID, {"project reference"});
        if (!projects.insert(ref.documentId).second)
            throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {ref.documentId});
        if (!ref.path.empty() && !paths.insert(ref.path).second)
            throw AppError(ErrorCode::WORKSPACE_DUPLICATE_FILE, {ref.path});
    }
    for (const auto &shared : workspace.resources) {
        if (!text(shared.id, 200) || !resources.insert(shared.id).second || !shared.calendar.id.empty() ||
            !shared.calendar.name.empty() || shared.name.find('\0') != std::string::npos)
            throw AppError(ErrorCode::WORKSPACE_INVALID, {"shared resource identity"});
        if (shared.exceptions.size() > 10000 || shared.calendar.periods.size() > 100 ||
            shared.calendar.holidays.size() > 10000)
            throw AppError(ErrorCode::WORKSPACE_LIMIT);
        PlanInput plan;
        plan.name = "validation";
        plan.startDate = "2026-01-01";
        plan.calculationProfile = effortProfile;
        auto calendar = shared.calendar;
        calendar.id = "shared-calendar";
        calendar.name = shared.name;
        plan.calendars.push_back(calendar);
        Resource resource;
        resource.id = shared.id;
        resource.name = shared.name;
        resource.type = shared.type;
        resource.calendarId = calendar.id;
        plan.resources.push_back(resource);
        try {
            // A shared pool combines projects and has a higher ceiling than a local Resource.
            const auto validUnits = [](const std::optional<int> &value) {
                return !value || (*value >= 0 && *value <= maxSharedResourceUnits);
            };
            if (!validUnits(shared.maxUnits))
                throw AppError(ErrorCode::RESOURCE_INPUT_INVALID);
            std::set<std::string> dates;
            for (const auto &day : shared.exceptions)
                if (!validDate(day.date) || !dates.insert(day.date).second || !validUnits(day.units))
                    throw AppError(ErrorCode::RESOURCE_EXCEPTION_INVALID);
            validatePlan(plan);
        } catch (const AppError &error) {
            auto diagnostic = error.diagnostic();
            diagnostic.resourceIds = {shared.id};
            diagnostic.calendarIds = {shared.id};
            throw AppError(std::move(diagnostic));
        }
    }
    std::set<std::pair<std::string, std::string>> locals;
    for (const auto &binding : workspace.bindings) {
        if (!projects.contains(binding.projectId) || !resources.contains(binding.sharedId) ||
            !text(binding.resourceId, 200) || !locals.emplace(binding.projectId, binding.resourceId).second)
            throw AppError(ErrorCode::WORKSPACE_BINDING_INVALID, {binding.projectId + "/" + binding.resourceId});
    }
}
} // namespace jobs
