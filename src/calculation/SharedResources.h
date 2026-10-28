#pragma once
#include "calculation/Resources.h"
#include "domain/Workspace.h"
#include <memory>

namespace jobs {
struct ProjectTaskRef {
    std::string projectId, taskId;
    auto operator<=>(const ProjectTaskRef &) const = default;
};
struct SharedResourceDay {
    std::string sharedId, date;
    std::optional<int> capacity;
    std::int64_t peakUnits = 0, demandUnitMinutes = 0, overloadMinutes = 0, excessUnitMinutes = 0,
                 unknownCapacityMinutes = 0;
    std::vector<ProjectTaskRef> tasks;
};
struct WorkspaceProjectResult {
    std::string projectId, name;
    std::uint64_t revision = 0;
    CalculationStatus status = CalculationStatus::Incomplete;
    std::optional<std::string> finish = {};
    std::size_t tasks = 0;
};
struct SharedResourceResult {
    CalculationStatus status = CalculationStatus::Incomplete;
    bool forecast = true;
    std::vector<WorkspaceProjectResult> projects;
    std::vector<SharedResourceDay> days;
    std::vector<Diagnostic> diagnostics;
    std::size_t includedProjects = 0, unmappedResources = 0, occupancyCount = 0, omittedDiagnostics = 0;
};
SharedResourceResult calculateSharedResources(const Workspace &, const std::vector<std::shared_ptr<const Project>> &,
                                              bool forecast, const std::shared_ptr<CalculationControl> &control = {});
} // namespace jobs
