#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName("JOBS");
    QCoreApplication::setApplicationVersion("0.1.0");
    QCoreApplication::setOrganizationName("ToolMoa");
    jobs::JsonProjectStore store;
    jobs::Document document(store);
    jobs::MainWindow window(document);
    window.show();
    return app.exec();
}
