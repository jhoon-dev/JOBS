#include "ui/GanttView.h"
#include "calculation/WorkCalendar.h"
#include <QApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QTime>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jobs {
namespace {
QString shifted(const QString& stamp,qint64 delta) {
    auto date=QDate::fromString(stamp.left(10),Qt::ISODate);auto time=QTime::fromString(stamp.mid(11),"HH:mm");
    const auto minute=date.toJulianDay()*1440+time.hour()*60+time.minute()+delta;
    return QDate::fromJulianDay(minute/1440).toString(Qt::ISODate)+" "+QTime(int(minute%1440)/60,int(minute%60)).toString("HH:mm");
}
}
void GanttView::cancelDrag(){drag_.reset();delete ghost_;ghost_=nullptr;delete ghostLabel_;ghostLabel_=nullptr;viewport()->unsetCursor();}
void GanttView::mousePressEvent(QMouseEvent* e) {
    cancelDrag();auto* item=itemAt(e->pos());const auto kind=item?item->data(1).toString():QString{};
    if(item&&(kind=="task"||kind=="milestone"||kind=="label"||kind=="resizeHandle")) {
        const auto id=item->data(0).toString().toStdString();if(taskSelected)taskSelected(id);
        const auto row=std::find_if(rows_.begin(),rows_.end(),[&](const auto& r){return r.id==id;});
        if(e->button()==Qt::LeftButton&&row!=rows_.end()&&row->editable&&taskEdited){
            const auto finishX=mapFromScene(QPointF(position(row->finish),0)).x();const auto startX=mapFromScene(QPointF(position(row->start),0)).x();
            const bool resize=row->start!=row->finish&&(kind=="resizeHandle"||e->modifiers().testFlag(Qt::ShiftModifier)||(finishX-startX>12&&std::abs(e->pos().x()-finishX)<=5));
            drag_=Drag{id,e->pos(),mapToScene(e->pos()).x(),resize,false,false,row->start,row->finish,{}};setFocus();e->accept();return;
        }
        if(editHint)editHint("현재 최신 계획에서만 드래그할 수 있습니다. 실제/잔여 예측·읽기 전용·실제 시작 이력은 편집 대상에서 제외합니다.");
    }
    QGraphicsView::mousePressEvent(e);
}
void GanttView::updateDrag(const QPoint& point) {
    if(!drag_||!input_)return;
    auto& d=*drag_;
    if(!d.moved&&(point-d.press).manhattanLength()<QApplication::startDragDistance())return;
    d.moved=true;d.valid=false;
    const auto it=std::find_if(input_->tasks.begin(),input_->tasks.end(),[&](const auto& t){return t.id==d.taskId;});if(it==input_->tasks.end())return;
    const auto delta=qint64(std::llround((mapToScene(point).x()-d.sceneX)/dayWidth_*1440/15))*15;
    try {
        WorkCalendar cal(*input_,*it);const auto raw=shifted(d.resize?d.finish:d.start,delta);if(!validStamp(raw.toStdString()))throw std::runtime_error("지원 날짜 범위 밖입니다.");
        QString start,finish,description;
        if(d.resize){const auto duration=cal.offset(raw.toStdString())-cal.offset(d.start.toStdString());if(duration<=0||duration>5000000)throw std::runtime_error("기간은 시작 뒤의 양수 근무분이어야 합니다.");start=d.start;finish=QString::fromStdString(cal.boundary(cal.offset(d.start.toStdString())+duration,true));d.target=finish;description="기간 "+QString::number(duration)+" 근무분 · 기존 제약 유지";}
        else{const auto local=*it->durationMinutes?cal.startAtLeast(raw.toStdString()):cal.finishAtLeast(raw.toStdString());start=QString::fromStdString(cal.boundary(local,*it->durationMinutes==0));finish=*it->durationMinutes?QString::fromStdString(cal.boundary(local+*it->durationMinutes,true)):start;d.target=start;description="날짜 이동 · "+QString::fromStdString(it->constraintType=="ASAP"||it->constraintType=="ALAP"?"SNET 시작 이후":it->constraintType)+" 제약 날짜 변경";}
        const auto row=std::find_if(rows_.begin(),rows_.end(),[&](const auto& r){return r.id==d.taskId;});const auto y=HeaderHeight+int(row-rows_.begin())*RowHeight+RowHeight/2;
        if(!ghost_){ghost_=scene()->addRect({},QPen(QColor("#177fb8"),2,Qt::DashLine),QBrush(QColor(35,150,220,65)));ghost_->setZValue(10);ghostLabel_=scene()->addSimpleText({});ghostLabel_->setZValue(11);ghostLabel_->setBrush(palette().color(QPalette::Text));}
        ghost_->setRect(QRectF(position(start),y-9,std::max(qreal(4),position(finish)-position(start)),18));ghostLabel_->setText(start+" → "+finish+" · "+description);ghostLabel_->setPos(position(start),y-30);
        d.valid=true;viewport()->setCursor(d.resize?Qt::SizeHorCursor:Qt::ClosedHandCursor);if(editHint)editHint(description+" · "+start+" → "+finish+" · 놓으면 선행관계/달력/제약을 재계산 · Esc 취소");
    }catch(const std::exception& error){if(ghost_)ghost_->setPen(QPen(QColor("#df5040"),2,Qt::DashLine));if(editHint)editHint("드래그 적용 불가: "+QString::fromUtf8(error.what()));}
}
void GanttView::mouseMoveEvent(QMouseEvent* e){if(drag_){if(!e->buttons().testFlag(Qt::LeftButton)){cancelDrag();return;}updateDrag(e->pos());e->accept();return;}QGraphicsView::mouseMoveEvent(e);}
void GanttView::mouseReleaseEvent(QMouseEvent* e){if(drag_&&e->button()==Qt::LeftButton){updateDrag(e->pos());const auto d=*drag_;const auto id=documentId_;const auto revision=revision_;cancelDrag();if(d.moved&&d.valid&&taskEdited)taskEdited(id,revision,d.taskId,d.resize,d.target.toStdString());e->accept();return;}QGraphicsView::mouseReleaseEvent(e);}
void GanttView::keyPressEvent(QKeyEvent* e){if(e->key()==Qt::Key_Escape&&drag_){cancelDrag();if(editHint)editHint("간트 드래그를 취소했습니다.");e->accept();return;}QGraphicsView::keyPressEvent(e);}
bool GanttView::event(QEvent* e){if(e->type()==QEvent::WindowDeactivate||e->type()==QEvent::Hide)cancelDrag();return QGraphicsView::event(e);}
}
