#include "updates/UpdateCatalog.h"
#include "persistence/ApplicationStorage.h"

namespace jobs {
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
#ifdef JOBS_MAC_APP_STORE
    config.channel = UpdateChannel::MacAppStore;
#endif
    return config;
}
QUrl storeUpdatesUrl() {
    return QUrl("ms-windows-store://downloadsandupdates");
}
QUrl macAppStoreUrl() {
    return QUrl("macappstore://apps.apple.com/app/id6829387727");
}
} // namespace jobs
