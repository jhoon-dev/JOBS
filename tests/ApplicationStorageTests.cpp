#include "persistence/ApplicationStorage.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <iostream>
#include <stdexcept>

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ToolMoa");
    QCoreApplication::setApplicationName("JOBS-Storage-Contract");
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
        std::cout << "PASS: unpackaged Qt settings and recovery paths unchanged; no state written\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
