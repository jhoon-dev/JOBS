#pragma once
#include "updates/UpdateCatalog.h"
#include "updates/StoreUpdateClient.h"
#include <QTimer>
#include <memory>
#include <optional>

namespace jobs {
class UpdateController final : public QObject {
    Q_OBJECT
public:
    explicit UpdateController(UpdateConfig, QObject *parent = nullptr, std::unique_ptr<StoreUpdateClient> = {});
    ~UpdateController() override;
    const UpdateConfig &config() const {
        return config_;
    }
    UpdateState state() const {
        return state_;
    }
    const QString &status() const {
        return status_;
    }
    bool busy() const {
        return state_ == UpdateState::Checking;
    }
    bool available() const {
        return state_ == UpdateState::Available;
    }
    bool automatic() const;
    void setAutomatic(bool);
    QString lastChecked() const;
    void scheduleStartup(std::function<quintptr()> window);
    void check(quintptr window = 0);
    void cancel();
signals:
    void changed();

private:
    UpdateConfig config_;
    std::unique_ptr<StoreUpdateClient> store_;
    QTimer deadline_;
    bool startupScheduled_ = false;
    std::optional<bool> automaticOverride_;
    quint64 generation_ = 0;
    UpdateState state_ = UpdateState::Idle;
    QString status_ = "아직 확인하지 않았습니다.";
    void setState(UpdateState, QString);
    void fail(QString);
    void recordCheck();
};
} // namespace jobs
