#pragma once
#include <QString>
#include <QUrl>
#include <functional>
class QMainWindow;
namespace jobs {
class UpdateController;
struct UpdateUiHooks {
    std::function<bool()> prepareInstall;
    std::function<void()> finishInstall;
    std::function<bool(const QString &)> launchInstaller;
    std::function<bool(const QUrl &)> openUrl;
};
void attachUpdateUi(QMainWindow &, UpdateController &, UpdateUiHooks = {});
} // namespace jobs
