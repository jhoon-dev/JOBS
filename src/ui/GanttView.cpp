#include "ui/GanttView.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTime>
#include <algorithm>
#include <unordered_map>

namespace jobs {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
QColor criticalColor() { return QColor("#e46c63"); }
QColor normalColor() { return QColor("#4997dd"); }
}
GanttView::GanttView(QWidget* parent) : QGraphicsView(parent) {
    setObjectName("ganttView"); setAccessibleName("간트 일정 · 표와 같은 작업 선택");
    setScene(new QGraphicsScene(this)); setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setRenderHint(QPainter::Antialiasing); setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    rebuild();
}
qreal GanttView::position(const QString& stamp) const {
    const auto day = QDate::fromString(stamp.left(10), Qt::ISODate);
    const auto time = QTime::fromString(stamp.mid(11), "HH:mm");
    return (qreal(firstDay_.daysTo(day)) + qreal(time.hour() * 60 + time.minute()) / 1440) * dayWidth_;
}
void GanttView::setPlan(const Project* p, const ScheduleResult* result, const std::vector<std::size_t>& indices) {
    const auto x = horizontalScrollBar()->value(), y = verticalScrollBar()->value();
    rows_.clear(); dayCount_ = 0; holidays_.clear(); displayRowCount_ = indices.size();
    if (p && result && result->status == CalculationStatus::Valid && result->documentId == p->documentId &&
        result->inputRevision == p->revision && result->profile == p->calculationProfile && result->projectFinish) {
        firstDay_ = QDate::fromString(qs(p->startDate), Qt::ISODate);
        const auto last = QDate::fromString(qs(*result->projectFinish).left(10), Qt::ISODate);
        dayCount_ = std::max(7, int(firstDay_.daysTo(last)) + 2);
        for (const auto& day : p->holidays) holidays_.insert(QDate::fromString(qs(day), Qt::ISODate));
        std::unordered_map<std::string, const TaskSchedule*> dates;
        for (const auto& row : result->tasks) dates.emplace(row.taskId, &row);
        bool complete = true;
        for (auto index : indices) {
            const auto& t = p->tasks.at(index); auto it = dates.find(t.id);
            if (it == dates.end()) { complete = false; break; }
            const auto& s = *it->second;
            rows_.push_back({t.id, QString::number(index + 1) + ". " + qs(t.name), qs(s.start), qs(s.finish),
                s.critical, t.predecessors, s.drivingPredecessors});
        }
        if (!complete) { rows_.clear(); dayCount_ = 0; }
    }
    rebuild(); horizontalScrollBar()->setValue(x); verticalScrollBar()->setValue(y);
}
void GanttView::setDayWidth(int width) {
    if (width != 16 && width != 64) return;
    auto day = qreal(horizontalScrollBar()->value()) / dayWidth_;
    dayWidth_ = width; rebuild(); horizontalScrollBar()->setValue(int(day * dayWidth_));
}
void GanttView::setSelectedTask(const std::string& id) {
    selectedId_ = id;
    for (auto* item : scene()->items()) {
        const auto kind = item->data(1).toString();
        if (kind != "task" && kind != "milestone") continue;
        const bool selected = item->data(0).toString() == qs(id);
        auto* shape = dynamic_cast<QAbstractGraphicsShapeItem*>(item);
        if (shape) shape->setPen(QPen(selected ? palette().color(QPalette::Text) : QColor("#202833"), selected ? 3.0 : 1.0));
    }
    viewport()->update();
}
void GanttView::rebuild() {
    scene()->clear();
    if (dayCount_ == 0) {
        auto* message = scene()->addText("F9로 최신 유효한 일정을 계산하세요.\n오래됨·누락·충돌 상태에서는 막대를 표시하지 않습니다.");
        message->setDefaultTextColor(palette().color(QPalette::Text)); message->setPos(12, 16);
        scene()->setSceneRect(0, 0, 380, std::max(120, HeaderHeight + int(displayRowCount_) * RowHeight)); return;
    }
    if (rows_.empty()) {
        auto* message = scene()->addText("선택한 WBS에 작업이 없습니다.");
        message->setDefaultTextColor(palette().color(QPalette::Text)); message->setPos(12, HeaderHeight + 8);
    }
    const auto width = dayCount_ * dayWidth_;
    const auto height = HeaderHeight + int(rows_.size()) * RowHeight;
    scene()->setSceneRect(0, 0, width, std::max(height, 120));
    std::unordered_map<std::string, std::size_t> index;
    for (std::size_t i = 0; i < rows_.size(); ++i) index.emplace(rows_[i].id, i);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        const auto& row = rows_[i]; const auto y = HeaderHeight + int(i) * RowHeight + RowHeight / 2;
        const auto start = position(row.start), finish = position(row.finish);
        QAbstractGraphicsShapeItem* bar;
        const bool milestone = row.start == row.finish;
        if (milestone) {
            QPolygonF diamond; diamond << QPointF(start, y - 7) << QPointF(start + 7, y) << QPointF(start, y + 7) << QPointF(start - 7, y);
            bar = scene()->addPolygon(diamond);
        } else bar = scene()->addRect(QRectF(start, y - 6, std::max(qreal(3), finish - start), 12));
        bar->setBrush(row.critical ? criticalColor() : normalColor());
        bar->setData(0, qs(row.id)); bar->setData(1, milestone ? "milestone" : "task"); bar->setZValue(2);
        const auto kind = milestone ? "마일스톤" : row.critical ? "임계 작업" : "여유 작업";
        QString tooltip = row.name + "\n" + kind + "\n" + row.start + " → " + row.finish + "\nID: " + qs(row.id);
        int hidden = 0;
        for (const auto& prev : row.predecessors) if (!index.contains(prev)) ++hidden;
        if (hidden) tooltip += "\n현재 WBS 밖 선행: " + QString::number(hidden) + "개 (전체 프로젝트 계산에 반영됨)";
        bar->setToolTip(tooltip);
        auto* label = scene()->addSimpleText(row.name + (milestone ? " ◆" : row.critical ? " [임계]" : ""));
        label->setBrush(palette().color(QPalette::Text)); label->setPos(finish + 9, y - 9); label->setZValue(3);
        label->setData(0, qs(row.id)); label->setData(1, "label"); label->setToolTip(tooltip);
        for (const auto& predecessor : row.predecessors) {
            auto found = index.find(predecessor); if (found == index.end()) continue;
            const auto& prev = rows_[found->second];
            auto x1 = position(prev.finish); auto y1 = HeaderHeight + int(found->second) * RowHeight + RowHeight / 2;
            auto elbow = std::max(x1 + 8, start - 10);
            QPainterPath path(QPointF(x1, y1)); path.lineTo(elbow, y1); path.lineTo(elbow, y - 10); path.lineTo(start - 5, y - 10); path.lineTo(start - 5, y); path.lineTo(start, y);
            const bool driving = row.critical && prev.critical && std::find(row.driving.begin(), row.driving.end(), predecessor) != row.driving.end();
            const auto color = driving ? criticalColor() : palette().color(QPalette::Text);
            auto* link = scene()->addPath(path, QPen(color, 1)); link->setZValue(1);
            link->setData(0, qs(predecessor + "->" + row.id)); link->setData(1, "dependency");
            link->setToolTip("FS 0 · " + qs(predecessor) + " → " + qs(row.id));
            QPolygonF arrow; arrow << QPointF(start, y) << QPointF(start - 5, y - 3) << QPointF(start - 5, y + 3);
            scene()->addPolygon(arrow, QPen(color), QBrush(color))->setZValue(1);
        }
    }
    setSelectedTask(selectedId_);
}
void GanttView::mousePressEvent(QMouseEvent* event) {
    auto* item = itemAt(event->pos());
    if (item && taskSelected && (item->data(1) == "task" || item->data(1) == "milestone" || item->data(1) == "label"))
        taskSelected(item->data(0).toString().toUtf8().toStdString());
    QGraphicsView::mousePressEvent(event);
}
void GanttView::drawBackground(QPainter* painter, const QRectF& rect) {
    painter->fillRect(rect, palette().brush(QPalette::Base));
    if (!dayCount_) return;
    auto first = std::max(0, int(rect.left() / dayWidth_)); auto last = std::min(dayCount_ - 1, int(rect.right() / dayWidth_));
    for (int i = first; i <= last; ++i) {
        const auto d = firstDay_.addDays(i); const auto x = i * dayWidth_;
        if (d.dayOfWeek() > 5 || holidays_.contains(d)) painter->fillRect(QRectF(x, rect.top(), dayWidth_, rect.height()), QColor(128, 128, 128, 40));
        painter->setPen(QPen(palette().color(QPalette::Mid), 0)); painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    painter->setPen(QPen(palette().color(QPalette::Mid), 0));
    for (int y = HeaderHeight + std::max(0, int((rect.top() - HeaderHeight) / RowHeight)) * RowHeight; y <= rect.bottom(); y += RowHeight)
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
}
void GanttView::drawForeground(QPainter* painter, const QRectF& rect) {
    if (!dayCount_) return;
    const auto top = mapToScene(QPoint(0, 0)).y();
    painter->fillRect(QRectF(rect.left(), top, rect.width(), HeaderHeight), palette().brush(QPalette::AlternateBase));
    painter->setPen(palette().color(QPalette::Text));
    auto first = std::max(0, int(rect.left() / dayWidth_)); auto last = std::min(dayCount_ - 1, int(rect.right() / dayWidth_));
    for (int i = first; i <= last; ++i) {
        const auto d = firstDay_.addDays(i); const auto x = i * dayWidth_;
        if (dayWidth_ >= 64 || d.dayOfWeek() == 1 || i == 0) {
            painter->drawText(QRectF(x + 3, top + 3, 90, 18), Qt::AlignLeft, d.toString(dayWidth_ >= 64 ? "MM-dd" : "yyyy-MM-dd"));
            painter->drawText(QRectF(x + 3, top + 23, 90, 17), Qt::AlignLeft, d.toString("ddd"));
        }
    }
    painter->drawLine(QPointF(rect.left(), top + HeaderHeight), QPointF(rect.right(), top + HeaderHeight));
}
}
