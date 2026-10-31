#include "updates/UpdateCatalog.h"
#include "persistence/ApplicationStorage.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSettings>
#include <QVersionNumber>
#include <stdexcept>
#include <algorithm>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <vector>
#endif

namespace jobs {
namespace {
bool validVersion(const QString &text) {
    static const QRegularExpression pattern("^(0|[1-9][0-9]{0,4})\\.(0|[1-9][0-9]{0,4})\\.(0|[1-9][0-9]{0,4})$");
    if (!pattern.match(text).hasMatch())
        return false;
    const auto parts = QVersionNumber::fromString(text).segments();
    return std::all_of(parts.begin(), parts.end(), [](int n) { return n <= 65535; });
}
bool https(const QUrl &url) {
    return url.isValid() && url.scheme() == "https" && url.userInfo().isEmpty() &&
           (url.port() == -1 || url.port() == 443) && url.fragment().isEmpty();
}
void require(bool condition, const char *reason) {
    if (!condition)
        throw std::runtime_error(reason);
}
} // namespace
QString appVersion() {
    return QStringLiteral(JOBS_APP_VERSION);
}
UpdateConfig updateConfig() {
    const auto storage = applicationStorage();
    UpdateConfig config;
    config.currentVersion = appVersion();
    config.stateDirectory = storage.stateDirectory;
    config.settingsFile = storage.settingsFile;
    if (storage.packaged()) {
        config.channel = storage.packageFamilyName.startsWith("ToolMoa.JOBS.MsixDevelopment_")
                             ? UpdateChannel::DevelopmentPackage
                             : UpdateChannel::Store;
    }
#ifdef Q_OS_WIN
    else {
        QSettings marker(QDir(QCoreApplication::applicationDirPath()).filePath("install-info.ini"),
                         QSettings::IniFormat);
        // QSettings registry reads may create their root key. Detection must remain read-only.
        constexpr wchar_t registration[] = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ToolMoa.JOBS";
        constexpr DWORD flags = RRF_RT_REG_SZ | RRF_SUBKEY_WOW6464KEY;
        DWORD bytes = 0;
        QString location;
        if (RegGetValueW(HKEY_CURRENT_USER, registration, L"InstallLocation", flags, nullptr, nullptr, &bytes) ==
                ERROR_SUCCESS &&
            bytes >= sizeof(wchar_t) && bytes <= 65536) {
            std::vector<wchar_t> value(bytes / sizeof(wchar_t) + 1, 0);
            DWORD capacity = DWORD(value.size() * sizeof(wchar_t));
            if (RegGetValueW(HKEY_CURRENT_USER, registration, L"InstallLocation", flags, nullptr, value.data(),
                             &capacity) == ERROR_SUCCESS)
                location = QString::fromWCharArray(value.data());
        }
        const auto registered = QFileInfo(location).canonicalFilePath();
        const auto executableDirectory = QFileInfo(QCoreApplication::applicationDirPath()).canonicalFilePath();
        config.installedWindows = marker.value("JOBS/ProductId").toString() == "ToolMoa.JOBS" &&
                                  !registered.isEmpty() &&
                                  registered.compare(executableDirectory, Qt::CaseInsensitive) == 0;
    }
#endif
#ifdef JOBS_MAC_APP_STORE
    config.channel = UpdateChannel::MacAppStore;
#endif
    return config;
}
bool newerVersion(const QString &candidate, const QString &current) {
    require(validVersion(candidate) && validVersion(current), "Invalid release version");
    return QVersionNumber::compare(QVersionNumber::fromString(candidate), QVersionNumber::fromString(current)) > 0;
}
QUrl updateCatalogUrl() {
    return QUrl("https://api.github.com/repos/jhoon-dev/JOBS/releases/latest");
}
QUrl storeUpdatesUrl() {
    return QUrl("ms-windows-store://downloadsandupdates");
}
QUrl macAppStoreUrl() {
    return QUrl("macappstore://apps.apple.com/app/id6829146371");
}
bool releasePageAllowed(const QUrl &url) {
    return https(url) && url.host() == "github.com" && url.query().isEmpty() &&
           url.path().startsWith("/jhoon-dev/JOBS/releases/tag/") && !url.path().contains("..") &&
           !url.path().contains('%');
}
bool downloadUrlAllowed(const QUrl &url) {
    if (!https(url))
        return false;
    if (url.host() == "github.com")
        return url.path().startsWith("/jhoon-dev/JOBS/releases/download/") && !url.path().contains("..");
    return url.host() == "release-assets.githubusercontent.com" || url.host() == "objects.githubusercontent.com";
}
UpdateRelease parseRelease(const QByteArray &json, const QString &current) {
    require(json.size() <= MaximumCatalogBytes, "Release response too large");
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(json, &error);
    require(error.error == QJsonParseError::NoError && document.isObject(), "Invalid release response");
    const auto object = document.object();
    require(object.value("draft").isBool() && !object.value("draft").toBool() && object.value("prerelease").isBool() &&
                !object.value("prerelease").toBool(),
            "Not a stable published release");
    UpdateRelease release;
    auto tag = object.value("tag_name").toString();
    release.version = tag.startsWith('v') ? tag.mid(1) : tag;
    newerVersion(release.version, current); // Validate both sides even for an older release.
    release.page = QUrl(object.value("html_url").toString());
    require(releasePageAllowed(release.page) && release.page.path() == "/jhoon-dev/JOBS/releases/tag/" + tag,
            "Release URL does not match repository and tag");
    release.notes = object.value("body").toString().left(32000);
    const QRegularExpression name("^JOBS-" + QRegularExpression::escape(release.version) +
                                  "-Setup-win64-[0-9a-f]{8,40}\\.exe$");
    const auto assets = object.value("assets");
    require(assets.isArray() && assets.toArray().size() <= 100, "Invalid release assets");
    for (const auto &value : assets.toArray()) {
        const auto asset = value.toObject();
        if (!name.match(asset.value("name").toString()).hasMatch())
            continue;
        require(release.assetName.isEmpty(), "Ambiguous installer assets");
        release.assetName = asset.value("name").toString();
        const auto size = asset.value("size").toDouble(-1);
        require(size >= 1 && size <= MaximumInstallerBytes && size == qint64(size), "Invalid installer size");
        release.bytes = qint64(size);
        release.download = QUrl(asset.value("browser_download_url").toString());
        require(downloadUrlAllowed(release.download) && release.download.host() == "github.com" &&
                    release.download.path() == "/jhoon-dev/JOBS/releases/download/" + tag + "/" + release.assetName &&
                    release.download.query().isEmpty(),
                "Installer URL does not match release");
        const auto digest = asset.value("digest").toString();
        if (QRegularExpression("^sha256:[0-9a-fA-F]{64}$").match(digest).hasMatch())
            release.sha256 = QByteArray::fromHex(digest.mid(7).toLatin1());
    }
    return release;
}
} // namespace jobs
