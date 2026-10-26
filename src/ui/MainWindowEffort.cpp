#include "ui/MainWindow.h"
#include "calculation/Effort.h"
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <algorithm>
#include <cmath>
namespace jobs {
void MainWindow::editTaskEffort() {
    if(!document_.project()||selectedTask_.empty())return;
    const auto snapshot=*document_.project();const auto it=std::find_if(snapshot.tasks.begin(),snapshot.tasks.end(),[&](const auto& t){return t.id==selectedTask_;});if(it==snapshot.tasks.end())return;
    const auto index=std::size_t(it-snapshot.tasks.begin());const auto& t=*it;
    QDialog dialog(this);dialog.setObjectName("effortDialog");dialog.setWindowTitle("공수·작업 유형·분할 · "+QString::fromStdString(t.name));dialog.resize(760,640);
    auto* layout=new QVBoxLayout(&dialog);
    auto* note=new QLabel("100% = 자원 1단위 · 1공수시간 = 자원 1단위 × 1시간 (인력/장비 합산)\n고정 기간: 기간 유지, 공수 입력 시 배정률 조정. 고정 공수: 공수 유지, 기간 입력 시 배정률 조정.\n고정 단위: 배정률 유지, 기간 입력→공수 계산 / 공수 입력→완료일 계산.\n배정률은 정수%입니다. 기간·공수로 배정률 조정 시 1% 반올림으로 요청 값과 차이가 날 수 있습니다.");note->setWordWrap(true);layout->addWidget(note);
    auto* form=new QFormLayout;layout->addLayout(form);
    auto* type=new QComboBox;type->setObjectName("taskType");type->addItem("고정 기간","fixed-duration");type->addItem("고정 공수","fixed-work");type->addItem("고정 단위","fixed-units");type->setCurrentIndex(type->findData(QString::fromStdString(t.taskType)));form->addRow("작업 유형",type);
    auto* basis=new QComboBox;basis->setObjectName("effortBasis");basis->addItem("기간 입력","duration");basis->addItem("공수 입력","work");basis->setCurrentIndex(usesWork(t)?1:0);form->addRow("이번 변경의 입력 기준",basis);
    auto* duration=new QLineEdit(t.durationMinutes?QString::number(*t.durationMinutes):QString{});duration->setObjectName("effortDuration");duration->setPlaceholderText("근무분 정수 · 미정이면 빈칸");form->addRow("기간 (근무분)",duration);
    auto* work=new QLineEdit(t.workUnitMinutes?QString::number(double(*t.workUnitMinutes)/6000.0,'f',4):QString{});work->setObjectName("effortHours");work->setPlaceholderText("자원시간 · 최대 소수 4자리 (1/100 자원분 반올림)");form->addRow("공수 (자원시간)",work);
    const auto plan=calculatePlan(snapshot);QString measured="최신 계획 계산 불가: 입력/진단을 확인하세요.";
    if(plan.status==CalculationStatus::Valid)for(const auto& r:plan.tasks)if(r.taskId==t.id) {
        PlanInput input=snapshot;input.calculationProfile=effortProfile;TaskExecution model(input,t);
        measured="현재 계획: "+QString::fromStdString(r.start)+" → "+QString::fromStdString(r.finish)+" · 활성 기간 "+QString::number(model.duration(r))+"분 · 공수 "+QString::number(double(model.work(r))/6000.0,'f',4)+"h";
        if(!usesWork(t))work->setText(QString::number(double(model.work(r))/6000.0,'f',4));
        if(usesWork(t))duration->setText(QString::number(model.duration(r)));
    }
    auto* measuredLabel=new QLabel(measured);measuredLabel->setWordWrap(true);layout->addWidget(measuredLabel);
    auto* gaps=new QPlainTextEdit;gaps->setObjectName("taskBreaks");gaps->setPlaceholderText("2026-10-26 13:00 | 2026-10-28 09:00\n중단 시작 | 재개 시각 (한 줄에 한 구간)");QStringList lines;for(const auto& b:t.breaks)lines<<QString::fromStdString(b.start)+" | "+QString::fromStdString(b.finish);gaps->setPlainText(lines.join('\n'));form->addRow("분할 중단 구간",gaps);
    auto* gapNote=new QLabel("중단은 이 작업에만 적용하는 절대 시각 구간입니다. 날짜순으로 입력하고 겹치지 않게 하세요. 빈칸으로 적용하면 분할을 해제합니다. 날짜 이동 후에도 중단 시각은 유지되며, 완료 뒤 구간은 효과가 없습니다. 자원별 달력은 자원 등록에서 선택합니다.");gapNote->setWordWrap(true);layout->addWidget(gapNote);
    auto* error=new QLabel;error->setObjectName("effortError");error->setWordWrap(true);layout->addWidget(error);
    auto* buttons=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);layout->addWidget(buttons);const bool editable=!document_.readOnly();buttons->button(QDialogButtonBox::Ok)->setEnabled(editable);type->setEnabled(editable);basis->setEnabled(editable);duration->setReadOnly(!editable);work->setReadOnly(!editable);gaps->setReadOnly(!editable);
    auto fields=[&]{const auto kind=type->currentData().toString();const bool workInput=basis->currentData()=="work";work->setEnabled(editable&&(workInput||kind=="fixed-work"));duration->setEnabled(editable&&(!workInput||kind=="fixed-duration"));};
    connect(type,&QComboBox::currentIndexChanged,&dialog,[&]{fields();});connect(basis,&QComboBox::currentIndexChanged,&dialog,[&]{fields();});fields();
    connect(buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
    connect(buttons,&QDialogButtonBox::accepted,&dialog,[&]{try {
        if(document_.project()->documentId!=snapshot.documentId||document_.project()->revision!=snapshot.revision)throw std::runtime_error("문서가 변경되었습니다. 편집 창을 다시 여세요.");
        auto minutes=t.durationMinutes,hours=t.workUnitMinutes;bool ok=false;
        if(duration->isEnabled()){minutes.reset();if(!duration->text().trimmed().isEmpty()){auto n=duration->text().trimmed().toLongLong(&ok);if(!ok||n<0||n>5000000)throw std::runtime_error("기간은 0~5,000,000 근무분 정수입니다.");minutes=n;}}
        if(work->isEnabled()){hours.reset();if(!work->text().trimmed().isEmpty()){auto n=work->text().trimmed().toDouble(&ok);if(!ok||!std::isfinite(n)||n<0||n>double(maxWork)/6000.0)throw std::runtime_error("공수 범위를 확인하세요.");hours=std::llround(n*6000.0);}}
        std::vector<TaskBreak> breaks;for(auto line:gaps->toPlainText().split('\n',Qt::SkipEmptyParts)){auto parts=line.split('|');if(parts.size()!=2)throw std::runtime_error("중단 시작 | 재개 시각으로 입력하세요.");breaks.push_back({parts[0].trimmed().toStdString(),parts[1].trimmed().toStdString()});}
        const auto kind=type->currentData().toString().toStdString();const bool workInput=basis->currentData()=="work";
        document_.setTaskEffort(index,kind,workInput,minutes,hours,std::move(breaks));dialog.accept();
    }catch(const std::exception& e){error->setText(QString::fromUtf8(e.what()));}});
    dialog.exec();refresh();if(editable&&document_.project()->revision!=snapshot.revision)calculateSchedule();
}
}
