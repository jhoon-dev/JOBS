#pragma once
#include "application/Document.h"
#include <QMainWindow>
class QTableWidget;
class QLabel;
class QPlainTextEdit;
class QAction;
class QTreeWidget;
class QComboBox;
namespace jobs {
class GanttView;
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(Document& document);
protected:
    void closeEvent(QCloseEvent* event) override;
private:
    Document& document_;
    QTableWidget* table_;
    QTreeWidget* wbsTree_;
    GanttView* gantt_;
    QLabel* wbsSummary_;
    QLabel* summary_;
    QPlainTextEdit* diagnostics_;
    QAction *save_, *saveAs_, *add_;
    QAction *calculate_, *calendar_;
    QAction *addWbs_, *renameWbs_, *moveWbs_, *removeWbs_, *assignWbs_;
    QAction *baselineSave_, *statusEdit_, *progressEdit_, *comparison_;
    QComboBox* baselineChoice_;
    std::string selectedBaseline_;
    bool baselinePreparing_ = false;
    std::string selectedWbs_, selectedTask_, treeDocument_;
    std::vector<std::size_t> visibleRows_;
    bool calculating_ = false;
    bool confirmChanges();
    bool save(bool as);
    void create();
    void open();
    void refresh(bool rebuildTree = true);
    void calculateSchedule();
    void editHolidays();
    void rebuildWbsTree();
    void createWbs();
    void renameWbs();
    void chooseWbs(bool moveNode);
    void saveBaseline();
    void editStatusAt();
    void reportProgress();
    void showComparison();
    void refreshTracking();
    void report(const std::exception& error);
};
}
