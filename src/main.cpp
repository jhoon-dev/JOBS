#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/DesktopWorkspace.h"
#include "persistence/JsonRecoveryStore.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("JOBS");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("ToolMoa");
    auto store = std::make_shared<jobs::JsonProjectStore>();
    auto recovery = std::make_shared<jobs::JsonRecoveryStore>(jobs::JsonRecoveryStore::defaultDirectory());
    jobs::DesktopWorkspace workspace(store, recovery);
    workspace.newWindow();
    return app.exec();
}
