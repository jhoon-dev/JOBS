#pragma once
#include "domain/Project.h"

namespace jobs {
inline constexpr int maxSharedResourceUnits = 10000000;
struct ProjectReference {
    std::string documentId, path, name;
    bool operator==(const ProjectReference &) const = default;
};
struct SharedResource {
    std::string id, name, type = "labor";
    std::optional<int> maxUnits = 100;
    std::vector<CapacityDay> exceptions = {};
    TaskCalendar calendar = {};
    bool operator==(const SharedResource &) const = default;
};
struct ResourceBinding {
    std::string projectId, resourceId, sharedId;
    bool operator==(const ResourceBinding &) const = default;
};
struct Workspace {
    std::string id;
    std::vector<ProjectReference> projects;
    std::vector<SharedResource> resources;
    std::vector<ResourceBinding> bindings;
    bool operator==(const Workspace &) const = default;
};
void validateWorkspace(const Workspace &, bool requirePaths = false);
} // namespace jobs
