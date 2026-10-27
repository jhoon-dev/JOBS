#pragma once
#include "application/ProjectStore.h"

namespace jobs {
class JsonProjectStore final : public ProjectStore {
public:
    std::string newId() const override;
    std::string normalizePath(const std::string& path) const override;
    bool sameFile(const std::string& left, const std::string& right) const override;
    std::unique_ptr<WriteLease> lock(const std::string& path) override;
    StoredProject load(const std::string& path) const override;
    std::string save(const std::string& path, const Project& project,
                     const std::string& expectedFingerprint) override;
};
}
