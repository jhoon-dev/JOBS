#pragma once
#include "application/Document.h"
#include <QMainWindow>
class QTableWidget;
class QLabel;
class QAction;
namespace jobs {
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(Document& document);
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    Document& document_;
    QTableWidget* table_;
    QLabel* summary_;
    QAction *save_, *saveAs_, *add_;
    bool confirmChanges();
    bool save(bool as);
    void create();
    void open();
    void refresh();
    void report(const std::exception& error);
};
}
