#include "updates/UpdateController.h"
#include "updates/UpdateInstaller.h"
#include "persistence/ApplicationStorage.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace jobs {
struct RunningUpdateInstance {
    bool valid = true;
#ifdef Q_OS_WIN
    HANDLE handle = nullptr;
    explicit RunningUpdateInstance(bool installed) {
        if (installed) {
            handle = CreateMutexW(nullptr, FALSE, L"Local\\ToolMoa.JOBS.Standalone.Running");
            valid = handle != nullptr;
        }
    }
    ~RunningUpdateInstance() {
        if (handle)
            CloseHandle(handle);
    }
#else
    explicit RunningUpdateInstance(bool) {}
#endif
};
namespace {
QSettings settings(const UpdateConfig &config) {
    if (!config.settingsFile.isEmpty())
        return QSettings(config.settingsFile, QSettings::IniFormat);
    return applicationSettings();
}
} // namespace
UpdateController::UpdateController(UpdateConfig config, QObject *parent, QNetworkAccessManager *network,
                                   std::unique_ptr<StoreUpdateClient> store)
    : QObject(parent), config_(std::move(config)), network_(network ? network : new QNetworkAccessManager(this)),
      store_(store ? std::move(store) : createStoreUpdateClient()) {
    instance_ = std::make_unique<RunningUpdateInstance>(config_.installedWindows);
    if (!instance_->valid)
        config_.installedWindows = false;
    deadline_.setSingleShot(true);
    connect(&deadline_, &QTimer::timeout, this,
            [this] { fail("시간이 초과됐습니다. 네트워크를 확인하고 다시 시도하세요."); });
}
UpdateController::~UpdateController() {
    ++generation_;
    if (reply_) {
        reply_->disconnect(this);
        reply_->abort();
        reply_->deleteLater();
    }
    store_->cancel();
}
bool UpdateController::automatic() const {
    if (automaticOverride_)
        return *automaticOverride_;
    return settings(config_).value("updates/checkAtStartup", true).toBool();
}
void UpdateController::setAutomatic(bool enabled) {
    automaticOverride_ = enabled;
    auto preference = settings(config_);
    preference.setValue("updates/checkAtStartup", enabled);
    preference.sync();
    if (preference.status() != QSettings::NoError)
        status_ = "자동 확인 설정을 저장하지 못해 이번 실행에만 적용합니다. 설정 폴더의 권한을 확인하세요.";
    emit changed();
}
QString UpdateController::lastChecked() const {
    return settings(config_).value("updates/lastChecked").toString();
}
void UpdateController::recordCheck() {
    settings(config_).setValue("updates/lastChecked", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
}
void UpdateController::setState(UpdateState state, QString text) {
    state_ = state;
    status_ = std::move(text);
    emit changed();
}
void UpdateController::discardDownload() {
    file_.reset();
    directory_.reset();
    installerPath_.clear();
}
void UpdateController::fail(QString text) {
    ++generation_;
    deadline_.stop();
    if (reply_) {
        reply_->disconnect(this);
        reply_->abort();
        reply_->deleteLater();
        reply_.clear();
    }
    store_->cancel();
    discardDownload();
    downloading_ = false;
    setState(UpdateState::Failed, std::move(text));
}
void UpdateController::cancel() {
    if (!busy())
        return;
    fail("업데이트 요청을 취소했습니다.");
    setState(UpdateState::Cancelled, "업데이트 요청을 취소했습니다.");
}
void UpdateController::scheduleStartup(std::function<quintptr()> window) {
    if (startupScheduled_)
        return;
    startupScheduled_ = true;
    QTimer::singleShot(3000, this, [this, window] {
        if (automatic() && !busy() && state_ == UpdateState::Idle &&
            config_.channel != UpdateChannel::DevelopmentPackage)
            check(window());
    });
}
void UpdateController::check(quintptr window) {
    if (busy())
        return;
    discardDownload();
    release_ = {};
    response_.clear();
    downloading_ = false;
    const auto generation = ++generation_;
    if (config_.channel == UpdateChannel::DevelopmentPackage) {
        setState(UpdateState::Unsupported,
                 "개발용 MSIX입니다. Store 등록·게시 후 Store 업데이트를 사용할 수 있습니다.");
        return;
    }
    setState(UpdateState::Checking, "새 버전을 확인하고 있습니다…");
    deadline_.start(std::clamp(config_.checkTimeoutMs, 1, 15000));
    if (config_.channel == UpdateChannel::Store) {
        store_->check(window, [this, generation](StoreUpdateResult result) {
            if (generation != generation_)
                return;
            deadline_.stop();
            recordCheck();
            if (!result.success) {
                fail("Microsoft Store 확인 실패: " + result.error);
                return;
            }
            setState(result.count ? UpdateState::Available : UpdateState::Current,
                     result.count ? "Microsoft Store에 이 앱의 업데이트가 있습니다."
                                  : "Microsoft Store에서 제공하는 최신 버전입니다.");
        });
    } else
        request(updateCatalogUrl());
}
bool UpdateController::canDownload() const {
    return config_.channel == UpdateChannel::Release && config_.installedWindows && state_ == UpdateState::Available &&
           release_.sha256.size() == 32 && release_.bytes > 0 && downloadUrlAllowed(release_.download);
}
void UpdateController::download() {
    if (!canDownload())
        return;
    discardDownload();
    const auto root = QDir(config_.stateDirectory).filePath("updates");
    if (!QDir().mkpath(root)) {
        fail("업데이트 캐시 폴더를 만들 수 없습니다.");
        return;
    }
    directory_ = std::make_unique<QTemporaryDir>(QDir(root).filePath("download-XXXXXX"));
    if (!directory_->isValid()) {
        fail("다운로드 폴더를 만들 수 없습니다.");
        return;
    }
    installerPath_ = directory_->filePath("JOBS-update.exe");
    file_ = std::make_unique<QFile>(installerPath_);
    if (!file_->open(QIODevice::WriteOnly | QIODevice::NewOnly)) {
        fail("다운로드 파일을 만들 수 없습니다.");
        return;
    }
    downloading_ = true;
    received_ = 0;
    hash_.reset();
    ++generation_;
    setState(UpdateState::Downloading, "업데이트를 다운로드하고 있습니다…");
    deadline_.start(600000);
    request(release_.download);
}
void UpdateController::request(const QUrl &url, int redirects) {
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "JOBS-Updater");
    request.setRawHeader("Accept", downloading_ ? "application/octet-stream" : "application/vnd.github+json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    auto *reply = network_->get(request);
    reply_ = reply;
    reply->setReadBufferSize(128 * 1024);
    const auto generation = generation_;
    connect(reply, &QIODevice::readyRead, this, [this, reply, generation] {
        if (generation != generation_)
            return;
        if (reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() != 200) {
            reply->readAll();
            return;
        }
        const auto data = reply->readAll();
        if (!downloading_) {
            if (response_.size() + data.size() > MaximumCatalogBytes) {
                fail("릴리스 응답이 너무 큽니다.");
                return;
            }
            response_.append(data);
        } else {
            if (received_ + data.size() > release_.bytes || received_ + data.size() > MaximumInstallerBytes) {
                fail("다운로드 크기가 릴리스 정보와 다릅니다.");
                return;
            }
            if (file_->write(data) != data.size()) {
                fail("다운로드 파일을 기록하지 못했습니다.");
                return;
            }
            hash_.addData(data);
            received_ += data.size();
            emit progress(received_, release_.bytes);
        }
    });
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation, redirects] { finished(reply, generation, redirects); });
}
void UpdateController::finished(QNetworkReply *reply, quint64 generation, int redirects) {
    reply->deleteLater();
    if (generation != generation_)
        return;
    reply_.clear();
    const auto status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 300 && status < 400) {
        const auto target =
            reply->url().resolved(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
        if (!downloading_ || redirects >= 5 || !downloadUrlAllowed(target)) {
            fail("허용되지 않은 업데이트 이동 경로입니다.");
            return;
        }
        request(target, redirects + 1);
        return;
    }
    if (!downloading_)
        recordCheck();
    if (status == 404 && !downloading_) {
        deadline_.stop();
        setState(UpdateState::Unpublished, "공개된 정식 릴리스가 아직 없습니다.");
        return;
    }
    if (status == 403 || status == 429) {
        fail("업데이트 서버의 요청 한도에 도달했습니다. 나중에 다시 시도하세요.");
        return;
    }
    if (reply->error() != QNetworkReply::NoError || status != 200) {
        fail("업데이트 서버에 연결하지 못했습니다. 네트워크를 확인하고 다시 시도하세요.");
        return;
    }
    deadline_.stop();
    if (downloading_) {
        const bool valid = received_ == release_.bytes && hash_.result() == release_.sha256 && file_->flush();
        file_->close();
        downloading_ = false;
        if (!valid) {
            fail("다운로드 크기 또는 SHA-256 검증에 실패했습니다. 설치하지 않았습니다.");
            return;
        }
        if (!markDownloadedInstaller(installerPath_, release_.download)) {
            fail("설치 파일의 인터넷 출처 정보를 기록하지 못했습니다. 릴리스 페이지에서 수동으로 다운로드해 주세요.");
            return;
        }
        setState(UpdateState::Ready, "다운로드와 SHA-256 검증이 완료됐습니다. 설치 전 문서를 저장해 주세요.");
        return;
    }
    try {
        release_ = parseRelease(response_, config_.currentVersion);
        const bool newer = newerVersion(release_.version, config_.currentVersion);
        setState(newer ? UpdateState::Available : UpdateState::Current,
                 newer ? "새 버전 " + release_.version + "을 사용할 수 있습니다."
                       : "현재 버전보다 새로운 정식 릴리스가 없습니다.");
    } catch (const std::exception &) {
        fail("릴리스 정보를 검증하지 못했습니다. 나중에 다시 확인해 주세요.");
    }
}
bool UpdateController::verifyReady() const {
    if (state_ != UpdateState::Ready || !directory_ || !directory_->isValid())
        return false;
    QFile file(installerPath_);
    if (!file.open(QIODevice::ReadOnly) || file.size() != release_.bytes)
        return false;
    QCryptographicHash hash(QCryptographicHash::Sha256);
    return hash.addData(&file) && hash.result() == release_.sha256;
}
void UpdateController::preserveInstaller() {
    if (directory_)
        directory_->setAutoRemove(false);
}
} // namespace jobs
