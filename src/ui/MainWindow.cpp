#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "ui/ReportDialog.h"
#include "ui/TaskInput.h"
#include "ui/AutoSaveController.h"
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
#include <QTableWidget>
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
class TaskTable final : public QTableWidget {
public:
    using QTableWidget::QTableWidget;
    void commitEditors() {
        const auto editors = findChildren<QLineEdit*>();
        for (auto* editor : editors) if (editor->isVisible()) {
            QPointer<QWidget> guard(editor);
            commitData(editor);
            if (guard) closeEditor(guard.data(), QAbstractItemDelegate::NoHint);
        }
    }
};
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
}
MainWindow::MainWindow(Document& document, JsonRecoveryStore* recoveryStore) : document_(document) {
    resize(1560, 900);
    auto* menu = menuBar()->addMenu("파일(&F)");
    auto* tools = addToolBar("파일 및 작업"); tools->setMovable(false);
    auto action = [&](const QString& name, const QKeySequence& shortcut, std::function<void()> callback) {
        auto* a = menu->addAction(name); a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { commitPendingCell(); callback(); }); tools->addAction(a); return a;
    };
    action("새로 만들기(&N)", QKeySequence::New, [this] { create(); });
    action("열기(&O)…", QKeySequence::Open, [this] { open(); });
    save_ = action("저장(&S)", QKeySequence::Save, [this] { save(false); });
    saveAs_ = action("다른 이름으로 저장(&A)…", QKeySequence::SaveAs, [this] { save(true); });
    menu->addSeparator();
    auto* importXmlAction=menu->addAction("Project XML 가져오기…");importXmlAction->setObjectName("importProjectXml");
    connect(importXmlAction,&QAction::triggered,this,[this]{commitPendingCell();importXml();});
    auto* exportXmlAction=menu->addAction("Project XML 내보내기…");exportXmlAction->setObjectName("exportProjectXml");
    connect(exportXmlAction,&QAction::triggered,this,[this]{commitPendingCell();exportXml();});
    menu->addSeparator();
    auto* quit = menu->addAction("종료(&X)"); quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    auto* editMenu = menuBar()->addMenu("편집(&E)");
    undo_ = editMenu->addAction("실행 취소"); undo_->setObjectName("undoEdit"); undo_->setShortcuts(QKeySequence::Undo);
    redo_ = editMenu->addAction("다시 실행"); redo_->setObjectName("redoEdit"); redo_->setShortcuts(QKeySequence::Redo);
    editMenu->setToolTipsVisible(true); editMenu->addSeparator();
    editMenu->addAction("승인·제출 기록은 보존 · 실적/원가는 정정 보고로 변경")->setEnabled(false);
    connect(editMenu, &QMenu::aboutToShow, this, &MainWindow::commitPendingCell);
    connect(undo_, &QAction::triggered, this, [this] { try { commitPendingCell(); if (document_.canUndo()) document_.undo(); refresh(); } catch (const std::exception& e) { report(e); } });
    connect(redo_, &QAction::triggered, this, [this] { try { commitPendingCell(); if (document_.canRedo()) document_.redo(); refresh(); } catch (const std::exception& e) { report(e); } });
    historyListener_ = document_.subscribe([this](auto event) { if(event==Document::Event::Replaced)taskViewReset_=true; if(calculationControl_&&calculationSession_!=document_.sessionGeneration()){calculationControl_->cancel();queuedCalculation_.reset();}updateHistoryActions(); });
    add_ = tools->addAction("작업 추가"); add_->setShortcut(QKeySequence("Ctrl+Shift+Insert"));
    connect(add_, &QAction::triggered, this, [this] {
        commitPendingCell();
        bool ok = false;
        auto name = QInputDialog::getText(this, "작업 추가", "작업명", QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok) return;
        try { document_.addTask(str(name), selectedWbs_); refresh(); } catch (const std::exception& e) { report(e); }
    });
    auto* projectMenu = menuBar()->addMenu("프로젝트(&P)");
    calculate_ = projectMenu->addAction("일정 계산(&C)"); calculate_->setShortcut(QKeySequence(Qt::Key_F9));
    tools->addAction(calculate_); connect(calculate_, &QAction::triggered, this, &MainWindow::calculateSchedule);
    cancelCalculation_=projectMenu->addAction("일정 계산 취소");cancelCalculation_->setObjectName("cancelCalculation");tools->addAction(cancelCalculation_);cancelCalculation_->setEnabled(false);
    connect(cancelCalculation_,&QAction::triggered,this,[this]{if(calculationControl_){calculationControl_->cancel();queuedCalculation_.reset();cancelCalculation_->setEnabled(false);calculationProgressLabel_->setText("계산 취소 중…");}});
    calculationProgressLabel_=new QLabel;calculationProgressLabel_->setObjectName("scheduleProgress");tools->addWidget(calculationProgressLabel_);
    calculationProgressTimer_=new QTimer(this);calculationProgressTimer_->setInterval(100);
    connect(calculationProgressTimer_,&QTimer::timeout,this,[this]{if(!calculationControl_||calculationControl_->cancelRequested.load())return;const QStringList phases{"입력 검증","선행관계","달력·공수","전진 배치","후진 여유","완료"};const auto stage=std::size_t(calculationControl_->stage.load());calculationProgressLabel_->setText(phases.at(qsizetype(stage))+" · "+QString::number(calculationControl_->completed.load())+" / "+QString::number(calculationControl_->total.load()));});
    calendar_ = projectMenu->addAction("휴일 편집(&H)…"); tools->addAction(calendar_);
    connect(calendar_, &QAction::triggered, this, &MainWindow::editHolidays);
    calendars_=projectMenu->addAction("작업·자원 달력 등록·편집…"); calendars_->setObjectName("manageCalendars");
    connect(calendars_,&QAction::triggered,this,[this]{commitPendingCell();manageCalendars();});
    taskCalendar_=projectMenu->addAction("선택 작업 달력 배정…"); taskCalendar_->setObjectName("assignTaskCalendar"); tools->addAction(taskCalendar_);
    connect(taskCalendar_,&QAction::triggered,this,[this]{commitPendingCell();chooseTaskCalendar();});
    constraints_=projectMenu->addAction("선택 작업 제약·마감일…");constraints_->setObjectName("taskConstraints");
    connect(constraints_,&QAction::triggered,this,[this]{commitPendingCell();editTaskConstraints();});
    effort_=projectMenu->addAction("선택 작업 공수·유형·분할…");effort_->setObjectName("taskEffort");tools->addAction(effort_);
    connect(effort_,&QAction::triggered,this,[this]{commitPendingCell();editTaskEffort();});
    scheduleIssues_=projectMenu->addAction("일정 충돌 진단…");scheduleIssues_->setObjectName("scheduleIssues");
    connect(scheduleIssues_,&QAction::triggered,this,[this]{commitPendingCell();showScheduleIssues();});
    auto* wbsMenu = menuBar()->addMenu("WBS(&W)");
    addToolBarBreak(); auto* wbsTools = addToolBar("WBS 편집"); wbsTools->setMovable(false);
    auto wbsAction = [&](const QString& label, std::function<void()> callback) {
        auto* a = wbsMenu->addAction(label); wbsTools->addAction(a);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { commitPendingCell(); callback(); }); return a;
    };
    addWbs_ = wbsAction("WBS 추가", [this] { createWbs(); });
    renameWbs_ = wbsAction("WBS 이름 변경", [this] { renameWbs(); });
    moveWbs_ = wbsAction("WBS 이동", [this] { chooseWbs(true); });
    removeWbs_ = wbsAction("빈 WBS 삭제", [this] {
        try { document_.removeWbs(selectedWbs_); selectedWbs_.clear(); refresh(); } catch (const std::exception& e) { report(e); }
    });
    assignWbs_ = wbsAction("선택 작업의 WBS 변경", [this] { chooseWbs(false); });
    wbsTools->addSeparator(); wbsTools->addWidget(new QLabel("간트 배율 ", this));
    auto* zoom = new QComboBox(this); zoom->setAccessibleName("간트 배율"); zoom->setObjectName("ganttZoom");
    zoom->addItems({"일", "주"}); wbsTools->addWidget(zoom);
    auto* trackingMenu = menuBar()->addMenu("추적(&T)");
    auto trackingAction = [&](const QString& label, std::function<void()> callback) {
        auto* a = trackingMenu->addAction(label); wbsTools->addAction(a);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { commitPendingCell(); callback(); }); return a;
    };
    wbsTools->addSeparator();
    baselineSave_ = trackingAction("기준선 저장", [this] { saveBaseline(); });
    statusEdit_ = trackingAction("상태일 설정", [this] { editStatusAt(); });
    progressEdit_ = trackingAction("선택 작업 실적", [this] { reportProgress(); });
    comparison_ = trackingAction("계획 대비 비교", [this] { showComparison(); });
    baselineChoice_ = new QComboBox(this); baselineChoice_->setObjectName("baselineChoice"); baselineChoice_->setAccessibleName("간트 비교 기준선");
    baselineChoice_->setMaximumWidth(190); wbsTools->addWidget(baselineChoice_);
    connect(baselineChoice_, &QComboBox::currentIndexChanged, this, [this] {
        selectedBaseline_ = str(baselineChoice_->currentData().toString()); refresh(false);
    });
    auto* resourceMenu = menuBar()->addMenu("자원(&R)");
    addToolBarBreak(); auto* resourceTools = addToolBar("자원 및 평준화"); resourceTools->setMovable(false);
    auto resourceAction = [&](const QString& label, const char* object, std::function<void()> callback) {
        auto* a = resourceMenu->addAction(label); a->setObjectName(object); resourceTools->addAction(a);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { commitPendingCell(); callback(); }); return a;
    };
    resources_ = resourceAction("자원 등록·가용량", "manageResources", [this] { manageResources(); });
    assignResources_ = resourceAction("선택 작업 자원 배정", "assignResources", [this] { assignResources(); });
    resourceComparison_ = resourceAction("기간별 부하·평준화 비교", "resourceComparison", [this] { showResources(); });
    auto* costMenu = menuBar()->addMenu("원가(&C)");
    auto costAction = [&](const QString& label, const char* object, std::function<void()> callback) {
        auto* a = costMenu->addAction(label); a->setObjectName(object); resourceTools->addAction(a);
        connect(a, &QAction::triggered, this, [this, callback = std::move(callback)] { commitPendingCell(); callback(); }); return a;
    };
    resourceTools->addSeparator();
    rates_ = costAction("통화·자원 단가", "resourceRates", [this] { editRates(); });
    taskCost_ = costAction("선택 작업 예산", "taskCost", [this] { editTaskCost(); });
    costReport_ = costAction("원가·물리 진척 보고", "costReport", [this] { reportCost(); });
    evm_ = costAction("원가·EVM 분석", "costAnalysis", [this] { showCosts(); });
    auto* reportsMenu = menuBar()->addMenu("보고서(&B)");
    reports_ = reportsMenu->addAction("프로젝트 대시보드·보고서…"); reports_->setObjectName("projectReports");
    resourceTools->addSeparator(); resourceTools->addAction(reports_);
    connect(reports_, &QAction::triggered, this, [this] { commitPendingCell(); if (document_.project()) { ReportDialog dialog(document_, selectedWbs_, selectedBaseline_, this); dialog.exec(); } });
    auto* central = new QWidget(this); auto* layout = new QVBoxLayout(central);
    auto* heading = new QLabel("<h1>JOBS</h1>프로젝트 일정 · 로컬 프로젝트 파일", central);
    layout->addWidget(heading);
    summary_ = new QLabel(central); summary_->setTextFormat(Qt::PlainText); summary_->setWordWrap(true);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(summary_);
    wbsSummary_ = new QLabel(central); wbsSummary_->setTextFormat(Qt::PlainText); wbsSummary_->setObjectName("wbsSummary");
    wbsSummary_->setWordWrap(true); layout->addWidget(wbsSummary_);
    auto* split = new QSplitter(Qt::Horizontal, central); layout->addWidget(split, 1);
    wbsTree_ = new QTreeWidget(split); wbsTree_->setObjectName("wbsTree"); wbsTree_->setAccessibleName("WBS 탐색");
    wbsTree_->setHeaderLabel("WBS · 프로젝트 구조"); wbsTree_->setMinimumWidth(170);
    table_ = new TaskTable(0, 14, split);
    table_->setHorizontalHeaderLabels({"작업명", "기간(h / m)", "선행(행·종류±지연)", "시작", "완료", "총여유(h)", "임계/상태", "자원", "안정 ID", "작업 달력", "제약", "마감일", "작업 유형 · 입력 공수(h)", "분할 중단"});
    table_->setAccessibleName("프로젝트 작업 목록");
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 14; ++column) table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    table_->setColumnHidden(8, true); // Stable ID remains on the task tooltip; relations persist IDs, never row numbers.
    table_->setSelectionBehavior(QAbstractItemView::SelectRows); table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel); table_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    table_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn); table_->setMinimumWidth(440);
    table_->horizontalHeader()->setFixedHeight(GanttView::HeaderHeight);
    table_->verticalHeader()->setDefaultSectionSize(GanttView::RowHeight);
    table_->verticalHeader()->setMinimumSectionSize(GanttView::RowHeight);
    gantt_ = new GanttView(split); gantt_->setMinimumWidth(300); split->setSizes({200, 750, 600});
    connect(zoom, &QComboBox::currentIndexChanged, this, [this](int i) { gantt_->setDayWidth(i == 0 ? 64 : 16); });
    connect(table_->verticalScrollBar(), &QScrollBar::valueChanged, gantt_->verticalScrollBar(), &QScrollBar::setValue);
    connect(gantt_->verticalScrollBar(), &QScrollBar::valueChanged, table_->verticalScrollBar(), &QScrollBar::setValue);
    connect(wbsTree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* item) {
        selectedWbs_ = item ? str(item->data(0, Qt::UserRole).toString()) : std::string{}; refresh(false);
    });
    connect(table_, &QTableWidget::itemSelectionChanged,this,&MainWindow::syncTaskSelection);
    gantt_->taskSelected = [this](const std::string& id) {
        for (std::size_t i = 0; i < visibleRows_.size(); ++i)
            if (document_.project()->tasks[visibleRows_[i]].id == id) { table_->selectRow(int(i)); table_->scrollToItem(table_->item(int(i), 0)); break; }
    };
    diagnostics_ = new QPlainTextEdit(central); diagnostics_->setReadOnly(true); diagnostics_->setMaximumHeight(160);
    gantt_->editHint=[this](const QString& hint){statusBar()->showMessage(hint);};
    gantt_->taskEdited=[this](const std::string& id,std::uint64_t revision,const std::string& taskId,bool resize,const std::string& target){try{const auto before=document_.project()?document_.project()->revision:0;document_.editGantt(id,revision,taskId,resize,target);if(document_.project()&&document_.project()->revision!=before){refresh();calculateSchedule();}}catch(const std::exception& e){report(e);refresh();}};
    diagnostics_->setObjectName("calculationDiagnostics"); diagnostics_->setAccessibleName("계산 상태 및 진단");
    layout->addWidget(diagnostics_);
    auto* note = new QLabel("기본 월–금 09–12 / 13–18 · 프로젝트 메뉴에서 작업 달력 등록/배정 · 기간: 시간(8, 1.5) 또는 분(1m), 빈칸=미정, 0=마일스톤\n선행: 프로젝트 행 번호 1, 2SS+2h, 3FF-30m, 4SF+1d · d=480 근무분 · 지연/선행=후행 달력 기준 · F9=전체 계산\n간트: 빨강+[임계]=계획 임계, 파랑=계획/예측, 초록+[실제]=실제 완료, ◆=마일스톤, 회색=기준선, 배경 음영=프로젝트 기본 휴무\n드래그=날짜 이동 · 오른쪽 끝/Shift=기간 · 이동량 15분 · Esc 취소 · 계획만 편집 · 적용 후 자동 재계산\n프로젝트 메뉴: 제약·마감일 설정/충돌 진단 · 실제 날짜는 고정 · 예측 여유·임계 미산정", central);
    note->setObjectName("taskInputHelp");editingHelp_=note;
    note->setWordWrap(true); layout->addWidget(note); setCentralWidget(central);
    connect(table_, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (column > 2) return;
        try {
            const auto text = table_->item(row, column)->text().trimmed();
            const auto taskRow = visibleRows_.at(std::size_t(row));
            const auto& id=document_.project()->tasks.at(taskRow).id;
            if(column==0)document_.renameTask(taskRow,str(text));
            else if(column==1)document_.setDuration(taskRow,taskDurationInput(text));
            else document_.setDependencies(taskRow,taskDependencyInput(text,*document_.project(),id));
        }
        catch (const std::exception& e) { report(e); }
        refresh();
    });
    setupTaskEditing(editMenu,layout);
    connect(table_,&QTableWidget::currentCellChanged,this,&MainWindow::syncTaskSelection);
    setupRecovery(recoveryStore); refresh();updateCompactLayout();
}
MainWindow::~MainWindow() { if(calculationControl_)calculationControl_->cancel();document_.unsubscribe(historyListener_); if (autosave_) autosave_->statusChanged = {}; }
void MainWindow::report(const std::exception& e) { QMessageBox::warning(this, "JOBS · 작업 실패", QString::fromUtf8(e.what())); }
void MainWindow::refresh(bool rebuildTree) {
    EvaluationScope evaluation;
    const bool updates=table_->updatesEnabled();table_->setUpdatesEnabled(false);
    struct RestoreUpdates { QWidget* widget;bool enabled;~RestoreUpdates(){widget->setUpdatesEnabled(enabled);} } restoreUpdates{table_,updates};
    updateHistoryActions();
    const QSignalBlocker blocker(table_);
    const int selectedColumn=std::max(0,table_->currentColumn());
    const auto& p = document_.project();
    if (p && (treeDocument_ != p->documentId || taskViewReset_)) {
        taskViewReset_=false;selectedTaskIds_.clear();
        const QSignalBlocker searchBlock(taskSearch_),filterBlock(taskFilter_),sortBlock(taskSort_);taskSearch_->clear();taskFilter_->setCurrentIndex(0);taskSort_->setCurrentIndex(0);
        selectedWbs_.clear(); selectedTask_.clear(); treeDocument_ = p->documentId;
        selectedBaseline_ = p->baselines.empty() ? std::string{} : p->baselines.back().id;
    }
    if (rebuildTree) rebuildWbsTree();
    setWindowTitle((p ? qs(p->name) : "프로젝트를 만들거나 여세요") + "[*] — JOBS");
    setWindowModified(document_.dirty());
    save_->setEnabled(p.has_value()); saveAs_->setEnabled(p.has_value());
    findChild<QAction*>("exportProjectXml")->setEnabled(p.has_value());
    add_->setEnabled(p.has_value() && !document_.readOnly());
    calculate_->setEnabled(p.has_value() && !calculating_);
    cancelCalculation_->setEnabled(calculating_&&calculationControl_&&!calculationControl_->cancelRequested.load());
    calendar_->setEnabled(p.has_value() && !document_.readOnly());
    calendars_->setEnabled(p.has_value()); taskCalendar_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty());
    constraints_->setEnabled(p.has_value()&&!selectedTask_.empty());scheduleIssues_->setEnabled(p.has_value());
    effort_->setEnabled(p.has_value()&&!selectedTask_.empty());
    addWbs_->setEnabled(p.has_value() && !document_.readOnly());
    const bool nodeEditable = p.has_value() && !document_.readOnly() && !selectedWbs_.empty();
    renameWbs_->setEnabled(nodeEditable); moveWbs_->setEnabled(nodeEditable); removeWbs_->setEnabled(nodeEditable);
    visibleRows_ = p ? wbsTaskRows(*p, selectedWbs_) : std::vector<std::size_t>{};
    applyTaskView();
    table_->setRowCount(int(visibleRows_.size())); table_->clearSelection(); table_->setCurrentCell(-1, -1);
    refreshTracking();
    resources_->setEnabled(p.has_value());
    assignResources_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty());
    resourceComparison_->setEnabled(p.has_value());
    rates_->setEnabled(p.has_value()); evm_->setEnabled(p.has_value()); reports_->setEnabled(p.has_value());
    taskCost_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty());
    costReport_->setEnabled(p.has_value() && !document_.readOnly() && !selectedTask_.empty() && p->statusAt.has_value());
    if (!p) { summary_->setText("새로 만들기(Ctrl+N) 또는 열기(Ctrl+O)로 시작하세요."); diagnostics_->clear(); wbsSummary_->clear();
        gantt_->setPlan(nullptr, nullptr, {}); assignWbs_->setEnabled(false); statusBar()->showMessage("준비"); return; }
    summary_->setText(qs(p->name) + "  |  시작일 " + qs(p->startDate) + "  |  revision " + QString::number(p->revision)
        + "  |  휴일 " + QString::number(p->holidays.size()) + "개"
        + "  |  상태일 " + (p->statusAt ? qs(*p->statusAt) : "미설정 · 현재 계획 모드")
        + "  |  기준선 " + QString::number(p->baselines.size()) + "개 · 실적 이력 " + QString::number(p->progress.size()) + "건"
        + "\n" + (document_.path().empty() ? "아직 저장하지 않은 프로젝트" : qs(document_.path()))
        + (document_.recoveryOrigin() ? " · 복구 사본: 새 파일에 저장 필요" : document_.requiresSaveAs() ? " · 이전 형식: 저장 시 새 파일 필요" : "")
        + (document_.recoveryOrigin() ? "\n복구 시각 UTC " + qs(document_.recoveryOrigin()->capturedAt) + " · 원본 r" + QString::number(document_.recoveryOrigin()->revision) + " · " + (document_.recoveryOrigin()->path.empty() ? "처음 저장 전" : qs(document_.recoveryOrigin()->path)) : "")
        + (document_.historyNotice().empty() ? QString{} : "\n편집 이력: " + qs(document_.historyNotice())));
    const auto& calculation = document_.calculation();
    const bool valid = calculation && calculation->status == CalculationStatus::Valid;
    std::unordered_map<std::string, const TaskSchedule*> schedules;
    if (valid) for (const auto& row : calculation->tasks) schedules.emplace(row.taskId, &row);
    std::unordered_map<std::string, int> numbers;
    for (int row = 0; row < int(p->tasks.size()); ++row) numbers.emplace(p->tasks[std::size_t(row)].id, row + 1);
    std::unordered_map<std::string,QStringList> assignedLabels;std::unordered_map<std::string,std::string> resourceNames;
    for(const auto& r:p->resources)resourceNames[r.id]=r.name;
    for(const auto& a:p->assignments)assignedLabels[a.taskId]<<qs(resourceNames.at(a.resourceId))+" "+QString::number(a.units)+"%";
    for (int row = 0; row < int(visibleRows_.size()); ++row) {
        const auto taskIndex = visibleRows_[std::size_t(row)]; const auto& t = p->tasks[taskIndex];
        table_->setVerticalHeaderItem(row, new QTableWidgetItem(QString::number(taskIndex + 1)));
        QStringList predecessors;
        for (const auto& d : dependencies(t)) predecessors << (numbers.contains(d.predecessorId) ? QString::number(numbers.at(d.predecessorId)) : "누락:" + qs(d.predecessorId)) + (d.type=="FS" && !d.lagMinutes?QString{}:qs(d.type)+(d.lagMinutes>=0?"+":"")+QString::number(d.lagMinutes)+"m");
        auto found = schedules.find(t.id); const auto* s = found == schedules.end() ? nullptr : found->second;
        auto duration = !t.durationMinutes ? QString{} : *t.durationMinutes % 3 == 0 ?
            QString::number(double(*t.durationMinutes) / 60, 'g', 12) : QString::number(*t.durationMinutes) + "m";
        if(usesWork(t)&&s&&!calculation->isForecast){WorkCalendar cal(*p,t);duration=QString::number(cal.offset(s->finish)-cal.offset(s->start))+"m";}
        QStringList values{qs(t.name), duration,
            predecessors.join(","), s ? qs(s->start) : "미계산", s ? qs(s->finish) : "미계산",
            s && !calculation->isForecast ? QString::number(double(s->totalFloatMinutes) / 60, 'g', 12) : "—",
            s ? (calculation->isForecast ? (s->completedActual ? "실제 완료" : "예측") : s->critical ? "임계" : "여유") : "—", "미설정", qs(t.id), qs(taskCalendarName(*p,t)),qs(t.constraintType)+(t.constraintAt?" "+qs(*t.constraintAt):QString{}),t.deadlineAt?qs(*t.deadlineAt):"없음"};
        values<<qs(taskTypeName(t.taskType))+(t.workUnitMinutes?" · "+QString::number(double(*t.workUnitMinutes)/6000.0,'f',4):" · 기간 기준")<<QString::number(t.breaks.size());
        const auto& assigned=assignedLabels[t.id];
        if (!assigned.isEmpty()) values[7] = assigned.join(", ");
        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values[column]);
            if (column > 2 || document_.readOnly()) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setToolTip("작업 ID: " + qs(t.id));
            if(column==10)item->setToolTip(qs(constraintName(t.constraintType))+" · "+qs(t.constraintType)+(t.constraintAt?" "+qs(*t.constraintAt):QString{}));
            if(column==11&&t.deadlineAt&&s&&s->finish>*t.deadlineAt){item->setBackground(QColor("#fff1cc"));item->setForeground(QColor("#653b00"));item->setToolTip("마감 초과 · 마감일은 경고 기준이며 날짜를 강제로 이동하지 않습니다.");}
            if(column==0&&calculation&&calculation->status==CalculationStatus::Conflict)for(const auto& issue:calculation->diagnostics)if(std::find(issue.taskIds.begin(),issue.taskIds.end(),t.id)!=issue.taskIds.end()){item->setBackground(QColor("#ffddd7"));item->setForeground(QColor("#662015"));item->setToolTip("<qt>"+qs(issue.message).toHtmlEscaped()+"</qt>");break;}
            table_->setItem(row, column, item);
        }
    }
    restoreTaskSelection(selectedColumn);
    assignWbs_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    taskCost_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    costReport_->setEnabled(!selectedTask_.empty() && !document_.readOnly() && p->statusAt.has_value());
    refreshTracking();
    assignResources_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
    gantt_->setPlan(&*p, calculation ? &*calculation : nullptr, visibleRows_,!document_.readOnly()&&!calculating_); gantt_->setSelectedTask(selectedTask_);
    const Baseline* baseline = nullptr;
    for (const auto& b : p->baselines) if (b.id == selectedBaseline_) baseline = &b;
    gantt_->setBaseline(baseline);
    const auto rollup = summarizeWbs(*p, calculation ? &*calculation : nullptr, selectedWbs_);
    wbsSummary_->setText("선택 범위: " + wbsTree_->currentItem()->text(0) + " · 작업 " + QString::number(rollup.taskCount)
        + "개 · 알려진 작업 기간 합계 " + QString::number(double(rollup.knownDurationMinutes) / 60, 'g', 10) + "h"
        + " · 기간 미정 " + QString::number(rollup.missingDurations) + "개 (합계는 WBS 경과기간과 다름)"
        + (rollup.start ? "\n계산 구간: " + qs(*rollup.start) + " → " + qs(*rollup.finish) : "\n계산 구간: 최신 유효한 결과 없음"));
    QString state = "미계산 · F9로 계산";
    if (calculation) {
        switch (calculation->status) {
        case CalculationStatus::Valid: state = (calculation->isForecast ? "실적 반영 예측 완료 · 프로젝트 완료 " : "계산 완료 · 프로젝트 완료 ") + qs(*calculation->projectFinish); break;
        case CalculationStatus::Incomplete: state = "입력 불완전 · 전체 완료일 없음"; break;
        case CalculationStatus::Conflict: state = "계산 충돌 · 전체 완료일 없음"; break;
        case CalculationStatus::Stale: state = "오래된 계산 · F9로 다시 계산"; break;
        case CalculationStatus::Cancelled: state = "계산 취소 · 부분 결과 없음 · F9로 다시 계산"; break;
        }
        state += "\n입력 revision " + QString::number(calculation->inputRevision) + " · " + qs(calculation->engineVersion) + " · " + qs(calculation->profile);
        state += " · " + qs(calculation->calculationPolicy);
        if (calculation->status != CalculationStatus::Stale) {
            int count = 0;
            for (const auto& d : calculation->diagnostics) {
                if (++count > 20) { state += "\n추가 진단 " + QString::number(calculation->diagnostics.size() - 20) + "개"; break; }
                state += "\n[" + qs(d.code) + "] " + qs(d.message);
                if (!d.taskIds.empty()) {
                    QStringList path;
                    for (const auto& id : d.taskIds) path << (numbers.contains(id) ? QString::number(numbers.at(id)) : qs(id));
                    state += " (작업 경로: " + path.join(" → ") + ")";
                }
            }
        }
    }
    diagnostics_->setPlainText(calculating_ ? "계산 중 · 편집할 수 있습니다. 변경 시 계산 결과는 오래됨으로 처리됩니다." : state);
    statusBar()->showMessage(QString(document_.readOnly() ? "읽기 전용 · " : document_.dirty() ? "미저장 변경 있음 · " : "명시 저장본과 같은 입력 · ")
        + (valid ? (calculation->isForecast ? "실적·잔여 예측 · 자원 미검증" : "선행관계·작업 달력 일정 · 자원 미검증") : "유효한 최신 일정 없음"));
}
void MainWindow::calculateSchedule() {
    commitPendingCell();
    if (!document_.project()) return;
    if(calculating_){queuedCalculation_=document_.sessionGeneration();return;}
    const auto snapshot = *document_.project(); calculating_ = true;calculationControl_=std::make_shared<CalculationControl>();const auto control=calculationControl_;calculationSession_=document_.sessionGeneration();calculationProgressLabel_->setText("일정 계산 준비 중…");calculationProgressTimer_->start();refresh();
    auto* watcher = new QFutureWatcher<ScheduleResult>(this);
    connect(watcher, &QFutureWatcher<ScheduleResult>::finished, this, [this, watcher, control, session = document_.sessionGeneration()] {
        calculating_ = false;calculationProgressTimer_->stop();calculationProgressLabel_->clear();calculationControl_.reset();
        try { if (document_.sessionGeneration() == session) {auto result=watcher->result();if(control->cancelRequested.load()){result.status=CalculationStatus::Cancelled;result.tasks.clear();result.projectFinish.reset();result.diagnostics={{"CALCULATION_CANCELLED","사용자가 일정 계산을 취소했습니다. 부분 결과는 적용하지 않았습니다.",{}}};}document_.applyCalculation(std::move(result));} } catch (const std::exception& e) { report(e); }
        watcher->deleteLater(); refresh();
        const auto queued=queuedCalculation_;queuedCalculation_.reset();
        if(queued&&*queued==document_.sessionGeneration())calculateSchedule();
    });
    watcher->setFuture(QtConcurrent::run([snapshot,control] { return jobs::calculate(snapshot,control); }));
}
void MainWindow::rebuildWbsTree() {
    const QSignalBlocker blocker(wbsTree_);
    QSet<QString> expanded;
    for (QTreeWidgetItemIterator it(wbsTree_); *it; ++it) if ((*it)->isExpanded()) expanded.insert((*it)->data(0, Qt::UserRole).toString());
    wbsTree_->clear();
    if (!document_.project()) return;
    const auto& p = *document_.project();
    auto* root = new QTreeWidgetItem(wbsTree_, {qs(p.name) + " · 전체 작업"}); root->setData(0, Qt::UserRole, QString{});
    root->setToolTip(0, "프로젝트 전체 작업 · 하위 WBS와 미분류 작업 포함"); root->setExpanded(true);
    std::unordered_map<std::string, QTreeWidgetItem*> items{{"", root}};
    for (const auto& node : p.wbsNodes) {
        auto* item = new QTreeWidgetItem(QStringList{qs(node.name)}); item->setData(0, Qt::UserRole, qs(node.id));
        item->setToolTip(0, "WBS ID: " + qs(node.id)); items.emplace(node.id, item);
    }
    for (const auto& node : p.wbsNodes) {
        items.at(node.parentId)->addChild(items.at(node.id)); items.at(node.id)->setExpanded(expanded.contains(qs(node.id)));
    }
    if (!items.contains(selectedWbs_)) selectedWbs_.clear();
    auto* selected = items.at(selectedWbs_); wbsTree_->setCurrentItem(selected);
    for (auto* ancestor = selected->parent(); ancestor; ancestor = ancestor->parent()) ancestor->setExpanded(true);
}
void MainWindow::createWbs() {
    bool ok = false;
    auto name = QInputDialog::getText(this, "WBS 추가", "선택한 WBS 아래에 만들 이름", QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok) return;
    try { selectedWbs_ = document_.addWbs(str(name), selectedWbs_); refresh(); } catch (const std::exception& e) { report(e); }
}
void MainWindow::renameWbs() {
    if (selectedWbs_.empty()) return;
    bool ok = false;
    auto name = QInputDialog::getText(this, "WBS 이름 변경", "이름", QLineEdit::Normal, wbsTree_->currentItem()->text(0), &ok).trimmed();
    if (!ok) return;
    try { document_.renameWbs(selectedWbs_, str(name)); refresh(); } catch (const std::exception& e) { report(e); }
}
void MainWindow::chooseWbs(bool moveNode) {
    if (!document_.project() || document_.readOnly()) return;
    const auto nodeId = selectedWbs_, taskId = selectedTask_;
    if ((moveNode && nodeId.empty()) || (!moveNode && taskId.empty())) return;
    const auto& p = *document_.project();
    QDialog dialog(this); dialog.setWindowTitle(moveNode ? "WBS 이동" : "작업 WBS 변경"); dialog.resize(450, 180);
    auto* layout = new QFormLayout(&dialog); auto* choice = new QComboBox(&dialog);
    choice->setObjectName("wbsDestination"); choice->setAccessibleName("대상 WBS");
    choice->addItem("프로젝트 루트 / 미분류", QString{});
    std::unordered_map<std::string, const WbsNode*> nodes;
    for (const auto& node : p.wbsNodes) nodes.emplace(node.id, &node);
    const auto excluded = moveNode ? wbsSubtree(p, nodeId) : std::unordered_set<std::string>{};
    for (const auto& node : p.wbsNodes) {
        if (excluded.contains(node.id)) continue;
        QStringList path{qs(node.name)}; auto parent = node.parentId;
        while (!parent.empty()) { path.prepend(qs(nodes.at(parent)->name)); parent = nodes.at(parent)->parentId; }
        choice->addItem(path.join(" / "), qs(node.id));
    }
    if (moveNode) choice->setCurrentIndex(choice->findData(qs(nodes.at(nodeId)->parentId)));
    else for (const auto& task : p.tasks) if (task.id == taskId) choice->setCurrentIndex(choice->findData(qs(task.wbsId)));
    layout->addRow("대상 WBS", choice);
    layout->addRow(new QLabel("작업 ID와 실행 선행관계는 유지됩니다.", &dialog));
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept); connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    try {
        auto target = str(choice->currentData().toString());
        if (moveNode) document_.moveWbs(nodeId, target);
        else for (std::size_t i = 0; i < document_.project()->tasks.size(); ++i)
            if (document_.project()->tasks[i].id == taskId) { document_.assignWbs(i, target); break; }
        refresh();
    } catch (const std::exception& e) { report(e); }
}
void MainWindow::editHolidays() {
    commitPendingCell();
    if (!document_.project() || document_.readOnly()) return;
    QDialog dialog(this); dialog.setWindowTitle("휴일 편집"); dialog.resize(400, 350);
    auto* layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("하루 전체 휴일 · 한 줄에 YYYY-MM-DD 한 개\n주말은 기본적으로 쉽니다.", &dialog));
    auto* dates = new QPlainTextEdit(&dialog); dates->setAccessibleName("휴일 날짜 목록");
    QStringList lines; for (const auto& d : document_.project()->holidays) lines << qs(d);
    dates->setPlainText(lines.join('\n')); layout->addWidget(dates);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    std::vector<std::string> holidays;
    for (auto line : dates->toPlainText().split('\n')) if (!line.trimmed().isEmpty()) holidays.push_back(str(line.trimmed()));
    try { document_.setHolidays(std::move(holidays)); refresh(); } catch (const std::exception& e) { report(e); }
}
bool MainWindow::confirmChanges() {
    commitPendingCell();
    if (!document_.dirty()) return true;
    auto result = QMessageBox::warning(this, "미저장 변경", "현재 프로젝트의 변경 내용을 저장하시겠습니까?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (result == QMessageBox::Cancel) return false;
    return result != QMessageBox::Save || save(false);
}
bool MainWindow::save(bool as) {
    commitPendingCell();
    if (!document_.project()) return false;
    try {
        if (as || document_.path().empty() || document_.readOnly() || document_.requiresSaveAs()) {
            QFileDialog dialog(this, "다른 이름으로 저장 · 기존 파일은 덮어쓰지 않습니다");
            dialog.setAcceptMode(QFileDialog::AcceptSave); dialog.setNameFilter("JOBS 프로젝트 (*.jobs)");
            dialog.setDefaultSuffix("jobs"); dialog.selectFile(qs(document_.project()->name) + ".jobs");
            if (dialog.exec() != QDialog::Accepted) return false;
            document_.saveAs(str(dialog.selectedFiles().first()));
        } else document_.save();
        refresh(); statusBar()->showMessage("저장 완료"); return true;
    } catch (const std::exception& e) { report(e); return false; }
}
void MainWindow::commitPendingCell() {
    auto* focused = QApplication::focusWidget();
    if (focused && qobject_cast<QLineEdit*>(focused) && table_->isAncestorOf(focused)) {
        if (auto* input = QApplication::inputMethod()) input->commit();
    }
    static_cast<TaskTable*>(table_)->commitEditors();
}
void MainWindow::create() {
    QDialog dialog(this); dialog.setWindowTitle("새 프로젝트"); dialog.resize(480, 440);
    auto* layout = new QFormLayout(&dialog);
    auto* name = new QLineEdit(&dialog); name->setAccessibleName("프로젝트명");
    auto* date = new QDateEdit(QDate::currentDate(), &dialog); date->setDisplayFormat("yyyy-MM-dd"); date->setCalendarPopup(true);
    date->setAccessibleName("프로젝트 시작일");
    auto* tasks = new QPlainTextEdit(&dialog); tasks->setPlaceholderText("설계\n개발\n검증"); tasks->setAccessibleName("작업명 목록 · 한 줄에 하나");
    auto* error = new QLabel(&dialog); error->setWordWrap(true);
    layout->addRow("프로젝트명(&N)", name); layout->addRow("시작일(&D)", date);
    layout->addRow("기본 근무달력", new QLabel("월–금 · 09–12 / 13–18\n1일 8시간 · 휴일은 생성 후 편집", &dialog));
    layout->addRow("작업명 목록(&T)", tasks); layout->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().isEmpty()) { error->setText("프로젝트명을 입력하세요."); name->setFocus(); return; }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted || !confirmChanges()) return;
    std::vector<std::string> names;
    for (auto line : tasks->toPlainText().split('\n')) if (!line.trimmed().isEmpty()) names.push_back(str(line.trimmed()));
    try { document_.create(str(name->text().trimmed()), str(date->date().toString(Qt::ISODate)), names); refresh(); }
    catch (const std::exception& e) { report(e); }
}
void MainWindow::open() {
    auto path = QFileDialog::getOpenFileName(this, "프로젝트 열기", {}, "JOBS 프로젝트 (*.jobs);;모든 파일 (*)");
    if (path.isEmpty() || !confirmChanges()) return;
    try { document_.open(str(path)); refresh(); }
    catch (const std::exception& e) {
        auto result = QMessageBox::warning(this, "열기 실패", QString::fromUtf8(e.what()) + "\n읽기 전용으로 열기를 시도하시겠습니까?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result == QMessageBox::Yes) {
            try { document_.open(str(path), true); refresh(); } catch (const std::exception& inner) { report(inner); }
        }
    }
}
void MainWindow::closeEvent(QCloseEvent* event) {
    if (baselinePreparing_) { QMessageBox::information(this, "기준선 저장 중", "기준선 계산을 마친 후 창을 닫으세요."); event->ignore(); return; }
    if (confirmChanges()) { if(calculationControl_){calculationControl_->cancel();queuedCalculation_.reset();}if (autosave_) autosave_->finishDocument(); event->accept(); } else event->ignore();
}
}
