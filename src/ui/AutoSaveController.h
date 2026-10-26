#pragma once
#include "application/Document.h"
#include "persistence/JsonRecoveryStore.h"
#include <QObject>
#include <QTimer>
namespace jobs {
class AutoSaveController final : public QObject {
public:
    AutoSaveController(Document&, JsonRecoveryStore, QObject* parent = nullptr, int intervalMs = 30000, int idleMs = 2000);
    ~AutoSaveController() override;
    QString status() const { return status_ + (cleanupError_.isEmpty() ? QString{} : " · 복구본 정리 실패: " + cleanupError_); }
    bool busy() const { return busy_; }
    bool enabled() const { return enabled_; }
    int intervalMs() const { return intervalMs_; }
    void setEnabled(bool);
    void setIntervalMs(int);
    void checkpointNow();
    void finishDocument();
    void adoptRecovered(std::shared_ptr<RecoveryClaim>);
    std::function<void()> statusChanged;
private:
    Document& document_;
    JsonRecoveryStore store_;
    std::shared_ptr<RecoverySession> session_, recoveredSession_;
    QTimer idle_, deadline_;
    std::uint64_t listener_ = 0, serial_ = 0, storedSerial_ = 0;
    bool enabled_ = true, busy_ = false, finished_ = false;
    int intervalMs_, idleMs_;
    QString status_, cleanupError_;
    void changed(Document::Event);
    void schedule();
    void retire(std::shared_ptr<RecoverySession>);
    void setStatus(QString);
};
}
