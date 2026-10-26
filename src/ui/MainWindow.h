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
class JsonRecoveryStore;
class AutoSaveController;
class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(Document& document, JsonRecoveryStore* recoveryStore = nullptr);
    ~MainWindow() override;
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
    QAction *undo_ = nullptr, *redo_ = nullptr, *recover_ = nullptr, *autosaveEnabled_ = nullptr, *autosaveNow_ = nullptr;
    std::uint64_t historyListener_ = 0;
    std::unique_ptr<JsonRecoveryStore> recoveryStore_;
    AutoSaveController* autosave_ = nullptr;
    QLabel* autosaveStatus_ = nullptr;
    void setupRecovery(JsonRecoveryStore*);
    void showRecovery(bool startup = false);
    void updateHistoryActions();
    QAction *calculate_, *calendar_;
    QAction *calendars_, *taskCalendar_;
    void manageCalendars();
    void chooseTaskCalendar();
    QAction *addWbs_, *renameWbs_, *moveWbs_, *removeWbs_, *assignWbs_;
    QAction *baselineSave_, *statusEdit_, *progressEdit_, *comparison_;
    QComboBox* baselineChoice_;
    std::string selectedBaseline_;
    bool baselinePreparing_ = false;
    QAction *resources_, *assignResources_, *resourceComparison_;
    void manageResources();
    void assignResources();
    void showResources();
    QAction *rates_, *taskCost_, *costReport_, *evm_;
    void editRates();
    void editTaskCost();
    void reportCost();
    void showCosts();
    QAction* reports_;
    std::string selectedWbs_, selectedTask_, treeDocument_;
    std::vector<std::size_t> visibleRows_;
    bool calculating_ = false;
    bool confirmChanges();
    void commitPendingCell();
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
