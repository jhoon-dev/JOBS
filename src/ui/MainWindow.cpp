#include "formatting/QtDiagnosticText.h"
#include "ui/MainWindow.h"
#include "ui/ApplicationIcon.h"
#include "ui/DesktopWorkspace.h"
#include "ui/GanttView.h"
#include "ui/ReportDialog.h"
#include "ui/TaskInput.h"
#include "ui/AutoSaveController.h"
#include "ui/TaskTableModel.h"
#include "ui/AsyncEditController.h"
#include "domain/Wbs.h"
#include "calculation/WorkCalendar.h"
#include <QScopedValueRollback>
#include <QTimer>
#include <QAction>
#include <QApplication>
#include <QPointer>
#include <QAbstractItemDelegate>
#include <QCloseEvent>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QInputMethod>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QRegularExpression>
#include <QTreeWidget>
#include <QTreeWidgetItemIterator>
#include <QSplitter>
#include <QScrollBar>
#include <QComboBox>
#include <functional>
#include <cmath>
#include <unordered_map>

namespace jobs {
namespace {
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
std::string str(const QString &s) {
    return s.toUtf8().toStdString();
}
} // namespace
MainWindow::MainWindow(Document &document, JsonRecoveryStore *recoveryStore, DesktopWorkspace *workspace)
    : workspace_(workspace), document_(document) {
    resize(1560, 900);
    setWindowIcon(applicationIcon());
    auto *menu = menuBar()->addMenu("파일(&F)");
    if (workspace_) {
        auto *multiple = menu->addAction("다중 프로젝트·공유 자원…");
        multiple->setObjectName("showWorkspace");
        connect(multiple, &QAction::triggered, this, [this] { workspace_->showWorkspace(this); });
        auto *extra = menu->addAction("새 프로젝트 창");
        extra->setObjectName("newProjectWindow");
        extra->setShortcut(QKeySequence("Ctrl+Shift+N"));
        connect(extra, &QAction::triggered, this, [this] {
            try {
                workspace_->createProject();
            } catch (const std::exception &error) {
                report(error);
            }
        });
        menu->addSeparator();
    }
    auto *tools = addToolBar("파일 및 작업");
    tools->setMovable(false);
    auto action = [&](const QString &name, const QKeySequence &shortcut, std::function<void()> callback) {
        auto *a = menu->addAction(name);
        a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { runAfterEdit(callback); });
        tools->addAction(a);
        return a;
    };
    action("새로 만들기(&N)", QKeySequence::New, [this] { create(); })->setObjectName("newProject");
    action("열기(&O)…", QKeySequence::Open, [this] { open(); })->setObjectName("openProject");
    save_ = action("저장(&S)", QKeySequence::Save, [this] { save(false); });
    saveAs_ = action("다른 이름으로 저장(&A)…", QKeySequence::SaveAs, [this] { save(true); });
    menu->addSeparator();
    auto *importXmlAction = menu->addAction("Project XML 가져오기…");
    importXmlAction->setObjectName("importProjectXml");
    connect(importXmlAction, &QAction::triggered, this, [this] { runAfterEdit([this] { importXml(); }); });
    auto *exportXmlAction = menu->addAction("Project XML 내보내기…");
    exportXmlAction->setObjectName("exportProjectXml");
    connect(exportXmlAction, &QAction::triggered, this, [this] { runAfterEdit([this] { exportXml(); }); });
    auto *importXerAction = menu->addAction("Primavera XER 가져오기…");
    importXerAction->setObjectName("importPrimaveraXer");
    connect(importXerAction, &QAction::triggered, this, [this] { runAfterEdit([this] { importXer(); }); });
    auto *exportXerAction = menu->addAction("Primavera XER 내보내기…");
    exportXerAction->setObjectName("exportPrimaveraXer");
    connect(exportXerAction, &QAction::triggered, this, [this] { runAfterEdit([this] { exportXer(); }); });
    menu->addSeparator();
    auto *quit = menu->addAction("종료(&X)");
    quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    auto *editMenu = menuBar()->addMenu("편집(&E)");
    undo_ = editMenu->addAction("실행 취소");
    undo_->setObjectName("undoEdit");
    undo_->setShortcuts(QKeySequence::Undo);
    redo_ = editMenu->addAction("다시 실행");
    redo_->setObjectName("redoEdit");
    redo_->setShortcuts(QKeySequence::Redo);
    editMenu->setToolTipsVisible(true);
    editMenu->addSeparator();
    editMenu->addAction("승인·제출 기록은 보존 · 실적/원가는 정정 보고로 변경")->setEnabled(false);
    connect(editMenu, &QMenu::aboutToShow, this, &MainWindow::commitPendingCell);
    connect(undo_, &QAction::triggered, this, [this] { undoEdit(false); });
    connect(redo_, &QAction::triggered, this, [this] { undoEdit(true); });
    historyListener_ = document_.subscribe([this](auto event) {
        if (event == Document::Event::Replaced)
            taskViewReset_ = true;
        if (calculationControl_ && calculationSession_ != document_.sessionGeneration()) {
            calculationControl_->cancel();
            queuedCalculation_.reset();
        }
        updateHistoryActions();
    });
    add_ = tools->addAction("작업 추가");
    add_->setShortcut(QKeySequence("Ctrl+Shift+Insert"));
    connect(add_, &QAction::triggered, this, [this] {
        runAfterEdit([this] {
            const auto session = document_.sessionGeneration();
            const auto revision = document_.project() ? document_.project()->revision : 0;
            const auto wbs = selectedWbs_;
            bool ok = false;
            const auto name = QInputDialog::getText(this, "작업 추가", "작업명", QLineEdit::Normal, {}, &ok).trimmed();
            if (!ok)
                return;
            try {
                if (!document_.project() || document_.sessionGeneration() != session ||
                    document_.project()->revision != revision)
                    throw AppError(ErrorCode::MAINWINDOW_001, {});
                submitEdit(InsertTasksEdit{{str(name)}, {}, wbs}, "작업 추가");
            } catch (const std::exception &e) {
                report(e);
            }
        });
    });
    auto *projectMenu = menuBar()->addMenu("프로젝트(&P)");
    calculate_ = projectMenu->addAction("일정 계산(&C)");
    calculate_->setShortcut(QKeySequence(Qt::Key_F9));
    tools->addAction(calculate_);
    connect(calculate_, &QAction::triggered, this, &MainWindow::calculateSchedule);
    cancelCalculation_ = projectMenu->addAction("일정 계산 취소");
    cancelCalculation_->setObjectName("cancelCalculation");
    tools->addAction(cancelCalculation_);
    cancelCalculation_->setEnabled(false);
    connect(cancelCalculation_, &QAction::triggered, this, [this] {
        if (calculationControl_) {
            calculationControl_->cancel();
            queuedCalculation_.reset();
            cancelCalculation_->setEnabled(false);
            calculationProgressLabel_->setText("계산 취소 중…");
        }
    });
    calculationProgressLabel_ = new QLabel;
    calculationProgressLabel_->setObjectName("scheduleProgress");
    statusBar()->addPermanentWidget(calculationProgressLabel_);
    calculationProgressTimer_ = new QTimer(this);
    calculationProgressTimer_->setInterval(100);
    connect(calculationProgressTimer_, &QTimer::timeout, this, [this] {
        if (!calculationControl_ || calculationControl_->cancelRequested.load())
            return;
        const QStringList phases{"입력 검증", "선행관계", "달력·공수", "전진 배치", "후진 여유", "완료"};
        const auto stage = std::size_t(calculationControl_->stage.load());
        calculationProgressLabel_->setText(phases.at(qsizetype(stage)) + " · " +
                                           QString::number(calculationControl_->completed.load()) + " / " +
                                           QString::number(calculationControl_->total.load()));
    });
    calendar_ = projectMenu->addAction("휴일 편집(&H)…");
    tools->addAction(calendar_);
    connect(calendar_, &QAction::triggered, this, &MainWindow::editHolidays);
    calendars_ = projectMenu->addAction("작업·자원 달력 등록·편집…");
    calendars_->setObjectName("manageCalendars");
    connect(calendars_, &QAction::triggered, this, [this] { runAfterEdit([this] { manageCalendars(); }); });
    taskCalendar_ = projectMenu->addAction("선택 작업 달력 배정…");
    taskCalendar_->setObjectName("assignTaskCalendar");
    tools->addAction(taskCalendar_);
    connect(taskCalendar_, &QAction::triggered, this, [this] { runAfterEdit([this] { chooseTaskCalendar(); }); });
    constraints_ = projectMenu->addAction("선택 작업 제약·마감일…");
    constraints_->setObjectName("taskConstraints");
    connect(constraints_, &QAction::triggered, this, [this] { runAfterEdit([this] { editTaskConstraints(); }); });
    effort_ = projectMenu->addAction("선택 작업 공수·유형·분할…");
    effort_->setObjectName("taskEffort");
    tools->addAction(effort_);
    connect(effort_, &QAction::triggered, this, [this] { runAfterEdit([this] { editTaskEffort(); }); });
    scheduleIssues_ = projectMenu->addAction("일정 충돌 진단…");
    scheduleIssues_->setObjectName("scheduleIssues");
    connect(scheduleIssues_, &QAction::triggered, this, [this] { runAfterEdit([this] { showScheduleIssues(); }); });
    auto *wbsMenu = menuBar()->addMenu("WBS(&W)");
    auto wbsAction = [&](const QString &label, std::function<void()> callback) {
        auto *a = wbsMenu->addAction(label);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { runAfterEdit(callback); });
        return a;
    };
    addWbs_ = wbsAction("WBS 추가", [this] { createWbs(); });
    renameWbs_ = wbsAction("WBS 이름 변경", [this] { renameWbs(); });
    moveWbs_ = wbsAction("WBS 이동", [this] { chooseWbs(true); });
    removeWbs_ = wbsAction("빈 WBS 삭제", [this] {
        try {
            document_.removeWbs(selectedWbs_);
            selectedWbs_.clear();
            refresh();
        } catch (const std::exception &e) {
            report(e);
        }
    });
    assignWbs_ = wbsAction("선택 작업의 WBS 변경", [this] { chooseWbs(false); });
    auto *zoom = new QComboBox(this);
    zoom->setAccessibleName("간트 배율");
    zoom->setObjectName("ganttZoom");
    zoom->addItems({"일", "주"});
    auto *trackingMenu = menuBar()->addMenu("추적(&T)");
    auto trackingAction = [&](const QString &label, std::function<void()> callback) {
        auto *a = trackingMenu->addAction(label);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { runAfterEdit(callback); });
        return a;
    };
    baselineSave_ = trackingAction("기준선 저장", [this] { saveBaseline(); });
    statusEdit_ = trackingAction("상태일 설정", [this] { editStatusAt(); });
    progressEdit_ = trackingAction("선택 작업 실적", [this] { reportProgress(); });
    comparison_ = trackingAction("계획 대비 비교", [this] { showComparison(); });
    baselineChoice_ = new QComboBox(this);
    baselineChoice_->setObjectName("baselineChoice");
    baselineChoice_->setAccessibleName("간트 비교 기준선");
    baselineChoice_->setMaximumWidth(190);
    connect(baselineChoice_, &QComboBox::currentIndexChanged, this, [this] {
        selectedBaseline_ = str(baselineChoice_->currentData().toString());
        refresh(false);
    });
    auto *resourceMenu = menuBar()->addMenu("자원(&R)");
    auto resourceAction = [&](const QString &label, const char *object, std::function<void()> callback) {
        auto *a = resourceMenu->addAction(label);
        a->setObjectName(object);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { runAfterEdit(callback); });
        return a;
    };
    resources_ = resourceAction("자원 등록·가용량", "manageResources", [this] { manageResources(); });
    assignResources_ = resourceAction("선택 작업 자원 배정", "assignResources", [this] { assignResources(); });
    resourceComparison_ = resourceAction("기간별 부하·평준화 비교", "resourceComparison", [this] { showResources(); });
    auto *costMenu = menuBar()->addMenu("원가(&C)");
    auto costAction = [&](const QString &label, const char *object, std::function<void()> callback) {
        auto *a = costMenu->addAction(label);
        a->setObjectName(object);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { runAfterEdit(callback); });
        return a;
    };
    rates_ = costAction("통화·자원 단가", "resourceRates", [this] { editRates(); });
    taskCost_ = costAction("선택 작업 예산", "taskCost", [this] { editTaskCost(); });
    costReport_ = costAction("원가·물리 진척 보고", "costReport", [this] { reportCost(); });
    evm_ = costAction("원가·EVM 분석", "costAnalysis", [this] { showCosts(); });
    auto *reportsMenu = menuBar()->addMenu("보고서(&B)");
    reports_ = reportsMenu->addAction("프로젝트 대시보드·보고서…");
    reports_->setObjectName("projectReports");
    connect(reports_, &QAction::triggered, this, [this] {
        runAfterEdit([this] {
            if (document_.project()) {
                ReportDialog dialog(document_, selectedWbs_, selectedBaseline_, this);
                dialog.exec();
            }
        });
    });
    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    central->setObjectName("planningWorkspace");
    layout->setContentsMargins(16, 12, 16, 8);
    layout->setSpacing(8);
    projectTitle_ = new QLabel("프로젝트를 시작하세요", central);
    projectTitle_->setObjectName("projectTitle");
    projectTitle_->setTextFormat(Qt::PlainText);
    layout->addWidget(projectTitle_);
    summary_ = new QLabel(central);
    summary_->setTextFormat(Qt::PlainText);
    summary_->setWordWrap(true);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(summary_);
    wbsSummary_ = new QLabel(central);
    wbsSummary_->setTextFormat(Qt::PlainText);
    wbsSummary_->setObjectName("wbsSummary");
    wbsSummary_->setWordWrap(true);
    layout->addWidget(wbsSummary_);
    documentNotice_ = new QLabel(central);
    documentNotice_->setObjectName("documentNotice");
    documentNotice_->setTextFormat(Qt::PlainText);
    documentNotice_->setWordWrap(true);
    documentNotice_->hide();
    layout->addWidget(documentNotice_);
    auto *split = new QSplitter(Qt::Horizontal, central);
    workspaceSplit_ = split;
    split->setObjectName("planningSplit");
    split->setChildrenCollapsible(false);
    layout->addWidget(split, 1);
    wbsTree_ = new QTreeWidget(split);
    wbsTree_->setObjectName("wbsTree");
    wbsTree_->setAccessibleName("WBS 탐색");
    wbsTree_->setHeaderLabel("WBS · 프로젝트 구조");
    wbsTree_->setMinimumWidth(170);
    table_ = new TaskTableView(split);
    taskModel_ = new TaskTableModel(document_, table_);
    table_->setModel(taskModel_);
    table_->setAccessibleName("프로젝트 작업 목록");
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Interactive);
    table_->setColumnWidth(0, 160);
    // Content sizing must scale with the viewport, not scan 1,000 rows per edit.
    table_->horizontalHeader()->setResizeContentsPrecision(0);
    for (int column = 5; column < 14; ++column)
        table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    table_->setColumnWidth(1, 74);
    table_->setColumnWidth(2, 112);
    table_->setColumnWidth(3, 120);
    table_->setColumnWidth(4, 120);
    table_->setColumnHidden(8,
                            true); // Stable ID remains on the task tooltip; relations persist IDs, never row numbers.
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    table_->setMinimumWidth(440);
    table_->horizontalHeader()->setFixedHeight(GanttView::HeaderHeight);
    table_->verticalHeader()->setDefaultSectionSize(GanttView::RowHeight);
    table_->verticalHeader()->setMinimumSectionSize(GanttView::RowHeight);
    gantt_ = new GanttView(split);
    gantt_->setMinimumWidth(300);
    split->setSizes({180, 690, 650});
    connect(zoom, &QComboBox::currentIndexChanged, this, [this](int i) { gantt_->setDayWidth(i == 0 ? 64 : 16); });
    connect(table_->verticalScrollBar(), &QScrollBar::valueChanged, gantt_->verticalScrollBar(), &QScrollBar::setValue);
    connect(gantt_->verticalScrollBar(), &QScrollBar::valueChanged, table_->verticalScrollBar(), &QScrollBar::setValue);
    connect(wbsTree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item) {
        selectedWbs_ = item ? str(item->data(0, Qt::UserRole).toString()) : std::string{};
        refresh(false);
    });
    connect(table_->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this] { syncTaskSelection(); });
    gantt_->taskSelected = [this](const std::string &id) {
        for (std::size_t i = 0; i < visibleRows_.size(); ++i)
            if (document_.project()->tasks[visibleRows_[i]].id == id) {
                table_->selectRow(int(i));
                table_->scrollTo(table_->model()->index(int(i), 0));
                break;
            }
    };
    diagnostics_ = new QPlainTextEdit(central);
    diagnostics_->setReadOnly(true);
    diagnostics_->setMaximumHeight(160);
    gantt_->editHint = [this](const QString &hint) { statusBar()->showMessage(hint); };
    gantt_->taskEdited = [this](const std::string &id, std::uint64_t revision, const std::string &taskId, bool resize,
                                const std::string &target) { applyGanttEdit(id, revision, taskId, resize, target); };
    diagnostics_->setObjectName("calculationDiagnostics");
    diagnostics_->setAccessibleName("계산 상태 및 진단");
    auto *note = new QLabel(
        "기본 월–금 09–12 / 13–18 · 프로젝트 메뉴에서 작업 달력 등록/배정 · 기간: 시간(8, 1.5) 또는 분(1m), 빈칸=미정, "
        "0=마일스톤\n선행: 프로젝트 행 번호 1, 2SS+2h, 3FF-30m, 4SF+1d · d=480 근무분 · 지연/선행=후행 달력 기준 · "
        "F9=전체 계산\n간트: 빨강+[임계]=계획 임계, 파랑=계획/예측, 초록+[실제]=실제 완료, ◆=마일스톤, 회색=기준선, "
        "배경 음영=프로젝트 기본 휴무\n드래그=날짜 이동 · 오른쪽 끝/Shift=기간 · 이동량 15분 · Esc 취소 · 계획만 편집 "
        "· 적용 후 자동 재계산\n프로젝트 메뉴: 제약·마감일 설정/충돌 진단 · 실제 날짜는 고정 · 예측 여유·임계 미산정",
        central);
    note->setObjectName("taskInputHelp");
    editingHelp_ = note;
    note->setWordWrap(true);
    note->hide();
    setCentralWidget(central);
    taskModel_->editRequested = [this](std::string id, int column, const QString &text) {
        return editTaskCell(std::move(id), column, text);
    };
    setupTaskEditing(editMenu, layout);
    connect(table_->selectionModel(), &QItemSelectionModel::currentChanged, this, [this] { syncTaskSelection(); });
    setupAsyncEditing(tools);
    editMenu->addAction(cancelEdit_);
    setupRecovery(recoveryStore);
    setupWorkspaceUi(tools, zoom);
    refresh();
    updateCompactLayout();
}
MainWindow::~MainWindow() {
    if (editController_) {
        delete editController_;
        editController_ = nullptr;
    }
    if (calculationControl_)
        calculationControl_->cancel();
    document_.unsubscribe(historyListener_);
    if (autosave_)
        autosave_->statusChanged = {};
}
void MainWindow::report(const std::exception &error) {
    const auto diagnostic = diagnosticFromException(error);
    QMessageBox box(QMessageBox::Warning, "JOBS · 작업 실패", qDiagnosticText(diagnostic), QMessageBox::Ok, this);
    box.setTextFormat(Qt::PlainText);
    std::string task;
    if (document_.project())
        for (const auto &id : diagnostic.taskIds)
            if (std::any_of(document_.project()->tasks.begin(), document_.project()->tasks.end(),
                            [&](const auto &item) { return item.id == id; })) {
                task = id;
                break;
            }
    if (task.empty() && document_.project()) {
        const auto &project = *document_.project();
        for (const auto &item : project.tasks) {
            const bool calendar = std::find(diagnostic.calendarIds.begin(), diagnostic.calendarIds.end(),
                                            item.calendarId) != diagnostic.calendarIds.end();
            const bool resource =
                std::any_of(project.assignments.begin(), project.assignments.end(), [&](const auto &assignment) {
                    if (assignment.taskId != item.id)
                        return false;
                    if (std::find(diagnostic.resourceIds.begin(), diagnostic.resourceIds.end(),
                                  assignment.resourceId) != diagnostic.resourceIds.end())
                        return true;
                    return std::any_of(project.resources.begin(), project.resources.end(), [&](const auto &value) {
                        return value.id == assignment.resourceId &&
                               std::find(diagnostic.calendarIds.begin(), diagnostic.calendarIds.end(),
                                         value.calendarId) != diagnostic.calendarIds.end();
                    });
                });
            if (calendar || resource) {
                task = item.id;
                break;
            }
        }
    }
    auto *select = task.empty() ? nullptr : box.addButton("관련 작업 선택", QMessageBox::ActionRole);
    box.exec();
    if (select && box.clickedButton() == select)
        selectTaskById(task);
}
void MainWindow::refresh(bool rebuildTree) {
    refreshWorkspaceUi();
    updateHistoryActions();
    if (refreshIncremental() || refreshProjected() || refreshBulk())
        return;
    EvaluationScope evaluation;
    const bool updates = table_->updatesEnabled();
    table_->setUpdatesEnabled(false);
    struct RestoreUpdates {
        QWidget *widget;
        bool enabled;
        ~RestoreUpdates() {
            widget->setUpdatesEnabled(enabled);
        }
    } restoreUpdates{table_, updates};
    updateHistoryActions();
    const QSignalBlocker blocker(table_);
    const QSignalBlocker selectionBlocker(table_->selectionModel());
    const int selectedColumn = std::max(0, table_->currentColumn());
    const auto &p = document_.project();
    if (p && (treeDocument_ != p->documentId || taskViewReset_)) {
        taskViewReset_ = false;
        selectedTaskIds_.clear();
        const QSignalBlocker searchBlock(taskSearch_), filterBlock(taskFilter_), sortBlock(taskSort_);
        taskSearch_->clear();
        taskFilter_->setCurrentIndex(0);
        taskSort_->setCurrentIndex(0);
        selectedWbs_.clear();
        selectedTask_.clear();
        treeDocument_ = p->documentId;
        selectedBaseline_ = p->baselines.empty() ? std::string{} : p->baselines.back().id;
    }
    if (rebuildTree)
        rebuildWbsTree();
    setWindowTitle((p ? qs(p->name) : "프로젝트를 만들거나 여세요") + "[*] — JOBS");
    setWindowModified(document_.dirty());
    save_->setEnabled(p.has_value());
    saveAs_->setEnabled(p.has_value());
    findChild<QAction *>("exportProjectXml")->setEnabled(p.has_value());
    findChild<QAction *>("exportPrimaveraXer")->setEnabled(p.has_value());
    add_->setEnabled(p.has_value() && !document_.readOnly());
    calculate_->setEnabled(p.has_value() && !calculating_);
    cancelCalculation_->setEnabled(calculating_ && calculationControl_ && !calculationControl_->cancelRequested.load());
    calendar_->setEnabled(p.has_value() && !document_.readOnly());
    calendars_->setEnabled(p.has_value());
    taskCalendar_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty());
    constraints_->setEnabled(p.has_value() && !selectedTask_.empty());
    scheduleIssues_->setEnabled(p.has_value());
    effort_->setEnabled(p.has_value() && !selectedTask_.empty());
    addWbs_->setEnabled(p.has_value() && !document_.readOnly());
    const bool nodeEditable = p.has_value() && !document_.readOnly() && !selectedWbs_.empty();
    renameWbs_->setEnabled(nodeEditable);
    moveWbs_->setEnabled(nodeEditable);
    removeWbs_->setEnabled(nodeEditable);
    applyTaskView();
    taskModel_->synchronize(visibleRows_);
    table_->clearSelection();
    table_->setCurrentCell(-1, -1);
    refreshTracking();
    resources_->setEnabled(p.has_value());
    assignResources_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty());
    resourceComparison_->setEnabled(p.has_value());
    rates_->setEnabled(p.has_value());
    evm_->setEnabled(p.has_value());
    reports_->setEnabled(p.has_value());
    taskCost_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty());
    costReport_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty() &&
                            p->statusAt.has_value());
    if (!p) {
        summary_->setText(QString("새로 만들기(%1) 또는 열기(%2)로 시작하세요.")
                              .arg(QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText),
                                   QKeySequence(QKeySequence::Open).toString(QKeySequence::NativeText)));
        diagnostics_->clear();
        wbsSummary_->clear();
        gantt_->setPlan(nullptr, nullptr, {});
        assignWbs_->setEnabled(false);
        statusBar()->showMessage("준비");
        return;
    }
    refreshDocumentSummary();
    const auto &calculation = document_.calculation();
    std::unordered_map<std::string, int> numbers;
    for (std::size_t row = 0; row < p->tasks.size(); ++row)
        numbers[p->tasks[row].id] = int(row + 1);
    restoreTaskSelection(selectedColumn);
    assignWbs_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    taskCost_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    costReport_->setEnabled(!selectedTask_.empty() && !document_.readOnly() && p->statusAt.has_value());
    refreshTracking();
    assignResources_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    gantt_->setPlan(&*p, calculation ? &*calculation : nullptr, visibleRows_, !document_.readOnly() && !calculating_);
    gantt_->setSelectedTask(selectedTask_);
    refreshBaseline();
    const auto rollup = summarizeWbs(*p, calculation ? &*calculation : nullptr, selectedWbs_);
    renderedRollup_ = rollup;
    refreshWbsSummary();
    QString state = "미계산 · F9로 계산";
    if (calculation) {
        switch (calculation->status) {
        case CalculationStatus::Valid:
            state = (calculation->isForecast ? "실적 반영 예측 완료 · 프로젝트 완료 " : "계산 완료 · 프로젝트 완료 ") +
                    qs(*calculation->projectFinish);
            break;
        case CalculationStatus::Incomplete:
            state = "입력 불완전 · 전체 완료일 없음";
            break;
        case CalculationStatus::Conflict:
            state = "계산 충돌 · 전체 완료일 없음";
            break;
        case CalculationStatus::Stale:
            state = "오래된 계산 · F9로 다시 계산";
            break;
        case CalculationStatus::Cancelled:
            state = "계산 취소 · 부분 결과 없음 · F9로 다시 계산";
            break;
        }
        state += "\n입력 revision " + QString::number(calculation->inputRevision) + " · " +
                 qs(calculation->engineVersion) + " · " + qs(calculation->profile);
        state += " · " + qs(calculation->calculationPolicy);
        if (calculation->status != CalculationStatus::Stale) {
            int count = 0;
            for (const auto &d : calculation->diagnostics) {
                if (++count > 20) {
                    state += "\n추가 진단 " + QString::number(calculation->diagnostics.size() - 20) + "개";
                    break;
                }
                state += "\n[" + qs(d.code) + "] " + qs(diagnosticMessage(d));
                if (!d.taskIds.empty()) {
                    QStringList path;
                    for (const auto &id : d.taskIds)
                        path << (numbers.contains(id) ? QString::number(numbers.at(id)) : qs(id));
                    state += " (작업 경로: " + path.join(" → ") + ")";
                }
            }
        }
    }
    diagnostics_->setPlainText(calculating_ ? "계산 중 · 편집할 수 있습니다. 변경 시 계산 결과는 오래됨으로 처리됩니다."
                                            : state);
    rendered_ = true;
    renderedView_ = viewKey();
    renderedSession_ = document_.sessionGeneration();
    renderedRevision_ = p->revision;
    renderedCalculation_ = document_.calculationGeneration();
    refreshStatus();
}
void MainWindow::calculateSchedule() {
    commitPendingCell();
    if (editController_ && editController_->busy()) {
        runAfterEdit([this] { calculateSchedule(); });
        return;
    }
    if (!document_.project())
        return;
    if (calculating_) {
        queuedCalculation_ = document_.sessionGeneration();
        return;
    }
    const auto snapshot = document_.snapshot();
    calculating_ = true;
    calculationControl_ = std::make_shared<CalculationControl>();
    const auto control = calculationControl_;
    calculationSession_ = document_.sessionGeneration();
    calculationProgressLabel_->setText("일정 계산 준비 중…");
    calculationProgressTimer_->start();
    refresh();
    auto *watcher = new QFutureWatcher<ScheduleResult>(this);
    connect(watcher, &QFutureWatcher<ScheduleResult>::finished, this,
            [this, watcher, control, session = document_.sessionGeneration()] {
                calculating_ = false;
                calculationProgressTimer_->stop();
                calculationProgressLabel_->clear();
                calculationControl_.reset();
                try {
                    if (document_.sessionGeneration() == session) {
                        auto result = watcher->result();
                        if (control->cancelRequested.load()) {
                            result.status = CalculationStatus::Cancelled;
                            result.tasks.clear();
                            result.plannedTasks.clear();
                            result.projectFinish.reset();
                            result.diagnostics = {{"CALCULATION_CANCELLED", {}, {}, "CALCULATION_CANCELLED_2"}};
                        }
                        document_.applyCalculation(std::move(result));
                    }
                } catch (const std::exception &e) {
                    report(e);
                }
                watcher->deleteLater();
                refresh();
                const auto queued = queuedCalculation_;
                queuedCalculation_.reset();
                if (queued && *queued == document_.sessionGeneration())
                    calculateSchedule();
            });
    watcher->setFuture(QtConcurrent::run([snapshot, control] { return jobs::calculate(*snapshot, control); }));
}
void MainWindow::rebuildWbsTree() {
    if (document_.project()) {
        const auto &p = *document_.project();
        if (cachedTreeDocument_ == p.documentId && cachedTreeName_ == p.name && cachedTreeNodes_ == p.wbsNodes &&
            wbsTree_->currentItem() && str(wbsTree_->currentItem()->data(0, Qt::UserRole).toString()) == selectedWbs_)
            return;
        cachedTreeDocument_ = p.documentId;
        cachedTreeName_ = p.name;
        cachedTreeNodes_ = p.wbsNodes;
    } else {
        cachedTreeDocument_.clear();
        cachedTreeName_.clear();
        cachedTreeNodes_.clear();
    }
    const QSignalBlocker blocker(wbsTree_);
    QSet<QString> expanded;
    for (QTreeWidgetItemIterator it(wbsTree_); *it; ++it)
        if ((*it)->isExpanded())
            expanded.insert((*it)->data(0, Qt::UserRole).toString());
    wbsTree_->clear();
    if (!document_.project())
        return;
    const auto &p = *document_.project();
    auto *root = new QTreeWidgetItem(wbsTree_, {qs(p.name) + " · 전체 작업"});
    root->setData(0, Qt::UserRole, QString{});
    root->setToolTip(0, "프로젝트 전체 작업 · 하위 WBS와 미분류 작업 포함");
    root->setExpanded(true);
    std::unordered_map<std::string, QTreeWidgetItem *> items{{"", root}};
    for (const auto &node : p.wbsNodes) {
        auto *item = new QTreeWidgetItem(QStringList{qs(node.name)});
        item->setData(0, Qt::UserRole, qs(node.id));
        item->setToolTip(0, "WBS ID: " + qs(node.id));
        items.emplace(node.id, item);
    }
    for (const auto &node : p.wbsNodes) {
        items.at(node.parentId)->addChild(items.at(node.id));
        items.at(node.id)->setExpanded(expanded.contains(qs(node.id)));
    }
    if (!items.contains(selectedWbs_))
        selectedWbs_.clear();
    auto *selected = items.at(selectedWbs_);
    wbsTree_->setCurrentItem(selected);
    for (auto *ancestor = selected->parent(); ancestor; ancestor = ancestor->parent())
        ancestor->setExpanded(true);
}
void MainWindow::createWbs() {
    bool ok = false;
    auto name =
        QInputDialog::getText(this, "WBS 추가", "선택한 WBS 아래에 만들 이름", QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok)
        return;
    try {
        selectedWbs_ = document_.addWbs(str(name), selectedWbs_);
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::renameWbs() {
    if (selectedWbs_.empty())
        return;
    bool ok = false;
    auto name =
        QInputDialog::getText(this, "WBS 이름 변경", "이름", QLineEdit::Normal, wbsTree_->currentItem()->text(0), &ok)
            .trimmed();
    if (!ok)
        return;
    try {
        document_.renameWbs(selectedWbs_, str(name));
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::chooseWbs(bool moveNode) {
    if (!document_.project() || document_.readOnly())
        return;
    const auto nodeId = selectedWbs_, taskId = selectedTask_;
    if ((moveNode && nodeId.empty()) || (!moveNode && taskId.empty()))
        return;
    const auto &p = *document_.project();
    QDialog dialog(this);
    dialog.setWindowTitle(moveNode ? "WBS 이동" : "작업 WBS 변경");
    dialog.resize(450, 180);
    auto *layout = new QFormLayout(&dialog);
    auto *choice = new QComboBox(&dialog);
    choice->setObjectName("wbsDestination");
    choice->setAccessibleName("대상 WBS");
    choice->addItem("프로젝트 루트 / 미분류", QString{});
    std::unordered_map<std::string, const WbsNode *> nodes;
    for (const auto &node : p.wbsNodes)
        nodes.emplace(node.id, &node);
    const auto excluded = moveNode ? wbsSubtree(p, nodeId) : std::unordered_set<std::string>{};
    for (const auto &node : p.wbsNodes) {
        if (excluded.contains(node.id))
            continue;
        QStringList path{qs(node.name)};
        auto parent = node.parentId;
        while (!parent.empty()) {
            path.prepend(qs(nodes.at(parent)->name));
            parent = nodes.at(parent)->parentId;
        }
        choice->addItem(path.join(" / "), qs(node.id));
    }
    if (moveNode)
        choice->setCurrentIndex(choice->findData(qs(nodes.at(nodeId)->parentId)));
    else
        for (const auto &task : p.tasks)
            if (task.id == taskId)
                choice->setCurrentIndex(choice->findData(qs(task.wbsId)));
    layout->addRow("대상 WBS", choice);
    layout->addRow(new QLabel("작업 ID와 실행 선행관계는 유지됩니다.", &dialog));
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    try {
        auto target = str(choice->currentData().toString());
        if (moveNode)
            document_.moveWbs(nodeId, target);
        else
            for (std::size_t i = 0; i < document_.project()->tasks.size(); ++i)
                if (document_.project()->tasks[i].id == taskId) {
                    document_.assignWbs(i, target);
                    break;
                }
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::editHolidays() {
    commitPendingCell();
    if (!document_.project() || document_.readOnly())
        return;
    QDialog dialog(this);
    dialog.setWindowTitle("휴일 편집");
    dialog.resize(400, 350);
    auto *layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("하루 전체 휴일 · 한 줄에 YYYY-MM-DD 한 개\n주말은 기본적으로 쉽니다.", &dialog));
    auto *dates = new QPlainTextEdit(&dialog);
    dates->setAccessibleName("휴일 날짜 목록");
    QStringList lines;
    for (const auto &d : document_.project()->holidays)
        lines << qs(d);
    dates->setPlainText(lines.join('\n'));
    layout->addWidget(dates);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return;
    std::vector<std::string> holidays;
    for (auto line : dates->toPlainText().split('\n'))
        if (!line.trimmed().isEmpty())
            holidays.push_back(str(line.trimmed()));
    try {
        document_.setHolidays(std::move(holidays));
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
bool MainWindow::confirmChanges() {
    commitPendingCell();
    if (!document_.dirty())
        return true;
    auto result =
        QMessageBox::warning(this, "미저장 변경", "현재 프로젝트의 변경 내용을 저장하시겠습니까?",
                             QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (result == QMessageBox::Cancel)
        return false;
    return result != QMessageBox::Save || save(false);
}
bool MainWindow::save(bool as) {
    commitPendingCell();
    if (!document_.project())
        return false;
    try {
        if (as || document_.path().empty() || document_.readOnly() || document_.requiresSaveAs()) {
            QFileDialog dialog(this, "다른 이름으로 저장 · 기존 파일은 덮어쓰지 않습니다");
            dialog.setAcceptMode(QFileDialog::AcceptSave);
            dialog.setNameFilter("JOBS 프로젝트 (*.jobs)");
            dialog.setDefaultSuffix("jobs");
            dialog.selectFile(qs(document_.project()->name) + ".jobs");
            if (dialog.exec() != QDialog::Accepted)
                return false;
            document_.saveAs(str(dialog.selectedFiles().first()));
        } else
            document_.save();
        refresh();
        statusBar()->showMessage("저장 완료");
        return true;
    } catch (const std::exception &e) {
        report(e);
        return false;
    }
}
void MainWindow::commitPendingCell() {
    auto *focused = QApplication::focusWidget();
    if (focused && qobject_cast<QLineEdit *>(focused) && table_->isAncestorOf(focused)) {
        if (auto *input = QApplication::inputMethod())
            input->commit();
    }
    table_->commitEditors();
}
void MainWindow::create() {
    if (workspace_ && document_.project()) {
        workspace_->createProject();
        return;
    }
    QDialog dialog(this);
    dialog.setWindowTitle("새 프로젝트");
    dialog.resize(480, 440);
    auto *layout = new QFormLayout(&dialog);
    auto *name = new QLineEdit(&dialog);
    name->setAccessibleName("프로젝트명");
    auto *date = new QDateEdit(QDate::currentDate(), &dialog);
    date->setDisplayFormat("yyyy-MM-dd");
    date->setCalendarPopup(true);
    date->setAccessibleName("프로젝트 시작일");
    auto *tasks = new QPlainTextEdit(&dialog);
    tasks->setPlaceholderText("설계\n개발\n검증");
    tasks->setAccessibleName("작업명 목록 · 한 줄에 하나");
    auto *error = new QLabel(&dialog);
    error->setWordWrap(true);
    layout->addRow("프로젝트명(&N)", name);
    layout->addRow("시작일(&D)", date);
    layout->addRow("기본 근무달력", new QLabel("월–금 · 09–12 / 13–18\n1일 8시간 · 휴일은 생성 후 편집", &dialog));
    layout->addRow("작업명 목록(&T)", tasks);
    layout->addRow(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().isEmpty()) {
            error->setText("프로젝트명을 입력하세요.");
            name->setFocus();
            return;
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted || !confirmChanges())
        return;
    std::vector<std::string> names;
    for (auto line : tasks->toPlainText().split('\n'))
        if (!line.trimmed().isEmpty())
            names.push_back(str(line.trimmed()));
    try {
        document_.create(str(name->text().trimmed()), str(date->date().toString(Qt::ISODate)), names);
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::open() {
    auto path = QFileDialog::getOpenFileName(this, "프로젝트 열기", {}, "JOBS 프로젝트 (*.jobs);;모든 파일 (*)");
    if (path.isEmpty())
        return;
    if (workspace_) {
        try {
            workspace_->openProject(str(path));
        } catch (const AppError &error) {
            if (error.code() == "FILE_LOCKED" &&
                QMessageBox::question(this, "프로젝트 잠김", qErrorText(error) + "\n읽기 전용으로 여시겠습니까?",
                                      QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
                try {
                    workspace_->openProject(str(path), true);
                } catch (const std::exception &inner) {
                    report(inner);
                }
            } else if (error.code() != "FILE_LOCKED")
                report(error);
        } catch (const std::exception &error) {
            report(error);
        }
        return;
    }
    if (!confirmChanges())
        return;
    try {
        document_.open(str(path));
        refresh();
    } catch (const std::exception &e) {
        auto result =
            QMessageBox::warning(this, "열기 실패", qErrorText(e) + "\n읽기 전용으로 열기를 시도하시겠습니까?",
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result == QMessageBox::Yes) {
            try {
                document_.open(str(path), true);
                refresh();
            } catch (const std::exception &inner) {
                report(inner);
            }
        }
    }
}
void MainWindow::closeEvent(QCloseEvent *event) {
    commitPendingCell();
    if (editController_ && editController_->busy()) {
        runAfterEdit([this] { close(); });
        event->ignore();
        return;
    }
    if (baselinePreparing_) {
        QMessageBox::information(this, "기준선 저장 중", "기준선 계산을 마친 후 창을 닫으세요.");
        event->ignore();
        return;
    }
    if (confirmChanges() && (!workspace_ || workspace_->confirmClosing(this))) {
        if (calculationControl_) {
            calculationControl_->cancel();
            queuedCalculation_.reset();
        }
        if (autosave_)
            autosave_->finishDocument();
        event->accept();
    } else
        event->ignore();
}
} // namespace jobs
