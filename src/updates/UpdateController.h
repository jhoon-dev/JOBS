#pragma once
#include "updates/UpdateCatalog.h"
#include "updates/StoreUpdateClient.h"
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QPointer>
#include <QTemporaryDir>
#include <QTimer>
#include <memory>

class QNetworkReply;
class QFile;
namespace jobs {
struct RunningUpdateInstance;
class UpdateController final : public QObject {
    Q_OBJECT
public:
    explicit UpdateController(UpdateConfig, QObject *parent = nullptr, QNetworkAccessManager * = nullptr,
                              std::unique_ptr<StoreUpdateClient> = {});
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
    const UpdateRelease &release() const {
        return release_;
    }
    bool busy() const {
        return state_ == UpdateState::Checking || state_ == UpdateState::Downloading;
    }
    bool available() const {
        return state_ == UpdateState::Available || state_ == UpdateState::Ready;
    }
    bool automatic() const;
    void setAutomatic(bool);
    QString lastChecked() const;
    void scheduleStartup(std::function<quintptr()> window);
    void check(quintptr window = 0);
    void cancel();
    bool canDownload() const;
    void download();
    bool verifyReady() const;
    const QString &installerPath() const {
        return installerPath_;
    }
    void preserveInstaller();
signals:
    void changed();
    void progress(qint64 received, qint64 total);

private:
    UpdateConfig config_;
    std::unique_ptr<RunningUpdateInstance> instance_;
    QNetworkAccessManager *network_;
    std::unique_ptr<StoreUpdateClient> store_;
    QPointer<QNetworkReply> reply_;
    QTimer deadline_;
    bool startupScheduled_ = false, downloading_ = false;
    std::optional<bool> automaticOverride_;
    quint64 generation_ = 0;
    UpdateState state_ = UpdateState::Idle;
    QString status_ = "아직 확인하지 않았습니다.", installerPath_;
    UpdateRelease release_;
    QByteArray response_;
    std::unique_ptr<QTemporaryDir> directory_;
    std::unique_ptr<QFile> file_;
    QCryptographicHash hash_{QCryptographicHash::Sha256};
    qint64 received_ = 0;
    void setState(UpdateState, QString);
    void fail(QString);
    void request(const QUrl &, int redirects = 0);
    void finished(QNetworkReply *, quint64, int);
    void discardDownload();
    void recordCheck();
};
} // namespace jobs
