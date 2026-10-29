#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/DesktopWorkspace.h"
#include "persistence/JsonRecoveryStore.h"
#include "updates/UpdateController.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("JOBS");
    QCoreApplication::setApplicationVersion(jobs::appVersion());
    QCoreApplication::setOrganizationName("ToolMoa");
    auto store = std::make_shared<jobs::JsonProjectStore>();
    auto recovery = std::make_shared<jobs::JsonRecoveryStore>(jobs::JsonRecoveryStore::defaultDirectory());
    jobs::UpdateController updates(jobs::updateConfig());
    jobs::DesktopWorkspace workspace(store, recovery);
    workspace.setUpdates(&updates);
    workspace.newWindow();
    return app.exec();
}
