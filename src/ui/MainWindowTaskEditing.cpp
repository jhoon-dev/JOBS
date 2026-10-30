#include "formatting/QtDiagnosticText.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "ui/TaskInput.h"
#include "ui/TaskViewProjection.h"
#include "calculation/WorkCalendar.h"
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSet>
#include <QSignalBlocker>
#include <QMimeData>
#include <QStatusBar>
#include <QTimer>
#include <QResizeEvent>
#include <QVBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QScreen>
#include <QStyle>
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
namespace jobs {
void MainWindow::setupTaskEditing(QMenu *menu, QVBoxLayout *layout) {
    table_->setObjectName("taskTable");
    table_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table_->setSelectionBehavior(QAbstractItemView::SelectItems);
    menu->addSeparator();
    auto action = [&](const QString &name, const char *id, const QKeySequence &key, auto callback) {
        auto *a = menu->addAction(name);
        a->setObjectName(id);
        a->setShortcut(key);
        a->setShortcutContext(Qt::WidgetShortcut);
        table_->addAction(a);
        connect(a, &QAction::triggered, this,
                [this, callback] { runAfterEdit([this, callback] { (this->*callback)(); }); });
        return a;
    };
    insertTasks_ = action("작업 삽입…", "insertTasks", QKeySequence(Qt::Key_Insert), &MainWindow::insertSelectedTasks);
    deleteTasks_ =
        action("선택 작업 삭제…", "deleteTasks", QKeySequence(Qt::Key_Delete), &MainWindow::deleteSelectedTasks);
    duplicateTasks_ =
        action("선택 작업 복제", "duplicateTasks", QKeySequence("Ctrl+D"), &MainWindow::duplicateSelectedTasks);
    copyTasks_ = action("표 선택 복사", "copyTaskCells", QKeySequence::Copy, &MainWindow::copySelectedCells);
    pasteTasks_ = action("표 붙여넣기", "pasteTaskCells", QKeySequence::Paste, &MainWindow::pasteCells);
    auto *editCell = menu->addAction("현재 셀 편집");
    editCell->setObjectName("editTaskCell");
    editCell->setShortcut(QKeySequence(Qt::Key_F2));
    editCell->setShortcutContext(Qt::WidgetShortcut);
    table_->addAction(editCell);
    connect(editCell, &QAction::triggered, this, [this] {
        commitPendingCell();
        const auto index = table_->currentIndex();
        if (index.isValid() && (index.flags() & Qt::ItemIsEditable))
            table_->edit(index);
    });
    auto *all = menu->addAction("표 전체 선택");
    all->setShortcut(QKeySequence::SelectAll);
    all->setShortcutContext(Qt::WidgetShortcut);
    table_->addAction(all);
    connect(all, &QAction::triggered, table_, &QTableView::selectAll);
    auto *strip = new QHBoxLayout;
    taskSearch_ = new QLineEdit;
    taskSearch_->setObjectName("taskSearch");
    taskSearch_->setAccessibleName("작업 이름·ID 검색");
    taskSearch_->setPlaceholderText("작업 이름·ID 검색 (" +
                                    QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText) + ")");
    taskSearch_->setClearButtonEnabled(true);
    taskSearch_->setMinimumWidth(180);
    taskSearch_->setMaximumWidth(360);
    taskSearch_->setToolTip(
        "검색은 표·간트 보기만 바꿉니다. 일정은 전체 프로젝트를 계산하고 보고서는 선택 WBS 범위를 사용합니다.");
    strip->addWidget(taskSearch_, 1);
    taskFilter_ = new QComboBox;
    taskFilter_->setObjectName("taskFilter");
    taskFilter_->setAccessibleName("작업 필터");
    taskFilter_->addItems({"전체 작업", "계획 임계", "마일스톤", "미배정"});
    taskFilter_->setMaximumWidth(140);
    strip->addWidget(taskFilter_);
    taskSort_ = new QComboBox;
    taskSort_->setObjectName("taskSort");
    taskSort_->setAccessibleName("작업 정렬");
    taskSort_->addItems({"프로젝트 원본 순서", "이름 ↑", "이름 ↓", "기간 ↑", "기간 ↓", "시작일 ↑", "시작일 ↓"});
    taskSort_->setMaximumWidth(190);
    strip->addWidget(taskSort_);
    auto *reset = new QPushButton("보기 초기화");
    reset->setObjectName("resetTaskView");
    reset->setMaximumWidth(100);
    strip->addWidget(reset);
    taskViewStatus_ = new QLabel;
    taskViewStatus_->setObjectName("taskViewStatus");
    taskViewStatus_->setTextFormat(Qt::PlainText);
    taskViewStatus_->setWordWrap(false);
    taskViewStatus_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    taskViewStatus_->setMinimumWidth(160);
    taskViewStatus_->setToolTip("선행 번호는 프로젝트 원본 행 번호입니다. 검색·필터는 표와 간트 보기만 바꿉니다.");
    strip->addWidget(taskViewStatus_);
    layout->insertLayout(3, strip);
    auto *find = menu->addAction("작업 검색");
    find->setShortcut(QKeySequence::Find);
    connect(find, &QAction::triggered, this, [this] {
        commitPendingCell();
        taskSearch_->setFocus();
        taskSearch_->selectAll();
    });
    auto *debounce = new QTimer(this);
    debounce->setSingleShot(true);
    debounce->setInterval(180);
    connect(taskSearch_, &QLineEdit::textChanged, debounce, qOverload<>(&QTimer::start));
    connect(debounce, &QTimer::timeout, this, [this] {
        commitPendingCell();
        refresh(false);
    });
    for (auto *combo : {taskFilter_, taskSort_})
        connect(combo, &QComboBox::currentIndexChanged, this, [this] {
            commitPendingCell();
            refresh(false);
        });
    connect(reset, &QPushButton::clicked, this, [this] {
        commitPendingCell();
        const QSignalBlocker a(taskSearch_), b(taskFilter_), c(taskSort_);
        taskSearch_->clear();
        taskFilter_->setCurrentIndex(0);
        taskSort_->setCurrentIndex(0);
        refresh(false);
    });
    table_->horizontalHeader()->setSectionsClickable(true);
    connect(table_->horizontalHeader(), &QHeaderView::sectionClicked, this, [this](int column) {
        int first = column == 0 ? 1 : column == 1 ? 3 : column == 3 ? 5 : 0;
        if (first)
            taskSort_->setCurrentIndex(taskSort_->currentIndex() == first ? first + 1 : first);
    });
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MainWindow::updateTaskActions);
    auto *help = menu->addAction("입력·단축키 도움말");
    help->setObjectName("taskEditingHelp");
    help->setShortcut(QKeySequence(Qt::Key_F1));
    connect(help, &QAction::triggered, this,
            [this] { QMessageBox::information(this, "입력·단축키 도움말", editingHelp_->text()); });
}
void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    updateCompactLayout();
}
void MainWindow::updateCompactLayout() {
    if (!editingHelp_ || !diagnostics_ || !projectTitle_ || !summary_ || !diagnosticsToggle_ || !centralWidget())
        return;
    // Cocoa can constrain a top-level native frame after the QWidget resize
    // callback. Include the screen budget rather than only the requested height.
    const int visibleHeight = screen() ? std::min(height(), screen()->availableGeometry().height()) : height();
    const bool shortWindow = visibleHeight < 520;
    const bool compact = visibleHeight < 760;
    const int mode = shortWindow ? 1 : compact ? 2 : 3;
    if (property("workspaceLayoutMode").toInt() == mode)
        return;
    setProperty("workspaceLayoutMode", mode);
    editingHelp_->hide();
    diagnostics_->setMaximumHeight(compact ? 80 : 140);
    projectTitle_->setVisible(!shortWindow);
    summary_->setVisible(!shortWindow);
    if (auto *layout = qobject_cast<QVBoxLayout *>(centralWidget()->layout())) {
        layout->setContentsMargins(shortWindow ? 8 : 16, shortWindow ? 4 : 12, shortWindow ? 8 : 16,
                                   shortWindow ? 4 : 8);
        layout->setSpacing(shortWindow ? 4 : 8);
    }
    auto *view = findChild<QHBoxLayout *>("workspaceViewRow");
    auto *diagnostic = findChild<QHBoxLayout *>("workspaceDiagnosticRow");
    if (view && diagnostic && diagnosticsToggle_) {
        auto *target = shortWindow ? view : diagnostic;
        if (target->indexOf(diagnosticsToggle_) < 0) {
            (shortWindow ? diagnostic : view)->removeWidget(diagnosticsToggle_);
            target->addWidget(diagnosticsToggle_);
        }
    }
    if (auto *status = findChild<QLabel *>("calculationStatus")) {
        status->setVisible(!shortWindow);
        if (diagnosticsToggle_)
            diagnosticsToggle_->setToolTip(status->text());
    }
    if (auto *tools = findChild<QToolBar *>("primaryTools");
        tools && tools->property("compactWorkspace").toBool() != shortWindow) {
        tools->setProperty("compactWorkspace", shortWindow);
        tools->style()->unpolish(tools);
        tools->style()->polish(tools);
        tools->updateGeometry();
    }
}
void MainWindow::syncTaskSelection() {

    selectedTaskIds_ = selectedTaskIds();
    const auto row = table_->currentRow();
    if (document_.project() && row >= 0 && std::size_t(row) < visibleRows_.size() &&
        table_->selectionModel()->hasSelection())
        selectedTask_ = document_.project()->tasks[visibleRows_[std::size_t(row)]].id;
    else
        selectedTask_.clear();
    if (!selectedTaskIds_.empty() &&
        std::find(selectedTaskIds_.begin(), selectedTaskIds_.end(), selectedTask_) == selectedTaskIds_.end())
        selectedTask_ = selectedTaskIds_.front();
    updateTaskActions();
    assignResources_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    taskCalendar_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    constraints_->setEnabled(!selectedTask_.empty());
    effort_->setEnabled(!selectedTask_.empty());
    taskCost_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    costReport_->setEnabled(!selectedTask_.empty() && !document_.readOnly() &&
                            document_.project()->statusAt.has_value());
    gantt_->setSelectedTask(selectedTask_);
    refreshWorkspaceUi();
    assignWbs_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    progressEdit_->setEnabled(!selectedTask_.empty() && !document_.readOnly() &&
                              document_.project()->statusAt.has_value());
}
std::vector<std::string> MainWindow::selectedTaskIds() const {
    QSet<int> rows;
    for (const auto &range : table_->selectionModel()->selection())
        for (int row = range.top(); row <= range.bottom(); ++row)
            rows.insert(row);
    auto ordered = rows.values();
    std::sort(ordered.begin(), ordered.end());
    std::vector<std::string> ids;
    for (int row : ordered)
        ids.push_back(table_->model()->index(row, 8).data().toString().toUtf8().toStdString());
    return ids;
}
void MainWindow::updateTaskActions() {
    if (!insertTasks_)
        return;
    const bool writable = document_.project() && !document_.readOnly();
    const bool selection = table_->selectionModel()->hasSelection();
    if (auto *edit = findChild<QAction *>("editTaskCell"))
        edit->setEnabled(writable && table_->currentIndex().isValid() &&
                         (table_->currentIndex().flags() & Qt::ItemIsEditable));
    insertTasks_->setEnabled(writable);
    deleteTasks_->setEnabled(writable && selection);
    duplicateTasks_->setEnabled(writable && selection);
    copyTasks_->setEnabled(selection);
    pasteTasks_->setEnabled(
        writable && table_->currentRow() >= 0 && table_->currentColumn() >= 0 && table_->currentColumn() < 3 &&
        (QApplication::clipboard()->mimeData() && QApplication::clipboard()->mimeData()->hasText()));
}
void MainWindow::restoreTaskSelection(int column) {
    if (selectedTaskIds_.empty() && !selectedTask_.empty())
        selectedTaskIds_.push_back(selectedTask_);
    const std::unordered_set<std::string> requested(selectedTaskIds_.begin(), selectedTaskIds_.end());
    std::vector<std::string> kept;
    int active = -1, first = -1;
    QItemSelection selection;
    int run = -1;
    for (int row = 0; row < int(visibleRows_.size()); ++row) {
        const auto &id = document_.project()->tasks[visibleRows_[std::size_t(row)]].id;
        if (!requested.contains(id)) {
            if (run >= 0) {
                selection.select(table_->model()->index(run, 0), table_->model()->index(row - 1, 13));
                run = -1;
            }
            continue;
        }
        if (run < 0)
            run = row;
        kept.push_back(id);
        if (first < 0)
            first = row;
        if (id == selectedTask_)
            active = row;
    }
    if (run >= 0)
        selection.select(table_->model()->index(run, 0), table_->model()->index(int(visibleRows_.size()) - 1, 13));
    table_->selectionModel()->select(selection, QItemSelectionModel::ClearAndSelect);
    selectedTaskIds_ = std::move(kept);
    if (active < 0)
        active = first;
    if (active >= 0) {
        selectedTask_ = document_.project()->tasks[visibleRows_[std::size_t(active)]].id;
        table_->setCurrentCell(active, std::clamp(column, 0, 13), QItemSelectionModel::NoUpdate);
    } else
        selectedTask_.clear();
    updateTaskActions();
}
void MainWindow::applyTaskView() {
    if (!document_.project()) {
        visibleRows_.clear();
        taskViewStatus_->clear();
        return;
    }
    const auto &result = document_.calculation();
    const auto view =
        projectTaskView(*document_.project(), result ? &*result : nullptr,
                        {selectedWbs_, taskSearch_->text(), taskFilter_->currentIndex(), taskSort_->currentIndex()});
    visibleRows_ = view.rows;
    const int sort = taskSort_->currentIndex(), filter = taskFilter_->currentIndex();
    table_->horizontalHeader()->setSortIndicatorShown(sort != 0);
    if (sort)
        table_->horizontalHeader()->setSortIndicator(sort <= 2   ? 0
                                                     : sort <= 4 ? 1
                                                                 : 3,
                                                     sort % 2 ? Qt::AscendingOrder : Qt::DescendingOrder);
    const bool plan = result && result->status == CalculationStatus::Valid && !result->isForecast;
    taskViewStatus_->setText(QString("표시 %1 / WBS 범위 %2").arg(visibleRows_.size()).arg(view.scopeCount) +
                             (filter == 1 && !plan ? " · 최신 계획 계산 필요" : QString{}));
}
void MainWindow::insertSelectedTasks() {
    if (!document_.project() || document_.readOnly())
        return;
    const auto session = document_.sessionGeneration(), revision = document_.project()->revision;
    const auto before = selectedTask_, wbs = selectedWbs_;
    QDialog dialog(this);
    dialog.setObjectName("insertTasksDialog");
    dialog.setWindowTitle("작업 삽입");
    dialog.resize(480, 320);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel(before.empty()
                                     ? "프로젝트 끝에 추가합니다. 작업 이름을 한 줄에 하나씩 입력하세요."
                                     : "현재 작업의 프로젝트 원본 순서 앞에 삽입합니다. 한 줄에 한 작업입니다."));
    auto *names = new QPlainTextEdit;
    names->setObjectName("insertTaskNames");
    names->setPlainText("새 작업");
    layout->addWidget(names);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    try {
        if (document_.sessionGeneration() != session || !document_.project() ||
            document_.project()->revision != revision)
            throw AppError(ErrorCode::MAINWINDOWTASKEDITING_001, {});
        const auto text = names->toPlainText();
        submitEdit(DeferredTaskEdit{[text, before, wbs](const Project &) -> BulkTaskCommand {
                       if (text.size() > 4 * 1024 * 1024 || text.toUtf8().size() > 4 * 1024 * 1024)
                           throw AppError(ErrorCode::MAINWINDOWTASKEDITING_002, {});
                       std::vector<std::string> values;
                       for (const auto &line : text.split('\n')) {
                           calculationCheckpoint();
                           if (!line.trimmed().isEmpty())
                               values.push_back(line.trimmed().toUtf8().toStdString());
                       }
                       return InsertTasksEdit{std::move(values), before, wbs};
                   }},
                   "작업 삽입");
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::deleteSelectedTasks() {
    if (!document_.project() || document_.readOnly())
        return;
    try {
        const auto ids = selectedTaskIds();
        const auto source = document_.snapshot();
        const auto revision = source->revision, session = document_.sessionGeneration();
        auto control = std::make_shared<CalculationControl>();
        QMessageBox confirm(QMessageBox::Warning, "선택 작업 삭제", "삭제 영향을 확인하고 있습니다…",
                            QMessageBox::Yes | QMessageBox::No, this);
        confirm.setObjectName("deleteTasksConfirmation");
        confirm.setDefaultButton(QMessageBox::No);
        confirm.button(QMessageBox::Yes)->setEnabled(false);
        QFutureWatcher<std::pair<TaskRemoval, QString>> watcher;
        bool ready = false;
        connect(&watcher, &QFutureWatcherBase::finished, &confirm, [&] {
            auto [info, error] = watcher.result();
            if (!error.isEmpty()) {
                confirm.setText(error);
                return;
            }
            if (!info.protectedTaskIds.empty()) {
                confirm.setText(
                    "선택에 실적/원가 보고 이력이 있는 작업이 포함돼 삭제할 수 없습니다. 보고 이력을 보존합니다.");
                return;
            }
            confirm.setText(QString("작업 %1개, 선행관계 %2개, 자원 배정 %3개, 작업 예산 %4개를 삭제합니다.\n승인 "
                                    "기준선과 저장 시나리오는 보존됩니다. 실행 취소할 수 있습니다.")
                                .arg(info.tasks)
                                .arg(info.links)
                                .arg(info.assignments)
                                .arg(info.costs));
            ready = true;
            confirm.button(QMessageBox::Yes)->setEnabled(true);
        });
        watcher.setFuture(QtConcurrent::run([source, ids, control] {
            try {
                EvaluationScope scope(control);
                return std::pair{Document::previewRemoveTasks(*source, ids), QString{}};
            } catch (const CalculationCancelled &) {
                return std::pair{TaskRemoval{}, QString("삭제 확인을 취소했습니다.")};
            } catch (const std::exception &e) {
                return std::pair{TaskRemoval{}, qErrorText(e)};
            }
        }));
        const auto answer = confirm.exec();
        control->cancel();
        if (answer != QMessageBox::Yes || !ready)
            return;
        if (document_.sessionGeneration() != session || !document_.project() ||
            document_.project()->revision != revision)
            throw AppError(ErrorCode::MAINWINDOWTASKEDITING_003, {});
        submitEdit(RemoveTasksEdit{ids}, "작업 삭제");
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::duplicateSelectedTasks() {
    try {
        submitEdit(DuplicateTasksEdit{selectedTaskIds()}, "작업 복제");
    } catch (const std::exception &e) {
        report(e);
    }
}

void MainWindow::copySelectedCells() {
    try {
        const auto indices = table_->selectionModel()->selectedIndexes();
        if (indices.isEmpty())
            return;
        int top = indices.front().row(), bottom = top, left = indices.front().column(), right = left;
        for (const auto &i : indices) {
            top = std::min(top, i.row());
            bottom = std::max(bottom, i.row());
            left = std::min(left, i.column());
            right = std::max(right, i.column());
        }
        if (indices.size() != qsizetype(bottom - top + 1) * (right - left + 1))
            throw AppError(ErrorCode::MAINWINDOWTASKEDITING_004, {});
        if (left == 0 && right == table_->columnCount() - 1)
            right = 2;
        if (bottom - top + 1 > 10000 || (bottom - top + 1) * (right - left + 1) > 30000)
            throw AppError(ErrorCode::MAINWINDOWTASKEDITING_005, {});
        QList<QStringList> rows;
        for (int r = top; r <= bottom; ++r) {
            QStringList row;
            for (int c = left; c <= right; ++c)
                if (!table_->isColumnHidden(c))
                    row << table_->model()->index(r, c).data().toString();
            rows << row;
        }
        const auto text = encodeTaskTsv(rows);
        if (text.toUtf8().size() > 4 * 1024 * 1024)
            throw AppError(ErrorCode::MAINWINDOWTASKEDITING_006, {});
        QApplication::clipboard()->setText(text);
        statusBar()->showMessage("선택 표를 복사했습니다.");
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::pasteCells() {
    if (!document_.project() || document_.readOnly())
        return;
    try {
        const auto text = QApplication::clipboard()->text();
        const int row = table_->currentRow(), column = table_->currentColumn();
        submitEdit(DeferredTaskEdit{[text, row, column, rows = visibleRows_](const Project &p) -> BulkTaskCommand {
                       const auto cells = decodeTaskTsv(text);
                       if (cells.isEmpty() || row < 0 || column < 0 || column >= 3 || column + cells.front().size() > 3)
                           throw AppError(ErrorCode::MAINWINDOWTASKEDITING_007, {});
                       if (std::size_t(row) + std::size_t(cells.size()) > rows.size())
                           throw AppError(ErrorCode::MAINWINDOWTASKEDITING_008, {});
                       std::vector<TaskEdit> edits;
                       for (qsizetype r = 0; r < cells.size(); ++r) {
                           calculationCheckpoint();
                           TaskEdit e;
                           e.taskId = p.tasks[rows.at(std::size_t(row) + std::size_t(r))].id;
                           for (qsizetype c = 0; c < cells[r].size(); ++c) {
                               try {
                                   const auto &value = cells[r][c];
                                   if (column + c == 0) {
                                       if (value.trimmed().isEmpty())
                                           throw AppError(ErrorCode::MAINWINDOWTASKEDITING_009, {});
                                       e.name = value.trimmed().toUtf8().toStdString();
                                   } else if (column + c == 1)
                                       e.duration.emplace(taskDurationInput(value));
                                   else
                                       e.edges = taskDependencyInput(value, p, e.taskId);
                               } catch (const std::exception &error) {
                                   throw AppError(ErrorCode::MAINWINDOWTASKEDITING_010,
                                                  {diagnosticArgument(std::to_string(r + 1)),
                                                   diagnosticArgument(std::to_string(c + 1)),
                                                   diagnosticArgument(error.what())});
                               }
                           }
                           edits.push_back(std::move(e));
                       }
                       return BatchTaskEdit{std::move(edits)};
                   }},
                   "표 붙여넣기");
    } catch (const std::exception &e) {
        report(e);
    }
}
} // namespace jobs
