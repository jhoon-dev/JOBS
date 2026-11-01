#pragma once
#include "application/ProjectStore.h"
#include "domain/Workspace.h"
#include <functional>

namespace jobs {
struct StoredWorkspace {
    Workspace workspace;
    std::string path, fingerprint;
};
class JsonWorkspaceStore {
public:
    explicit JsonWorkspaceStore(ProjectStore &files) : files_(files) {}
    StoredWorkspace load(const std::string &path,
                         const std::function<void(const std::string &)> &authorizeReference = {}) const;
    std::string save(const std::string &path, const Workspace &, const std::string &expectedFingerprint) const;

private:
    ProjectStore &files_;
};
} // namespace jobs
