#pragma once
#include "application/Recovery.h"
#include <QString>
#include <memory>
#include <vector>
#include <functional>
namespace jobs {
struct RecoveryCandidate {
    QString sessionId, fileHash, error, sourceState, differences;
    std::uint64_t sequence = 0, revision = 0;
    std::string projectName, documentId, capturedAt, sourcePath;
    std::optional<std::uint64_t> lastSavedRevision;
    std::size_t tasks = 0, baselines = 0, progress = 0, costs = 0;
    bool valid = false;
    std::optional<Diagnostic> failure;
};
struct RecoveryScan {
    std::vector<RecoveryCandidate> candidates;
    int activeSessions = 0;
};
class RecoverySession {
public:
    ~RecoverySession();
    QString id() const;
    std::uint64_t checkpoint(const RecoverySnapshot &);
    void retire() noexcept;
    bool retired() const noexcept;
    void cleanup();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    RecoverySession(QString root, QString id, bool create, std::function<void()> beforePublish = {});
    friend class JsonRecoveryStore;
};
struct RecoveryClaim {
    std::shared_ptr<RecoverySession> session;
    RecoverySnapshot snapshot;
};
class JsonRecoveryStore {
public:
    // Optional durability-stage observer enables deterministic interruption tests.
    explicit JsonRecoveryStore(QString root, std::function<void()> beforePublish = {});
    static QString defaultDirectory();
    const QString &directory() const {
        return root_;
    }
    std::shared_ptr<RecoverySession> newSession() const;
    RecoveryScan scan() const;
    std::unique_ptr<RecoveryClaim> claim(const RecoveryCandidate &) const;
    void discardSession(const QString &sessionId) const;

private:
    QString root_;
    std::function<void()> beforePublish_;
};
} // namespace jobs
