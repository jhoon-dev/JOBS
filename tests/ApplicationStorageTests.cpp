#include "persistence/ApplicationStorage.h"
#include "persistence/JsonRecoveryStore.h"
#include "application/ProductIdentity.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ToolMoa");
    QCoreApplication::setApplicationName("PPM-Storage-Contract");
    try {
        const auto storage = jobs::applicationStorage();
        if (storage.packaged())
            throw std::logic_error("unpackaged test unexpectedly has package identity");
        if (storage.stateDirectory != QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) ||
            storage.recoveryDirectory != QDir(storage.stateDirectory).filePath("recovery") ||
            !storage.settingsFile.isEmpty())
            throw std::logic_error("standalone data contract changed");
        const auto settings = jobs::applicationSettings();
        const QSettings legacy;
        if (settings.format() != legacy.format() || settings.fileName() != legacy.fileName() ||
            jobs::JsonRecoveryStore::defaultDirectory() != storage.recoveryDirectory)
            throw std::logic_error("standalone settings/recovery migrated unexpectedly");
        QCoreApplication::setApplicationName("JOBS");
        const auto legacyData = jobs::applicationStorage();
        const auto legacySettings = jobs::applicationSettings();
        QCoreApplication::setApplicationName(jobs::productShortName);
        const auto branded = jobs::applicationStorage();
        const auto brandedSettings = jobs::applicationSettings();
        if (branded.stateDirectory != legacyData.stateDirectory ||
            branded.recoveryDirectory != legacyData.recoveryDirectory ||
            brandedSettings.fileName() != legacySettings.fileName())
            throw std::logic_error("PPM branding orphaned existing settings or recovery");
        std::cout << "PASS: PPM uses existing settings/recovery paths; no state written\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
