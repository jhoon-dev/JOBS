#pragma once
#include "domain/Project.h"
namespace jobs {
// Recovery is a complete input snapshot, separate from an explicitly saved file.
struct RecoverySnapshot {
    Project project;
    std::string sourcePath, sourceFingerprint, capturedAt;
    bool sourceReadOnly = false, sourceRequiresSaveAs = false;
    std::optional<std::uint64_t> lastSavedRevision;
};
struct RecoveryOrigin {
    std::string documentId, path, capturedAt;
    std::uint64_t revision = 0;
    std::optional<std::uint64_t> lastSavedRevision;
};
}
