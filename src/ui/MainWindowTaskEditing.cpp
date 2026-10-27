#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "ui/TaskInput.h"
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
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
namespace jobs {
namespace {
QString qs(const std::string& s){return QString::fromUtf8(s.data(),qsizetype(s.size()));}
}
void MainWindow::setupTaskEditing(QMenu* menu,QVBoxLayout* layout) {
    table_->setObjectName("taskTable");table_->setSelectionMode(QAbstractItemView::ExtendedSelection);table_->setSelectionBehavior(QAbstractItemView::SelectItems);
    menu->addSeparator();
    auto action=[&](const QString& name,const char* id,const QKeySequence& key,auto callback){auto* a=menu->addAction(name);a->setObjectName(id);a->setShortcut(key);a->setShortcutContext(Qt::WidgetShortcut);table_->addAction(a);connect(a,&QAction::triggered,this,[this,callback]{runAfterEdit([this, callback] { (this->*callback)(); });});return a;};
    insertTasks_=action("작업 삽입…","insertTasks",QKeySequence(Qt::Key_Insert),&MainWindow::insertSelectedTasks);
    deleteTasks_=action("선택 작업 삭제…","deleteTasks",QKeySequence(Qt::Key_Delete),&MainWindow::deleteSelectedTasks);
    duplicateTasks_=action("선택 작업 복제","duplicateTasks",QKeySequence("Ctrl+D"),&MainWindow::duplicateSelectedTasks);
    copyTasks_=action("표 선택 복사","copyTaskCells",QKeySequence::Copy,&MainWindow::copySelectedCells);
    pasteTasks_=action("표 붙여넣기","pasteTaskCells",QKeySequence::Paste,&MainWindow::pasteCells);
    auto* editCell=menu->addAction("현재 셀 편집");editCell->setObjectName("editTaskCell");editCell->setShortcut(QKeySequence(Qt::Key_F2));editCell->setShortcutContext(Qt::WidgetShortcut);table_->addAction(editCell);
    connect(editCell,&QAction::triggered,this,[this]{commitPendingCell();const auto index=table_->currentIndex();if(index.isValid()&&(index.flags()&Qt::ItemIsEditable))table_->edit(index);});
    auto* all=menu->addAction("표 전체 선택");all->setShortcut(QKeySequence::SelectAll);all->setShortcutContext(Qt::WidgetShortcut);table_->addAction(all);connect(all,&QAction::triggered,table_,&QTableView::selectAll);
    auto* strip=new QHBoxLayout;taskSearch_=new QLineEdit;taskSearch_->setObjectName("taskSearch");taskSearch_->setAccessibleName("작업 이름·ID 검색");taskSearch_->setPlaceholderText("작업 이름·ID 검색 ("+QKeySequence(QKeySequence::Find).toString(QKeySequence::NativeText)+")");taskSearch_->setClearButtonEnabled(true);taskSearch_->setMinimumWidth(260);taskSearch_->setMaximumWidth(280);taskSearch_->setToolTip("검색은 표·간트 보기만 바꿉니다. 일정은 전체 프로젝트를 계산하고 보고서는 선택 WBS 범위를 사용합니다.");strip->addWidget(taskSearch_);
    taskFilter_=new QComboBox;taskFilter_->setObjectName("taskFilter");taskFilter_->setAccessibleName("작업 필터");taskFilter_->addItems({"전체 작업","계획 임계","마일스톤","미배정"});strip->addWidget(taskFilter_);
    taskSort_=new QComboBox;taskSort_->setObjectName("taskSort");taskSort_->setAccessibleName("작업 정렬");taskSort_->addItems({"프로젝트 원본 순서","이름 ↑","이름 ↓","기간 ↑","기간 ↓","시작일 ↑","시작일 ↓"});strip->addWidget(taskSort_);
    auto* reset=new QPushButton("보기 초기화");reset->setObjectName("resetTaskView");strip->addWidget(reset);taskViewStatus_=new QLabel;taskViewStatus_->setObjectName("taskViewStatus");taskViewStatus_->setTextFormat(Qt::PlainText);taskViewStatus_->setWordWrap(true);strip->addWidget(taskViewStatus_,1);layout->insertLayout(3,strip);
    auto* find=menu->addAction("작업 검색");find->setShortcut(QKeySequence::Find);connect(find,&QAction::triggered,this,[this]{commitPendingCell();taskSearch_->setFocus();taskSearch_->selectAll();});
    auto* debounce=new QTimer(this);debounce->setSingleShot(true);debounce->setInterval(180);connect(taskSearch_,&QLineEdit::textChanged,debounce,qOverload<>(&QTimer::start));connect(debounce,&QTimer::timeout,this,[this]{commitPendingCell();refresh(false);});
    for(auto* combo:{taskFilter_,taskSort_})connect(combo,&QComboBox::currentIndexChanged,this,[this]{commitPendingCell();refresh(false);});
    connect(reset,&QPushButton::clicked,this,[this]{commitPendingCell();const QSignalBlocker a(taskSearch_),b(taskFilter_),c(taskSort_);taskSearch_->clear();taskFilter_->setCurrentIndex(0);taskSort_->setCurrentIndex(0);refresh(false);});
    table_->horizontalHeader()->setSectionsClickable(true);connect(table_->horizontalHeader(),&QHeaderView::sectionClicked,this,[this](int column){int first=column==0?1:column==1?3:column==3?5:0;if(first)taskSort_->setCurrentIndex(taskSort_->currentIndex()==first?first+1:first);});
    connect(QApplication::clipboard(),&QClipboard::dataChanged,this,&MainWindow::updateTaskActions);
    auto* help=menu->addAction("입력·단축키 도움말");help->setObjectName("taskEditingHelp");help->setShortcut(QKeySequence(Qt::Key_F1));
    connect(help,&QAction::triggered,this,[this]{QMessageBox::information(this,"입력·단축키 도움말",editingHelp_->text());});
}
void MainWindow::resizeEvent(QResizeEvent* event){QMainWindow::resizeEvent(event);updateCompactLayout();}
void MainWindow::updateCompactLayout(){if(!editingHelp_||!diagnostics_)return;const bool compact=height()<760;editingHelp_->setVisible(!compact);diagnostics_->setMaximumHeight(compact?80:160);}
void MainWindow::syncTaskSelection() {

        selectedTaskIds_=selectedTaskIds();
        const auto row = table_->currentRow();
        if (document_.project() && row >= 0 && std::size_t(row) < visibleRows_.size() && table_->selectionModel()->hasSelection())
            selectedTask_ = document_.project()->tasks[visibleRows_[std::size_t(row)]].id;
        else selectedTask_.clear();
        if(!selectedTaskIds_.empty()&&std::find(selectedTaskIds_.begin(),selectedTaskIds_.end(),selectedTask_)==selectedTaskIds_.end())selectedTask_=selectedTaskIds_.front();
        updateTaskActions();
        assignResources_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
        taskCalendar_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
        constraints_->setEnabled(!selectedTask_.empty());
        effort_->setEnabled(!selectedTask_.empty());
        taskCost_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
        costReport_->setEnabled(!selectedTask_.empty() && !document_.readOnly() && document_.project()->statusAt.has_value());
        gantt_->setSelectedTask(selectedTask_); assignWbs_->setEnabled(!selectedTask_.empty() && !document_.readOnly());
        progressEdit_->setEnabled(!selectedTask_.empty() && !document_.readOnly() && document_.project()->statusAt.has_value());
}
std::vector<std::string> MainWindow::selectedTaskIds() const {
    QSet<int> rows;for(const auto& index:table_->selectionModel()->selectedIndexes())rows.insert(index.row());auto ordered=rows.values();std::sort(ordered.begin(),ordered.end());std::vector<std::string> ids;
    for(int row:ordered)ids.push_back(table_->model()->index(row,8).data().toString().toUtf8().toStdString());
    return ids;
}
void MainWindow::updateTaskActions() {
    if(!insertTasks_)return;
    const bool writable=document_.project()&&!document_.readOnly();const bool selection=!selectedTaskIds().empty();
    if(auto* edit=findChild<QAction*>("editTaskCell"))edit->setEnabled(writable&&table_->currentIndex().isValid()&&(table_->currentIndex().flags()&Qt::ItemIsEditable));
    insertTasks_->setEnabled(writable);deleteTasks_->setEnabled(writable&&selection);duplicateTasks_->setEnabled(writable&&selection);copyTasks_->setEnabled(selection);
    pasteTasks_->setEnabled(writable&&table_->currentRow()>=0&&table_->currentColumn()>=0&&table_->currentColumn()<3&&(QApplication::clipboard()->mimeData()&&QApplication::clipboard()->mimeData()->hasText()));
}
void MainWindow::restoreTaskSelection(int column) {
    if(selectedTaskIds_.empty()&&!selectedTask_.empty())selectedTaskIds_.push_back(selectedTask_);
    const std::unordered_set<std::string> requested(selectedTaskIds_.begin(),selectedTaskIds_.end());std::vector<std::string> kept;int active=-1,first=-1;
    for(int row=0;row<int(visibleRows_.size());++row){const auto& id=document_.project()->tasks[visibleRows_[std::size_t(row)]].id;if(!requested.contains(id))continue;kept.push_back(id);if(first<0)first=row;if(id==selectedTask_)active=row;table_->selectionModel()->select(table_->model()->index(row,0),QItemSelectionModel::Select|QItemSelectionModel::Rows);}
    selectedTaskIds_=std::move(kept);if(active<0)active=first;
    if(active>=0){selectedTask_=document_.project()->tasks[visibleRows_[std::size_t(active)]].id;table_->setCurrentCell(active,std::clamp(column,0,13),QItemSelectionModel::NoUpdate);}else selectedTask_.clear();updateTaskActions();
}
void MainWindow::applyTaskView() {
    if(!document_.project()){taskViewStatus_->clear();return;}const auto& p=*document_.project();const auto total=visibleRows_.size();const auto query=taskSearch_->text().trimmed();const auto filter=taskFilter_->currentIndex(),sort=taskSort_->currentIndex();
    const auto& result=document_.calculation();const bool valid=result&&result->status==CalculationStatus::Valid;const bool plan=valid&&!result->isForecast;
    std::unordered_map<std::string,const TaskSchedule*> dates;std::unordered_set<std::string> assigned;
    if(filter||sort>=3){if(valid)for(const auto& r:result->tasks)dates[r.taskId]=&r;if(filter==3)for(const auto& a:p.assignments)assigned.insert(a.taskId);}
    std::erase_if(visibleRows_,[&](std::size_t i){const auto& t=p.tasks[i];if(!query.isEmpty()&&!qs(t.name).contains(query,Qt::CaseInsensitive)&&!qs(t.id).contains(query,Qt::CaseInsensitive))return true;
        if(filter==1)return !plan||!dates.contains(t.id)||!dates.at(t.id)->critical;
        if(filter==2)return !(usesWork(t)?t.workUnitMinutes==0:t.durationMinutes==0)&&!(dates.contains(t.id)&&dates.at(t.id)->start==dates.at(t.id)->finish);
        return filter==3&&assigned.contains(t.id);});
    if(sort){struct Key{QString name;std::optional<std::int64_t> duration;std::optional<std::string> start;};std::vector<Key> keys(p.tasks.size());
        for(auto i:visibleRows_){const auto& t=p.tasks[i];auto& k=keys[i];k.name=qs(t.name);k.duration=t.durationMinutes;if(dates.contains(t.id)){const auto& r=*dates.at(t.id);k.start=r.start;if(usesWork(t)&&plan){WorkCalendar cal(p,t);k.duration=cal.offset(r.finish)-cal.offset(r.start);}}}
        std::stable_sort(visibleRows_.begin(),visibleRows_.end(),[&](std::size_t a,std::size_t b){const auto& x=keys[a];const auto& y=keys[b];if(sort<=2){const auto comparison=QString::compare(x.name,y.name,Qt::CaseInsensitive);return sort==1?comparison<0:comparison>0;}if(sort<=4){if(bool(x.duration)!=bool(y.duration))return bool(x.duration);if(!x.duration)return false;return sort==3?*x.duration<*y.duration:*x.duration>*y.duration;}if(bool(x.start)!=bool(y.start))return bool(x.start);if(!x.start)return false;return sort==5?*x.start<*y.start:*x.start>*y.start;});}
    table_->horizontalHeader()->setSortIndicatorShown(sort!=0);if(sort)table_->horizontalHeader()->setSortIndicator(sort<=2?0:sort<=4?1:3,sort%2?Qt::AscendingOrder:Qt::DescendingOrder);
    taskViewStatus_->setText(QString("표시 %1 / WBS 범위 %2 · 선행 번호=프로젝트 원본 행").arg(visibleRows_.size()).arg(total)+(filter==1&&!plan?" · 최신 계획 계산 필요":QString{}));
}
void MainWindow::insertSelectedTasks() {
    if(!document_.project()||document_.readOnly())return;
    const auto session=document_.sessionGeneration(),revision=document_.project()->revision;const auto before=selectedTask_,wbs=selectedWbs_;
    QDialog dialog(this);dialog.setObjectName("insertTasksDialog");dialog.setWindowTitle("작업 삽입");dialog.resize(480,320);auto* layout=new QVBoxLayout(&dialog);layout->addWidget(new QLabel(before.empty()?"프로젝트 끝에 추가합니다. 작업 이름을 한 줄에 하나씩 입력하세요.":"현재 작업의 프로젝트 원본 순서 앞에 삽입합니다. 한 줄에 한 작업입니다."));auto* names=new QPlainTextEdit;names->setObjectName("insertTaskNames");names->setPlainText("새 작업");layout->addWidget(names);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);if(dialog.exec()!=QDialog::Accepted)return;
    try{if(document_.sessionGeneration()!=session||!document_.project()||document_.project()->revision!=revision)throw std::runtime_error("입력 중 프로젝트가 변경되었습니다. 다시 삽입하세요.");if(names->toPlainText().toUtf8().size()>4*1024*1024)throw std::runtime_error("작업 이름 목록은 최대 4 MiB입니다.");std::vector<std::string> values;for(const auto& line:names->toPlainText().split('\n'))if(!line.trimmed().isEmpty())values.push_back(line.trimmed().toUtf8().toStdString());selectedTaskIds_=document_.insertTasks(values,before,wbs);selectedTask_=selectedTaskIds_.front();refresh();}catch(const std::exception& e){report(e);}
}
void MainWindow::deleteSelectedTasks() {
    if(!document_.project()||document_.readOnly())return;
    try{const auto ids=selectedTaskIds();const auto info=document_.previewRemoveTasks(ids);if(!info.protectedTaskIds.empty())throw std::runtime_error("선택에 실적/원가 보고 이력이 있는 작업이 포함돼 삭제할 수 없습니다. 보고 이력을 보존합니다.");
        const auto revision=document_.project()->revision,session=document_.sessionGeneration();QMessageBox confirm(QMessageBox::Warning,"선택 작업 삭제",QString("작업 %1개, 선행관계 %2개, 자원 배정 %3개, 작업 예산 %4개를 삭제합니다.\n승인 기준선과 저장 시나리오는 보존됩니다. 실행 취소할 수 있습니다.").arg(info.tasks).arg(info.links).arg(info.assignments).arg(info.costs),QMessageBox::Yes|QMessageBox::No,this);confirm.setObjectName("deleteTasksConfirmation");confirm.setDefaultButton(QMessageBox::No);if(confirm.exec()!=QMessageBox::Yes)return;
        if(document_.sessionGeneration()!=session||!document_.project()||document_.project()->revision!=revision)throw std::runtime_error("확인 중 프로젝트가 변경되었습니다. 삭제 대상을 다시 확인하세요.");
        document_.removeTasks(ids);selectedTaskIds_.clear();selectedTask_.clear();refresh();
    }catch(const std::exception& e){report(e);}
}
void MainWindow::duplicateSelectedTasks() {
    try{selectedTaskIds_=document_.duplicateTasks(selectedTaskIds());selectedTask_=selectedTaskIds_.front();refresh();}catch(const std::exception& e){report(e);}
}
void MainWindow::copySelectedCells() {
    try{const auto indices=table_->selectionModel()->selectedIndexes();if(indices.isEmpty())return;int top=indices.front().row(),bottom=top,left=indices.front().column(),right=left;for(const auto& i:indices){top=std::min(top,i.row());bottom=std::max(bottom,i.row());left=std::min(left,i.column());right=std::max(right,i.column());}
        if(indices.size()!=qsizetype(bottom-top+1)*(right-left+1))throw std::runtime_error("복사할 셀은 구멍 없는 연속 직사각형으로 선택하세요.");
        if(left==0&&right==table_->columnCount()-1)right=2;
        if(bottom-top+1>10000||(bottom-top+1)*(right-left+1)>30000)throw std::runtime_error("복사는 최대 10,000행·30,000셀입니다.");
        QList<QStringList> rows;
        for(int r=top;r<=bottom;++r){QStringList row;for(int c=left;c<=right;++c)if(!table_->isColumnHidden(c))row<<table_->model()->index(r,c).data().toString();rows<<row;}const auto text=encodeTaskTsv(rows);if(text.toUtf8().size()>4*1024*1024)throw std::runtime_error("복사 텍스트는 최대 4 MiB입니다.");QApplication::clipboard()->setText(text);statusBar()->showMessage("선택 표를 복사했습니다.");
    }catch(const std::exception& e){report(e);}
}
void MainWindow::pasteCells() {
    if(!document_.project()||document_.readOnly())return;
    try{const auto cells=decodeTaskTsv(QApplication::clipboard()->text());const int row=table_->currentRow(),column=table_->currentColumn();if(cells.isEmpty()||row<0||column<0||column>=3||column+cells.front().size()>3)throw std::runtime_error("이름·기간·선행 입력 3열 범위에 붙여넣으세요.");if(std::size_t(row)+std::size_t(cells.size())>visibleRows_.size())throw std::runtime_error("보이는 작업 행이 부족합니다. 작업 삽입으로 행을 먼저 만드세요.");
        const auto& p=*document_.project();std::vector<TaskEdit> edits;
        for(qsizetype r=0;r<cells.size();++r){TaskEdit e;e.taskId=p.tasks[visibleRows_.at(std::size_t(row)+std::size_t(r))].id;
            for(qsizetype c=0;c<cells[r].size();++c){try{const auto& value=cells[r][c];if(column+c==0){if(value.trimmed().isEmpty())throw std::runtime_error("작업 이름은 빈칸일 수 없습니다.");e.name=value.trimmed().toUtf8().toStdString();}else if(column+c==1)e.duration.emplace(taskDurationInput(value));else e.edges=taskDependencyInput(value,p,e.taskId);}catch(const std::exception& error){throw std::runtime_error("붙여넣기 "+std::to_string(r+1)+"행 "+std::to_string(c+1)+"열: "+error.what());}}edits.push_back(std::move(e));}
        bool inverse = false; for (const auto& edit : edits) if (edit.duration && hasInverseDuration(edit.taskId)) inverse = true;
        if (inverse) { submitEdit(BatchTaskEdit{std::move(edits)}, "표 공수 역산"); return; }
        document_.editTasks(edits);refresh();statusBar()->showMessage(QString("%1행 붙여넣기 완료 · 한 번에 실행 취소 가능").arg(cells.size()));
    }catch(const std::exception& e){report(e);}
}
}
