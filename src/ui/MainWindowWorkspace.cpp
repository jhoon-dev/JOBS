#include "ui/MainWindow.h"
#include "ui/AsyncEditController.h"
#include <QComboBox>
#include <QLineEdit>
#include <algorithm>

namespace jobs {
bool MainWindow::selectTaskById(const std::string &taskId) {
    if (!document_.project() || std::none_of(document_.project()->tasks.begin(), document_.project()->tasks.end(),
                                             [&](const auto &task) { return task.id == taskId; }))
        return false;
    if (editController_ && editController_->busy()) {
        runAfterEdit([this, taskId] { selectTaskById(taskId); });
        return true;
    }
    commitPendingCell();
    if (!document_.project() || std::none_of(document_.project()->tasks.begin(), document_.project()->tasks.end(),
                                             [&](const auto &task) { return task.id == taskId; }))
        return false;
    selectedWbs_.clear();
    selectedTask_ = taskId;
    selectedTaskIds_ = {taskId};
    if (taskSearch_)
        taskSearch_->clear();
    if (taskFilter_)
        taskFilter_->setCurrentIndex(0);
    if (taskSort_)
        taskSort_->setCurrentIndex(0);
    taskViewReset_ = true;
    rendered_ = false;
    refresh();
    return true;
}
} // namespace jobs
