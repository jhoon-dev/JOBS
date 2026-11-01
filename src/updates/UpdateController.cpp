#include "updates/UpdateController.h"
#include "persistence/ApplicationStorage.h"
#include <QDateTime>
#include <QSettings>
#include <algorithm>

namespace jobs {
namespace {
QSettings settings(const UpdateConfig &config) {
    if (!config.settingsFile.isEmpty())
        return QSettings(config.settingsFile, QSettings::IniFormat);
    return applicationSettings();
}
} // namespace
UpdateController::UpdateController(UpdateConfig config, QObject *parent, std::unique_ptr<StoreUpdateClient> store)
    : QObject(parent), config_(std::move(config)), store_(store ? std::move(store) : createStoreUpdateClient()) {
    deadline_.setSingleShot(true);
    connect(&deadline_, &QTimer::timeout, this,
            [this] { fail("시간이 초과됐습니다. Microsoft Store 연결을 확인하고 다시 시도하세요."); });
}
UpdateController::~UpdateController() {
    ++generation_;
    store_->cancel();
}
bool UpdateController::automatic() const {
    if (config_.channel != UpdateChannel::Store)
        return false;
    if (automaticOverride_)
        return *automaticOverride_;
    return settings(config_).value("updates/checkAtStartup", true).toBool();
}
void UpdateController::setAutomatic(bool enabled) {
    if (config_.channel != UpdateChannel::Store)
        return;
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
void UpdateController::fail(QString text) {
    ++generation_;
    deadline_.stop();
    store_->cancel();
    setState(UpdateState::Failed, std::move(text));
}
void UpdateController::cancel() {
    if (!busy())
        return;
    ++generation_;
    deadline_.stop();
    store_->cancel();
    setState(UpdateState::Cancelled, "업데이트 요청을 취소했습니다.");
}
void UpdateController::scheduleStartup(std::function<quintptr()> window) {
    if (startupScheduled_ || config_.channel != UpdateChannel::Store)
        return;
    startupScheduled_ = true;
    QTimer::singleShot(3000, this, [this, window] {
        if (automatic() && !busy() && state_ == UpdateState::Idle)
            check(window());
    });
}
void UpdateController::check(quintptr window) {
    if (busy())
        return;
    if (config_.channel == UpdateChannel::MacAppStore) {
        setState(UpdateState::Unsupported,
                 "Mac App Store에서 업데이트를 관리합니다. App Store를 열어 업데이트를 확인해 주세요.");
        return;
    }
    if (config_.channel != UpdateChannel::Store) {
        setState(UpdateState::Unsupported, "개발 실행본입니다. 업데이트는 스토어에서 배포한 앱을 통해 이용해 주세요.");
        return;
    }
    const auto generation = ++generation_;
    deadline_.start(std::clamp(config_.checkTimeoutMs, 1, 15000));
    setState(UpdateState::Checking, "Microsoft Store에서 새 버전을 확인하고 있습니다…");
    if (generation != generation_)
        return;
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
}
} // namespace jobs
