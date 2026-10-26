#pragma once
#include "application/Document.h"
#include <QMainWindow>
class QTableWidget;
class QLabel;
class QPlainTextEdit;
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
    QPlainTextEdit* diagnostics_;
    QAction *save_, *saveAs_, *add_;
    QAction *calculate_, *calendar_;
    bool calculating_ = false;
    bool confirmChanges();
    bool save(bool as);
    void create();
    void open();
    void refresh();
    void calculateSchedule();
    void editHolidays();
    void report(const std::exception& error);
};
}
