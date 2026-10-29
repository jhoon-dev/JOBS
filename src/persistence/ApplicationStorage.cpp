#include "persistence/ApplicationStorage.h"
#include "domain/Diagnostics.h"
#include <QDir>
#include <QStandardPaths>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <appmodel.h>
#include <shlobj.h>
#include <vector>
#endif

namespace jobs {
ApplicationStorage applicationStorage() {
    ApplicationStorage storage;
#ifdef Q_OS_WIN
    UINT32 length = 0;
    const auto status = GetCurrentPackageFamilyName(&length, nullptr);
    if (status != APPMODEL_ERROR_NO_PACKAGE) {
        if (status != ERROR_INSUFFICIENT_BUFFER || length < 2 || length > 256)
            throw AppError(ErrorCode::FILE_OPERATION_FAILED, {"GetCurrentPackageFamilyName", std::to_string(status)});
        std::vector<wchar_t> family(length);
        const auto read = GetCurrentPackageFamilyName(&length, family.data());
        if (read != ERROR_SUCCESS)
            throw AppError(ErrorCode::FILE_OPERATION_FAILED, {"GetCurrentPackageFamilyName", std::to_string(read)});
        storage.packageFamilyName = QString::fromWCharArray(family.data());
        PWSTR local = nullptr;
        // The older SDK name is the same 0x10000 flag as KF_FLAG_NO_PACKAGE_REDIRECTION.
        const auto result =
            SHGetKnownFolderPath(FOLDERID_LocalAppData, KF_FLAG_NO_APPCONTAINER_REDIRECTION, nullptr, &local);
        if (FAILED(result))
            throw AppError(ErrorCode::FILE_OPERATION_FAILED, {"LocalAppData", std::to_string(result)});
        const auto root = QString::fromWCharArray(local);
        CoTaskMemFree(local);
        storage.stateDirectory =
            QDir(root).filePath("Packages/" + storage.packageFamilyName + "/LocalState/ToolMoa/JOBS");
        storage.recoveryDirectory = QDir(storage.stateDirectory).filePath("recovery");
        storage.settingsFile = QDir(storage.stateDirectory).filePath("settings.ini");
        return storage;
    }
#endif
    storage.stateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    storage.recoveryDirectory = QDir(storage.stateDirectory).filePath("recovery");
    return storage;
}
QSettings applicationSettings() {
    const auto storage = applicationStorage();
    if (storage.packaged())
        return QSettings(storage.settingsFile, QSettings::IniFormat);
    return QSettings();
}
} // namespace jobs
