#include "ui/MainWindow.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QVBoxLayout>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QTimer>
#include <algorithm>
namespace jobs {
void MainWindow::editTaskConstraints() {
    if(!document_.project()||selectedTask_.empty())return;
    const auto& p=*document_.project();const auto it=std::find_if(p.tasks.begin(),p.tasks.end(),[&](const auto& t){return t.id==selectedTask_;});if(it==p.tasks.end())return;
    const auto row=std::size_t(it-p.tasks.begin());const auto before=p.revision;
    QDialog dialog(this);dialog.setObjectName("taskConstraintsDialog");dialog.setWindowTitle("작업 제약·마감일 · "+QString::fromStdString(it->name));dialog.resize(650,330);
    auto* layout=new QVBoxLayout(&dialog);auto* note=new QLabel("선행관계·작업 달력·프로젝트 시작일과 함께 계산합니다. 고정 날짜는 근무 경계와 정확히 일치해야 합니다.\n마감일은 경고 기준이며 날짜를 강제로 옮기지 않습니다. ALAP은 현재 프로젝트 완료일 안에서 배치합니다.\n현지 civil 시각 YYYY-MM-DD HH:mm · 시간대 자동 변환 없음 · 빈 마감일=해제");note->setWordWrap(true);layout->addWidget(note);
    auto* form=new QFormLayout;layout->addLayout(form);auto* type=new QComboBox;type->setObjectName("constraintType");
    for(const auto& key:{"ASAP","ALAP","SNET","SNLT","FNET","FNLT","MSO","MFO"})type->addItem(QString::fromStdString(constraintName(key))+" ("+key+")",key);
    type->setCurrentIndex(type->findData(QString::fromStdString(it->constraintType)));form->addRow("제약 종류",type);
    auto* at=new QLineEdit(QString::fromStdString(it->constraintAt.value_or("")));at->setObjectName("constraintAt");at->setPlaceholderText("2026-10-26 09:00");form->addRow("제약 날짜·시각",at);
    auto* due=new QLineEdit(QString::fromStdString(it->deadlineAt.value_or("")));due->setObjectName("deadlineAt");due->setPlaceholderText("없음");form->addRow("마감일·시각",due);
    const bool editable=!document_.readOnly();auto enabled=[&]{const auto key=type->currentData().toString();at->setEnabled(editable&&key!="ASAP"&&key!="ALAP");};type->setEnabled(editable);due->setReadOnly(!editable);enabled();connect(type,&QComboBox::currentIndexChanged,&dialog,[&]{enabled();});
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close);auto* apply=buttons->addButton("적용 후 재계산",QDialogButtonBox::AcceptRole);apply->setObjectName("applyConstraints");apply->setEnabled(editable);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(apply,&QPushButton::clicked,&dialog,[&]{try{
        auto key=type->currentData().toString().toStdString();std::optional<std::string> stamp,deadline;
        if(key!="ASAP"&&key!="ALAP")stamp=at->text().trimmed().toStdString();
        if(!due->text().trimmed().isEmpty())deadline=due->text().trimmed().toStdString();
        document_.setTaskTiming(row,key,stamp,deadline);dialog.accept();
    }catch(const std::exception& e){report(e);}});
    dialog.exec();if(document_.project()&&document_.project()->revision!=before){refresh();calculateSchedule();}
}
void MainWindow::showScheduleIssues() {
    if(!document_.project())return;
    const auto snapshot=*document_.project();
    QDialog dialog(this);dialog.setObjectName("scheduleIssuesDialog");dialog.setWindowTitle("일정 충돌 진단");dialog.resize(1100,620);auto* layout=new QVBoxLayout(&dialog);
    auto* message=new QLabel("스냅샷 계산 중…");message->setObjectName("scheduleIssuesStatus");message->setTextFormat(Qt::PlainText);message->setWordWrap(true);layout->addWidget(message);
    auto* table=new QTableWidget(0,4);table->setObjectName("scheduleIssuesTable");table->setHorizontalHeaderLabels({"분류","코드","원인·관련 날짜·해결 단서","관련 작업"});table->setEditTriggers(QAbstractItemView::NoEditTriggers);table->setSelectionBehavior(QAbstractItemView::SelectRows);table->horizontalHeader()->setSectionResizeMode(2,QHeaderView::Stretch);table->setColumnWidth(0,90);table->setColumnWidth(1,220);table->setColumnWidth(3,210);layout->addWidget(table,1);
    auto* task=new QComboBox;task->setObjectName("diagnosticTask");layout->addWidget(task);auto* buttons=new QDialogButtonBox(QDialogButtonBox::Close);auto* select=buttons->addButton("관련 작업 선택",QDialogButtonBox::AcceptRole);select->setObjectName("selectDiagnosticTask");select->setEnabled(false);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);connect(select,&QPushButton::clicked,&dialog,&QDialog::accept);
    connect(table,&QTableWidget::currentCellChanged,&dialog,[&,task,select](int row,int,int,int){task->clear();if(row<0)return;const auto ids=table->item(row,3)->data(Qt::UserRole).toStringList();for(const auto& id:ids){auto found=std::find_if(snapshot.tasks.begin(),snapshot.tasks.end(),[&](const auto& t){return QString::fromStdString(t.id)==id;});if(found!=snapshot.tasks.end())task->addItem(QString::number(found-snapshot.tasks.begin()+1)+". "+QString::fromStdString(found->name),id);}select->setEnabled(task->count()>0);});
    auto* watcher=new QFutureWatcher<ScheduleResult>(&dialog);connect(watcher,&QFutureWatcher<ScheduleResult>::finished,&dialog,[&,watcher]{auto result=watcher->result();watcher->deleteLater();
        message->setText("스냅샷 r"+QString::number(snapshot.revision)+" · "+(result.status==CalculationStatus::Valid?"유효한 일정":"충돌/입력 불완전 · 완료일 미확정")+(result.projectFinish?" · 완료 "+QString::fromStdString(*result.projectFinish):QString{})+"\n경고 마감일과 하드 제약 충돌을 구분합니다. 관련 작업을 선택해 제약·선행·달력·기간을 수정하세요.");
        table->setRowCount(int(result.diagnostics.size()));for(int r=0;r<table->rowCount();++r){const auto& d=result.diagnostics[std::size_t(r)];QStringList ids,names;for(const auto& id:d.taskIds){ids<<QString::fromStdString(id);for(std::size_t i=0;i<snapshot.tasks.size();++i)if(snapshot.tasks[i].id==id)names<<QString::number(i+1)+". "+QString::fromStdString(snapshot.tasks[i].name);}const bool warning=d.code=="DEADLINE_MISSED"||d.code=="OUT_OF_SEQUENCE";const QStringList cells{warning?"경고":result.status==CalculationStatus::Valid?"정보":d.code.find("POLICY")!=std::string::npos||d.code=="UNREPORTED_REMAINING"?"정보":"충돌/입력",QString::fromStdString(d.code),QString::fromStdString(d.message),names.join(", ")};for(int c=0;c<4;++c){auto* item=new QTableWidgetItem(cells[c]);item->setToolTip("<qt>"+cells[c].toHtmlEscaped()+"</qt>");if(c==3)item->setData(Qt::UserRole,ids);table->setItem(r,c,item);}}table->resizeRowsToContents();if(table->rowCount())table->setCurrentCell(0,0);
    });watcher->setFuture(QtConcurrent::run([snapshot]{return calculate(snapshot);}));
    const auto accepted=dialog.exec()==QDialog::Accepted;const auto id=task->currentData().toString().toStdString();if(accepted&&!id.empty()&&document_.project()&&document_.project()->documentId==snapshot.documentId&&document_.project()->revision==snapshot.revision){selectedWbs_.clear();selectedTaskIds_={id};selectedTask_=id;refresh();}
}
}
