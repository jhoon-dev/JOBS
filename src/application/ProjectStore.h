#pragma once
#include "domain/Project.h"
#include <memory>

namespace jobs {
struct StoredProject {
    Project project;
    std::string fingerprint;
    bool requiresSaveAs = false;
};
class WriteLease {
public:
    virtual ~WriteLease() = default;
};
class ProjectStore {
public:
    virtual ~ProjectStore() = default;
    virtual std::string newId() const = 0;
    virtual std::string normalizePath(const std::string& path) const = 0;
    virtual std::unique_ptr<WriteLease> lock(const std::string& path) = 0;
    virtual StoredProject load(const std::string& path) const = 0;
    // Empty expectedFingerprint means create-only. Never silently overwrite.
    virtual std::string save(const std::string& path, const Project& project,
                             const std::string& expectedFingerprint) = 0;
};
}
