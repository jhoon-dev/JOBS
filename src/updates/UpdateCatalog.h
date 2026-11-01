#pragma once
#include <QString>
#include <QUrl>

namespace jobs {
enum class UpdateChannel { Unpackaged, Store, DevelopmentPackage, MacAppStore };
enum class UpdateState { Idle, Checking, Current, Available, Unsupported, Cancelled, Failed };
struct UpdateConfig {
    QString currentVersion;
    UpdateChannel channel = UpdateChannel::Unpackaged;
    QString stateDirectory, settingsFile;
    int checkTimeoutMs = 15000;
};
QString appVersion();
UpdateConfig updateConfig();
QUrl storeUpdatesUrl();
QUrl macAppStoreUrl();
} // namespace jobs
