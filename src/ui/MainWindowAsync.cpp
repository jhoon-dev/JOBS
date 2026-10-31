#include "formatting/QtDiagnosticText.h"
#include "ui/MainWindow.h"
#include "ui/EditorUi.h"
#include "ui/AsyncEditController.h"
#include "ui/TaskInput.h"
#include "ui/TaskTableModel.h"
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <algorithm>

namespace jobs {
void MainWindow::setupAsyncEditing(QToolBar *tools) {
    editController_ = new AsyncEditController(document_, this);
    taskModel_->unchangedEditRequested = [this](std::string id, int column) {
        if (column == 1 && editController_->cancelDurationEdit(id))
            afterEdit_ = {};
    };
    editController_->beforeApply = [this] { commitPendingCell(); };
    editController_->captureView = [this] { return captureTaskView(); };
    cancelEdit_ = new QAction("편집 계산 취소", this);
    cancelEdit_->setObjectName("cancelEdit");
    cancelEdit_->setEnabled(false);
    tools->addAction(cancelEdit_);
    editProgress_ = new QLabel;
    editProgress_->setObjectName("editProgress");
    statusBar()->addPermanentWidget(editProgress_);
    editProgressTimer_ = new QTimer(this);
    editProgressTimer_->setInterval(100);
    connect(cancelEdit_, &QAction::triggered, this, [this] {
        afterEdit_ = {};
        editController_->cancel();
    });
    connect(editProgressTimer_, &QTimer::timeout, this, [this] {
        const auto control = editController_->control();
        if (!control)
            return;
        editProgress_->setText(editController_->label() + (control->cancelRequested.load()
                                                               ? " · 취소 중…"
                                                               : " · " + QString::number(control->completed.load()) +
                                                                     " / " + QString::number(control->total.load())));
    });
    editController_->stateChanged = [this] {
        const bool busy = editController_->busy();
        const auto control = editController_->control();
        cancelEdit_->setEnabled(busy && control && !control->cancelRequested.load());
        if (busy) {
            editProgress_->setText(editController_->label() + " · 계산 중…");
            editProgressTimer_->start();
        } else {
            editProgressTimer_->stop();
            editProgress_->clear();
        }
        updateHistoryActions();
    };
}
void MainWindow::runAfterEdit(std::function<void()> command) {
    commitPendingCell();
    if (editController_ && editController_->busy()) {
        if (!afterEdit_)
            afterEdit_ = std::move(command);
        statusBar()->showMessage("편집 계산이 성공하면 대기 명령을 실행합니다. 취소하면 명령도 취소됩니다.");
        return;
    }
    command();
}
std::shared_ptr<CalculationControl> MainWindow::submitEdit(EditCommand command, const QString &label,
                                                           std::function<void(EditState, const QString &)> completion) {
    return editController_->submit(
        std::move(command), label, [this, completion = std::move(completion)](EditState state, const QString &message) {
            const bool success = state == EditState::Applied || state == EditState::NoChange;
            const auto session = document_.sessionGeneration();
            const auto revision = document_.project() ? document_.project()->revision : 0;
            auto after = std::move(afterEdit_);
            afterEdit_ = {};
            if (state == EditState::Applied)
                if (auto view = document_.bulkView()) {
                    if (!view->createdIds.empty()) {
                        selectedTaskIds_ = view->createdIds;
                        selectedTask_ = selectedTaskIds_.front();
                    } else if (view->clearSelection) {
                        selectedTaskIds_.clear();
                        selectedTask_.clear();
                    }
                }
            refresh();
            if (!message.isEmpty())
                statusBar()->showMessage(message, 10000);
            QPointer<MainWindow> guard(this);
            if (completion)
                completion(state, message);
            if (!guard)
                return;
            if (success && after && document_.project() && document_.sessionGeneration() == session &&
                document_.project()->revision == revision)
                after();
        });
}
void MainWindow::submitDialogEdit(QDialog &dialog, QLabel *error, QDialogButtonBox *buttons, EditCommand command,
                                  const QString &label, std::function<void(bool)> setPending) {
    QPointer<QDialog> guard(&dialog);
    error->setTextFormat(Qt::PlainText);
    auto *cancel = buttons->findChild<QPushButton *>("cancelDialogEdit");
    if (!cancel) {
        cancel = buttons->addButton("계산 취소", QDialogButtonBox::ActionRole);
        cancel->setObjectName("cancelDialogEdit");
        alignEditorControl(*cancel, std::max(32, dialog.fontMetrics().height() + 16));
    }
    error->setText(label + " · 계산 중…");
    setPending(true);
    cancel->setEnabled(true);
    try {
        auto control = submitEdit(
            std::move(command), label, [guard, error, cancel, setPending](EditState state, const QString &message) {
                if (!guard)
                    return;
                setPending(false);
                cancel->setEnabled(false);
                if (state == EditState::Applied || state == EditState::NoChange)
                    guard->accept();
                else
                    error->setText(message.isEmpty() ? "편집 계산에 실패했습니다. 입력은 변경하지 않았습니다."
                                                     : message);
            });
        // Each attempt owns its cancellation token. An old dialog cannot cancel
        // a newer request which happens to be running when it closes.
        connect(cancel, &QPushButton::clicked, &dialog, [control] { control->cancel(); });
        connect(&dialog, &QDialog::finished, &dialog, [control] { control->cancel(); });
    } catch (...) {
        setPending(false);
        cancel->setEnabled(false);
        throw;
    }
}
bool MainWindow::hasInverseDuration(const std::string &id) const {
    if (!document_.project() || !isEffort(*document_.project()))
        return false;
    for (const auto &task : document_.project()->tasks)
        if (task.id == id)
            return task.taskType == TaskType::FixedWork;
    return false;
}
bool MainWindow::editTaskCell(std::string id, int column, const QString &text) {
    try {
        if (!document_.project())
            return false;
        const auto &tasks = document_.project()->tasks;
        const auto found = std::find_if(tasks.begin(), tasks.end(), [&](const auto &task) { return task.id == id; });
        if (found == tasks.end())
            return false;
        const auto row = std::size_t(found - tasks.begin());
        if (column == 0)
            document_.renameTask(row, text.toUtf8().toStdString());
        else if (column == 1) {
            const auto duration = taskDurationInput(text);
            if (hasInverseDuration(id)) {
                submitEdit(DurationEdit{std::move(id), duration}, "고정 공수 기간 역산");
                return true;
            }
            document_.setDuration(row, duration);
        } else
            document_.setDependencies(row, taskDependencyInput(text, *document_.project(), id));
        refresh();
        return true;
    } catch (const std::exception &error) {
        report(error);
        refresh();
        return false;
    }
}
void MainWindow::applyGanttEdit(const std::string &id, std::uint64_t revision, const std::string &taskId, bool resize,
                                const std::string &target) {
    try {
        if (!document_.project() || id != document_.project()->documentId || revision != document_.project()->revision)
            throw AppError(ErrorCode::MAINWINDOWASYNC_001, {});
        if (isEffort(*document_.project())) {
            // The source gesture must refer to a currently editable valid plan.
            if (!document_.calculation() || document_.calculation()->status != CalculationStatus::Valid ||
                document_.project()->statusAt)
                throw AppError(ErrorCode::MAINWINDOWASYNC_002, {});
            submitEdit(GanttTaskEdit{taskId, resize, target}, "간트 공수·날짜 편집",
                       [this](EditState state, const QString &) {
                           if (state == EditState::Applied)
                               calculateSchedule();
                       });
        } else {
            document_.editGantt(id, revision, taskId, resize, target);
            if (document_.project()->revision != revision) {
                refresh();
                calculateSchedule();
            }
        }
    } catch (const std::exception &error) {
        report(error);
        refresh();
    }
}
void MainWindow::undoEdit(bool redo) {
    try {
        commitPendingCell();
        if (editController_->busy()) {
            afterEdit_ = {};
            editController_->cancel();
            return;
        }
        if (document_.historyNeedsWorker(redo)) {
            submitEdit(HistoryEdit{redo}, redo ? "다시 실행" : "실행 취소");
            return;
        }
        if (redo && document_.canRedo())
            document_.redo();
        if (!redo && document_.canUndo())
            document_.undo();
        refresh();
    } catch (const std::exception &error) {
        report(error);
    }
}
} // namespace jobs
