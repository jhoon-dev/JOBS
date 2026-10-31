#pragma once
#include <QJsonObject>
#include <QString>
#include <QUrl>
#include <optional>

namespace jobs {
enum class UpdateChannel { Release, Store, DevelopmentPackage, MacAppStore };
enum class UpdateState {
    Idle,
    Checking,
    Current,
    Available,
    Unpublished,
    Unsupported,
    Downloading,
    Ready,
    Cancelled,
    Failed
};
struct UpdateRelease {
    QString version, notes, assetName;
    QUrl page, download;
    QByteArray sha256;
    qint64 bytes = 0;
};
struct UpdateConfig {
    QString currentVersion;
    UpdateChannel channel = UpdateChannel::Release;
    bool installedWindows = false;
    QString stateDirectory, settingsFile;
    int checkTimeoutMs = 15000;
};
inline constexpr qint64 MaximumInstallerBytes = 512LL * 1024 * 1024;
inline constexpr qint64 MaximumCatalogBytes = 2LL * 1024 * 1024;
QString appVersion();
UpdateConfig updateConfig();
bool newerVersion(const QString &candidate, const QString &current);
UpdateRelease parseRelease(const QByteArray &json, const QString &current);
bool releasePageAllowed(const QUrl &);
bool downloadUrlAllowed(const QUrl &);
QUrl updateCatalogUrl();
QUrl storeUpdatesUrl();
QUrl macAppStoreUrl();
} // namespace jobs
