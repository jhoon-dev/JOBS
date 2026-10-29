#pragma once
#include <QSettings>
#include <QString>

namespace jobs {
struct ApplicationStorage {
    QString packageFamilyName;
    QString stateDirectory;
    QString recoveryDirectory;
    QString settingsFile;
    bool packaged() const {
        return !packageFamilyName.isEmpty();
    }
};
// No directory creation, settings migration or fallback to another package's state.
ApplicationStorage applicationStorage();
QSettings applicationSettings();
} // namespace jobs
