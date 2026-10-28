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
        if (!text(ref.documentId, 200) || !text(ref.path, 4096, !requirePaths) || !text(ref.name, 2000, true))
            throw AppError(ErrorCode::WORKSPACE_INVALID, {"project reference"});
        if (!projects.insert(ref.documentId).second)
            throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {ref.documentId});
        if (!ref.path.empty() && !paths.insert(ref.path).second)
            throw AppError(ErrorCode::WORKSPACE_DUPLICATE_FILE, {ref.path});
    }
    for (const auto &shared : workspace.resources) {
        if (!text(shared.id, 200) || !resources.insert(shared.id).second || !shared.calendar.id.empty() ||
            !shared.calendar.name.empty())
            throw AppError(ErrorCode::WORKSPACE_INVALID, {"shared resource identity"});
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
        resource.maxUnits = shared.maxUnits;
        resource.exceptions = shared.exceptions;
        resource.calendarId = calendar.id;
        plan.resources.push_back(resource);
        try {
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
