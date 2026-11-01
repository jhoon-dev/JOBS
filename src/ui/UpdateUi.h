#pragma once
#include <QUrl>
#include <functional>
class QMainWindow;
namespace jobs {
class UpdateController;
struct UpdateUiHooks {
    std::function<bool(const QUrl &)> openUrl;
};
void attachUpdateUi(QMainWindow &, UpdateController &, UpdateUiHooks = {});
} // namespace jobs
