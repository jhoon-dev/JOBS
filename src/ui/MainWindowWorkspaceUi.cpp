#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include <QAction>
#include <QComboBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QStyle>
#include <QTextBlock>
#include <QToolBar>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace jobs {
void MainWindow::setupWorkspaceUi(QToolBar *tools, QComboBox *zoom) {
    tools->setObjectName("primaryTools");
    tools->setIconSize({18, 18});
    tools->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    for (auto *action : tools->actions())
        tools->removeAction(action);
    const auto add = [this, tools](QAction *action, const QString &label, QStyle::StandardPixmap icon) {
        action->setIcon(style()->standardIcon(icon));
        action->setIconText(label);
        tools->addAction(action);
    };
    add(findChild<QAction *>("newProject"), "새로 만들기", QStyle::SP_FileIcon);
    add(findChild<QAction *>("openProject"), "열기", QStyle::SP_DialogOpenButton);
    add(save_, "저장", QStyle::SP_DialogSaveButton);
    tools->addSeparator();
    add(undo_, "실행 취소", QStyle::SP_ArrowBack);
    add(redo_, "다시 실행", QStyle::SP_ArrowForward);
    for (auto *action : {undo_, redo_})
        if (auto *button = qobject_cast<QToolButton *>(tools->widgetForAction(action)))
            button->setToolButtonStyle(Qt::ToolButtonIconOnly);
    tools->addSeparator();
    add(add_, "작업 추가", QStyle::SP_FileDialogNewFolder);
    auto *taskMenu = new QMenu(this);
    for (auto *action : {insertTasks_, duplicateTasks_, deleteTasks_, copyTasks_, pasteTasks_})
        taskMenu->addAction(action);
    taskMenu->addSeparator();
    for (auto *action :
         {constraints_, effort_, taskCalendar_, assignWbs_, assignResources_, taskCost_, progressEdit_, costReport_})
        taskMenu->addAction(action);
    auto *taskButton = new QToolButton(tools);
    taskButton->setObjectName("selectedTaskMenu");
    taskButton->setText("작업 편집");
    taskButton->setAccessibleName("선택 작업 편집 메뉴");
    taskButton->setPopupMode(QToolButton::InstantPopup);
    taskButton->setMenu(taskMenu);
    tools->addWidget(taskButton);
    tools->addSeparator();
    add(calculate_, "일정 계산 · F9", QStyle::SP_MediaPlay);
    for (auto *action : {cancelCalculation_, cancelEdit_}) {
        tools->addAction(action);
        action->setVisible(action->isEnabled());
        connect(action, &QAction::changed, this, [action] {
            if (action->isVisible() != action->isEnabled())
                action->setVisible(action->isEnabled());
        });
    }
    auto *analysis = new QToolButton(tools);
    analysis->setObjectName("analysisMenu");
    analysis->setText("분석·보고");
    analysis->setPopupMode(QToolButton::InstantPopup);
    auto *analysisMenu = new QMenu(analysis);
    for (auto *action : {reports_, comparison_, resourceComparison_, evm_, scheduleIssues_})
        analysisMenu->addAction(action);
    analysis->setMenu(analysisMenu);
    tools->addWidget(analysis);
    auto *space = new QWidget(tools);
    space->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    tools->addWidget(space);
    auto *help = findChild<QAction *>("taskEditingHelp");
    help->setIconText("도움말 · F1");
    tools->addAction(help);

    auto *layout = qobject_cast<QVBoxLayout *>(centralWidget()->layout());
    layout->removeWidget(wbsSummary_);
    wbsSummary_->setWordWrap(false);
    wbsSummary_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    auto *view = new QHBoxLayout;
    auto *wbs = new QToolButton(centralWidget());
    wbs->setObjectName("toggleWbs");
    wbs->setText("WBS 탐색");
    wbs->setCheckable(true);
    wbs->setChecked(true);
    wbs->setToolTip("WBS 탐색 표시/숨기기 · 선택한 범위는 유지됩니다.");
    connect(wbs, &QToolButton::toggled, wbsTree_, &QWidget::setVisible);
    view->addWidget(wbs);
    view->addWidget(wbsSummary_, 1);
    view->addWidget(new QLabel("간트 배율", centralWidget()));
    view->addWidget(zoom);
    baselineChoice_->setMinimumWidth(150);
    view->addWidget(baselineChoice_);
    layout->insertLayout(3, view);

    auto *diagnosticBar = new QHBoxLayout;
    auto *status = new QLabel(centralWidget());
    status->setObjectName("calculationStatus");
    status->setTextFormat(Qt::PlainText);
    status->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    status->setText("일정 계산 · 프로젝트를 열고 F9로 계산하세요");
    diagnosticsToggle_ = new QToolButton(centralWidget());
    diagnosticsToggle_->setObjectName("toggleDiagnostics");
    diagnosticsToggle_->setCheckable(true);
    diagnosticsToggle_->setText("진단 보기");
    diagnosticsToggle_->setArrowType(Qt::RightArrow);
    diagnosticsToggle_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(diagnosticsToggle_, &QToolButton::toggled, this, [this](bool open) {
        diagnostics_->setVisible(open);
        diagnosticsToggle_->setText(open ? "진단 접기" : "진단 보기");
        diagnosticsToggle_->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
    });
    connect(
        diagnostics_, &QPlainTextEdit::textChanged, status,
        [this, status, revealed = std::pair{~std::uint64_t{0}, ~std::uint64_t{0}}]() mutable {
            auto text = diagnostics_->document()->firstBlock().text();
            const auto &result = document_.calculation();
            if (result && result->status != CalculationStatus::Stale && !result->diagnostics.empty())
                text += QString(" · 진단 %1건").arg(result->diagnostics.size());
            status->setText(text.isEmpty() ? "일정 계산 · 프로젝트를 열고 F9로 계산하세요" : text);
            status->setToolTip(status->text());
            const auto key = std::pair{document_.sessionGeneration(), document_.calculationGeneration()};
            if (result && revealed != key &&
                (result->status == CalculationStatus::Conflict || result->status == CalculationStatus::Incomplete)) {
                revealed = key;
                diagnosticsToggle_->setChecked(true);
            }
        });
    diagnosticBar->addWidget(status, 1);
    diagnosticBar->addWidget(diagnosticsToggle_);
    layout->addLayout(diagnosticBar);
    layout->addWidget(diagnostics_);
    diagnostics_->hide();
    summary_->setWordWrap(false);
    summary_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    projectTitle_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    table_->setAlternatingRowColors(true);
    table_->setShowGrid(false);
    table_->verticalHeader()->setSectionResizeMode(QHeaderView::Fixed);
    wbsTree_->setAlternatingRowColors(true);
    wbsTree_->setUniformRowHeights(true);
    workspaceSplit_->setHandleWidth(7);
    workspaceSplit_->setStretchFactor(0, 0);
    workspaceSplit_->setStretchFactor(1, 1);
    workspaceSplit_->setStretchFactor(2, 1);
    setStyleSheet(
        "QToolBar { spacing: 4px; padding: 6px; border: 0; border-bottom: 1px solid palette(midlight); }"
        "QToolBar QToolButton { padding: 6px 8px; }"
        "QLabel#projectTitle { font-size: 21px; font-weight: 600; padding: 2px 0; }"
        "QLabel#documentNotice { padding: 6px; border: 1px solid palette(mid); }"
        "QLineEdit#taskSearch { padding: 6px 8px; border: 1px solid palette(mid); border-radius: 4px; }"
        "QComboBox { padding: 4px 6px; }"
        "QHeaderView::section { padding: 4px; border: 0; border-bottom: 1px solid palette(midlight); }"
        "QTableView#taskTable, QTreeWidget#wbsTree, QGraphicsView#ganttView { border: 1px solid palette(midlight); }"
        "QToolButton#toggleWbs, QToolButton#toggleDiagnostics { padding: 5px 8px; }"
        "QLineEdit#taskSearch:focus { border: 1px solid palette(highlight); }");
}
void MainWindow::refreshWorkspaceUi() {
    const auto &project = document_.project();
    if (!projectTitle_)
        return;
    if (!project) {
        projectTitle_->setText("프로젝트를 시작하세요");
        projectTitle_->setToolTip(QString("새로 만들기 %1 · 프로젝트 열기 %2")
                                      .arg(QKeySequence(QKeySequence::New).toString(QKeySequence::NativeText),
                                           QKeySequence(QKeySequence::Open).toString(QKeySequence::NativeText)));
        summary_->setToolTip({});
        documentNotice_->hide();
    }
}
} // namespace jobs
