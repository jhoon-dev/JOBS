#include "formatting/PlanTokens.h"
#include "ui/GanttView.h"
#include "calculation/Effort.h"
#include <QGraphicsScene>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QTime>
#include <QResizeEvent>
#include <QScopedValueRollback>
#include <algorithm>
#include <unordered_map>

namespace jobs {
namespace {
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
QColor criticalColor() {
    return QColor("#e46c63");
}
QColor normalColor() {
    return QColor("#4997dd");
}
} // namespace
GanttView::GanttView(QWidget *parent) : QGraphicsView(parent) {
    setObjectName("ganttView");
    setAccessibleName("간트 일정 · 표와 같은 작업 선택");
    setScene(new QGraphicsScene(this));
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [this] { ensureVisibleRows(); });
    rebuild();
}
qreal GanttView::position(const QString &stamp) const {
    const auto day = QDate::fromString(stamp.left(10), Qt::ISODate);
    const auto time = QTime::fromString(stamp.mid(11), "HH:mm");
    return (qreal(firstDay_.daysTo(day)) + qreal(time.hour() * 60 + time.minute()) / 1440) * dayWidth_;
}
void GanttView::setPlan(const Project *p, const ScheduleResult *result, const std::vector<std::size_t> &indices,
                        bool editable) {
    EvaluationScope evaluation;
    cancelDrag();
    input_.reset();
    renderCache_ = std::make_shared<EvaluationCache>();
    editable_ = false;
    const auto x = horizontalScrollBar()->value(), y = verticalScrollBar()->value();
    rows_.clear();
    dayCount_ = 0;
    holidays_.clear();
    displayRowCount_ = indices.size();
    isForecast_ = result && result->isForecast;
    if (p && result && result->status == CalculationStatus::Valid && result->documentId == p->documentId &&
        result->inputRevision == p->revision && result->profile == p->calculationProfile && result->projectFinish) {
        input_ = static_cast<const PlanInput &>(*p);
        documentId_ = p->documentId;
        revision_ = p->revision;
        editable_ = editable && !isForecast_;
        firstDay_ = QDate::fromString(qs(p->startDate), Qt::ISODate);
        const auto last = QDate::fromString(qs(*result->projectFinish).left(10), Qt::ISODate);
        planFirstDay_ = firstDay_;
        planLastDay_ = last;
        dayCount_ = std::max(7, int(firstDay_.daysTo(last)) + 2);
        for (const auto &day : p->holidays)
            holidays_.insert(QDate::fromString(qs(day), Qt::ISODate));
        std::unordered_map<std::string, const TaskSchedule *> dates;
        for (const auto &row : result->tasks)
            dates.emplace(row.taskId, &row);
        bool complete = true;
        for (auto index : indices) {
            const auto &t = p->tasks.at(index);
            auto it = dates.find(t.id);
            if (it == dates.end()) {
                complete = false;
                break;
            }
            const auto &s = *it->second;
            rows_.push_back({t.id, QString::number(index + 1) + ". " + qs(t.name), qs(s.start), qs(s.finish),
                             s.critical, t.predecessors, s.drivingPredecessors, s.remainingStart, s.completedActual});
            rows_.back().edges = dependencies(t);
            rows_.back().calendar = qs(taskCalendarName(*p, t));
            auto &added = rows_.back();
            added.editable = editable_ && !std::any_of(p->progress.begin(), p->progress.end(), [&](const auto &e) {
                                 return e.taskId == t.id && e.actualStart.has_value();
                             });
            added.constraintType = t.constraintType;
            added.constraintAt = t.constraintAt;
            added.deadlineAt = t.deadlineAt;
            if (isEffort(*p) && !s.completedActual && s.start != s.finish) {
                added.executionTask = result->isForecast ? pendingTask(*p, t) : t;
                added.executionDates = s;
            }
            if (t.deadlineAt) {
                const auto due = QDate::fromString(qs(*t.deadlineAt).left(10), Qt::ISODate);
                if (due >= planFirstDay_ && due <= planFirstDay_.addDays(366 * 20 - 1))
                    planLastDay_ = std::max(planLastDay_, due);
            }
            for (const auto &c : p->calendars)
                if (c.id == t.calendarId) {
                    rows_.back().calendar += " · 근무 요일 ";
                    for (int d : c.weekdays)
                        rows_.back().calendar += QString::number(d) + " ";
                    for (const auto &period : c.periods)
                        rows_.back().calendar += QString(" %1:%2–%3:%4")
                                                     .arg(period.begin / 60, 2, 10, QChar('0'))
                                                     .arg(period.begin % 60, 2, 10, QChar('0'))
                                                     .arg(period.end / 60, 2, 10, QChar('0'))
                                                     .arg(period.end % 60, 2, 10, QChar('0'));
                    rows_.back().calendar += " · 휴일 ";
                    for (const auto &date : c.holidays)
                        rows_.back().calendar += qs(date) + " ";
                }
        }
        if (!complete) {
            rows_.clear();
            dayCount_ = 0;
        }
    }
    rebuild();
    horizontalScrollBar()->setValue(x);
    verticalScrollBar()->setValue(y);
    ensureVisibleRows();
}
void GanttView::setBaseline(const Baseline *baseline) {
    std::vector<BaselineDate> dates;
    if (baseline)
        for (const auto &row : baseline->schedule)
            dates.push_back({row.taskId, qs(row.start), qs(row.finish)});
    if (dates == baselineDates_)
        return;
    baselineDates_ = std::move(dates);
    rebuild();
}
void GanttView::setDayWidth(int width) {
    if (width != 16 && width != 64)
        return;
    auto day = qreal(horizontalScrollBar()->value()) / dayWidth_;
    dayWidth_ = width;
    rebuild();
    horizontalScrollBar()->setValue(int(day * dayWidth_));
}
void GanttView::setSelectedTask(const std::string &id) {
    const bool changed = selectedId_ != id;
    selectedId_ = id;
    if (changed && rows_.size() > 200) {
        const auto row = std::find_if(rows_.begin(), rows_.end(), [&](const auto &r) { return r.id == id; });
        if (row != rows_.end()) {
            const auto y = HeaderHeight + int(row - rows_.begin()) * RowHeight;
            const auto top = verticalScrollBar()->value();
            if (y < top + HeaderHeight || y + RowHeight > top + viewport()->height())
                verticalScrollBar()->setValue(std::max(0, y - HeaderHeight));
        }
    }
    for (auto *item : scene()->items()) {
        const auto kind = item->data(1).toString();
        if (kind != "task" && kind != "milestone")
            continue;
        const bool selected = item->data(0).toString() == qs(id);
        auto *shape = dynamic_cast<QAbstractGraphicsShapeItem *>(item);
        if (shape)
            shape->setPen(QPen(selected ? palette().color(QPalette::Text) : QColor("#202833"), selected ? 3.0 : 1.0));
    }
    for (auto *item : scene()->items())
        if (item->data(1) == "resizeHandle")
            item->setVisible(item->data(0).toString() == qs(id));
    viewport()->update();
}
void GanttView::ensureVisibleRows() {
    if (rebuilding_ || rows_.size() <= 200 || !dayCount_)
        return;
    const int first = std::max(0, (verticalScrollBar()->value() - HeaderHeight) / RowHeight);
    const int last = std::min(int(rows_.size()) - 1, (verticalScrollBar()->value() + viewport()->height()) / RowHeight);
    if (first < renderedFirst_ || last > renderedLast_)
        rebuild();
}
void GanttView::resizeEvent(QResizeEvent *event) {
    QGraphicsView::resizeEvent(event);
    ensureVisibleRows();
}
void GanttView::rebuild() {
    if (rebuilding_)
        return;
    QScopedValueRollback<bool> guard(rebuilding_, true);
    EvaluationScope evaluation({}, renderCache_);
    renderedFirst_ = renderedLast_ = -1;
    cancelDrag();
    scene()->clear();
    if (dayCount_ == 0) {
        auto *message =
            scene()->addText("일정 계산 후 간트가 표시됩니다.\n기간과 선행관계를 입력하고 F9로 계산하세요.");
        message->setDefaultTextColor(palette().color(QPalette::Text));
        message->setPos(12, 16);
        scene()->setSceneRect(0, 0, 380, std::max(120, HeaderHeight + int(displayRowCount_) * RowHeight));
        return;
    }
    firstDay_ = planFirstDay_;
    auto lastDay = planLastDay_;
    for (const auto &date : baselineDates_) {
        firstDay_ = std::min(firstDay_, QDate::fromString(date.start.left(10), Qt::ISODate));
        lastDay = std::max(lastDay, QDate::fromString(date.finish.left(10), Qt::ISODate));
    }
    dayCount_ = std::max(7, int(firstDay_.daysTo(lastDay)) + 2);
    if (rows_.empty()) {
        auto *message = scene()->addText("선택한 WBS에 작업이 없습니다.");
        message->setDefaultTextColor(palette().color(QPalette::Text));
        message->setPos(12, HeaderHeight + 8);
    }
    const auto width = dayCount_ * dayWidth_;
    const auto height = HeaderHeight + int(rows_.size()) * RowHeight;
    scene()->setSceneRect(0, 0, width, std::max(height, 120));
    std::unordered_map<std::string, std::size_t> index;
    std::unordered_map<std::string, const BaselineDate *> baseline;
    for (const auto &date : baselineDates_)
        baseline.emplace(date.id, &date);
    for (std::size_t i = 0; i < rows_.size(); ++i)
        index.emplace(rows_[i].id, i);
    renderedFirst_ =
        rows_.size() > 200 ? std::max(0, (verticalScrollBar()->value() - HeaderHeight) / RowHeight - 4) : 0;
    renderedLast_ = rows_.size() > 200 ? std::min(int(rows_.size()) - 1,
                                                  (verticalScrollBar()->value() + viewport()->height()) / RowHeight + 4)
                                       : int(rows_.size()) - 1;
    for (int visible = renderedFirst_; visible <= renderedLast_; ++visible) {
        const auto i = std::size_t(visible);
        auto &row = rows_[i];
        if (!row.piecesReady && input_ && row.executionTask && row.executionDates) {
            const auto &pending = *row.executionTask;
            const auto &s = *row.executionDates;
            if (usesWork(pending)) {
                auto spans = executionSpans(*input_, pending, s);
                std::sort(spans.begin(), spans.end(), [](const auto &a, const auto &b) { return a.start < b.start; });
                WorkCalendar clock(*input_);
                std::vector<std::pair<std::int64_t, std::int64_t>> merged;
                for (const auto &span : spans) {
                    if (!merged.empty() && span.start <= merged.back().second)
                        merged.back().second = std::max(merged.back().second, span.finish);
                    else
                        merged.emplace_back(span.start, span.finish);
                }
                for (const auto &[begin, end] : merged)
                    row.pieces.emplace_back(clock.boundary(begin, false), clock.boundary(end, true));
            } else
                row.pieces = WorkCalendar(*input_, pending).segments(s.remainingStart.value_or(s.start), s.finish);
            row.piecesReady = true;
        }
        const auto y = HeaderHeight + int(i) * RowHeight + RowHeight / 2;
        const auto start = position(row.start), finish = position(row.finish);
        if (baseline.contains(row.id)) {
            const auto &old = *baseline.at(row.id);
            const auto x = position(old.start), end = position(old.finish);
            auto *oldBar = scene()->addRect(QRectF(x, y + 8, std::max(qreal(3), end - x), 3),
                                            QPen(QColor("#a0a0a0"), 1, Qt::DashLine), QBrush(QColor("#a0a0a0")));
            oldBar->setData(0, qs(row.id));
            oldBar->setData(1, "baseline");
            oldBar->setToolTip("고정 기준선\n" + old.start + " → " + old.finish);
            oldBar->setZValue(1);
        }
        QAbstractGraphicsShapeItem *bar;
        const bool milestone = row.start == row.finish;
        if (milestone) {
            QPolygonF diamond;
            diamond << QPointF(start, y - 7) << QPointF(start + 7, y) << QPointF(start, y + 7) << QPointF(start - 7, y);
            bar = scene()->addPolygon(diamond);
        } else if (input_ && isEffort(*input_) && !row.completedActual) {
            const auto first = row.remainingStart ? qs(*row.remainingStart) : row.start;
            QPainterPath path;
            if (isForecast_ && first > row.start)
                path.addRect(QRectF(start, y - 3, std::max(qreal(1), position(first) - start), 6));
            for (const auto &[begin, end] : row.pieces)
                path.addRect(QRectF(position(qs(begin)), y - 6,
                                    std::max(qreal(1), position(qs(end)) - position(qs(begin))), 12));
            bar = scene()->addPath(path);
            auto *connector = scene()->addLine(start, y, finish, y, QPen(normalColor(), 1, Qt::DotLine));
            connector->setData(0, qs(row.id));
            connector->setData(1, "splitConnector");
            connector->setToolTip("점선: 비근무/중단 구간 · 공수·부하에서 제외");
            connector->setZValue(1);
        } else
            bar = scene()->addRect(QRectF(start, y - 6, std::max(qreal(3), finish - start), 12));
        bar->setBrush(row.completedActual ? QColor("#51b384") : row.critical ? criticalColor() : normalColor());
        bar->setData(0, qs(row.id));
        bar->setData(1, milestone ? "milestone" : "task");
        bar->setZValue(2);
        const auto kind = row.completedActual ? "실제 완료"
                          : isForecast_       ? "실적 반영 예측"
                          : milestone         ? "마일스톤"
                          : row.critical      ? "임계 작업"
                                              : "여유 작업";
        QString tooltip = row.name + "\n" + kind + "\n" + row.start + " → " + row.finish + "\nID: " + qs(row.id);
        tooltip += "\n작업 달력: " + row.calendar +
                   "\n막대는 시작~완료 구간입니다. 비근무 시간은 기간/자원 공수에서 제외합니다.";
        tooltip += "\n제약: " + qs(constraintName(row.constraintType)) + " (" + qs(enumToken(row.constraintType)) +
                   ")" + (row.constraintAt ? " " + qs(*row.constraintAt) : QString{}) +
                   "\n마감: " + (row.deadlineAt ? qs(*row.deadlineAt) : "없음");
        tooltip += row.editable
                       ? "\n드래그=날짜 이동 · 오른쪽 끝/Shift+드래그=기간 · 15분 격자 · Esc=취소 · 적용 후 자동 재계산"
                       : "\n현재 작업은 드래그할 수 없습니다 (읽기 전용/예측/실제 시작 이력).";
        if (row.remainingStart)
            tooltip += "\n잔여 작업 재개: " + qs(*row.remainingStart);
        int hidden = 0;
        for (const auto &prev : row.predecessors)
            if (!index.contains(prev))
                ++hidden;
        if (hidden)
            tooltip += "\n현재 WBS 밖 선행: " + QString::number(hidden) + "개 (전체 프로젝트 계산에 반영됨)";
        bar->setToolTip(tooltip);
        if (row.editable && !milestone) {
            auto *handle =
                scene()->addRect(QRectF(finish - 3, y - 8, 6, 16), QPen(QColor("#153b5a")), QBrush(QColor("#d6eafa")));
            handle->setData(0, qs(row.id));
            handle->setData(1, "resizeHandle");
            handle->setZValue(5);
            handle->setToolTip("오른쪽 끝 드래그: 작업 달력 근무 기간 변경 · 기존 제약 유지");
            handle->setVisible(row.id == selectedId_);
        }
        if (row.deadlineAt) {
            const auto x = position(qs(*row.deadlineAt));
            const auto color = row.finish > qs(*row.deadlineAt) ? QColor("#e66140") : QColor("#bd7d15");
            if (x >= 0 && x <= width) {
                auto *marker = scene()->addLine(x, y - 14, x, y + 13, QPen(color, 2, Qt::DashLine));
                marker->setData(0, qs(row.id));
                marker->setData(1, "deadline");
                marker->setToolTip("마감 " + qs(*row.deadlineAt) + " · 완료 " + row.finish +
                                   " · 경고 기준 (날짜 강제 이동 없음)");
                marker->setZValue(1);
            }
        }
        auto *label = scene()->addSimpleText(row.name + (row.completedActual ? " [실제]"
                                                         : isForecast_       ? " [예측]"
                                                         : milestone         ? " ◆"
                                                         : row.critical      ? " [임계]"
                                                                             : ""));
        label->setBrush(palette().color(QPalette::Text));
        label->setPos(finish + 9, y - 9);
        label->setZValue(3);
        label->setData(0, qs(row.id));
        label->setData(1, "label");
        label->setToolTip(tooltip);
        for (const auto &edge : row.edges) {
            const auto &predecessor = edge.predecessorId;
            auto found = index.find(predecessor);
            if (found == index.end())
                continue;
            const auto &prev = rows_[found->second];
            auto x1 = position(dependencyFromFinish(edge.type) ? prev.finish : prev.start);
            auto y1 = HeaderHeight + int(found->second) * RowHeight + RowHeight / 2;
            const auto target = dependencyToFinish(edge.type)       ? finish
                                : isForecast_ && row.remainingStart ? position(qs(*row.remainingStart))
                                                                    : start;
            auto elbow = std::max(x1 + 8, target - 10);
            QPainterPath path(QPointF(x1, y1));
            path.lineTo(elbow, y1);
            path.lineTo(elbow, y - 10);
            path.lineTo(target - 5, y - 10);
            path.lineTo(target - 5, y);
            path.lineTo(target, y);
            const bool driving = row.critical && prev.critical &&
                                 std::find(row.driving.begin(), row.driving.end(), predecessor) != row.driving.end();
            const auto color = driving ? criticalColor() : palette().color(QPalette::Text);
            const bool historical = isForecast_ && row.completedActual;
            auto *link = scene()->addPath(path, QPen(color, 1, historical ? Qt::DashLine : Qt::SolidLine));
            link->setZValue(1);
            link->setData(0, qs(predecessor + "->" + row.id));
            link->setData(1, "dependency");
            link->setToolTip(qs(enumToken(edge.type)) + (edge.lagMinutes >= 0 ? "+" : "") +
                             QString::number(edge.lagMinutes) + "m · 후행 달력 근무분 · " +
                             (historical ? "실제 날짜 고정 · " : "") + qs(predecessor) + " → " + qs(row.id));
            QPolygonF arrow;
            arrow << QPointF(target, y) << QPointF(target - 5, y - 3) << QPointF(target - 5, y + 3);
            scene()->addPolygon(arrow, QPen(color), QBrush(color))->setZValue(1);
        }
    }
    setSelectedTask(selectedId_);
}
void GanttView::drawBackground(QPainter *painter, const QRectF &rect) {
    painter->fillRect(rect, palette().brush(QPalette::Base));
    if (!dayCount_)
        return;
    auto first = std::max(0, int(rect.left() / dayWidth_));
    auto last = std::min(dayCount_ - 1, int(rect.right() / dayWidth_));
    for (int i = first; i <= last; ++i) {
        const auto d = firstDay_.addDays(i);
        const auto x = i * dayWidth_;
        if (d.dayOfWeek() > 5 || holidays_.contains(d))
            painter->fillRect(QRectF(x, rect.top(), dayWidth_, rect.height()), QColor(128, 128, 128, 25));
        painter->setPen(QPen(palette().color(QPalette::Midlight), 0));
        painter->drawLine(QPointF(x, rect.top()), QPointF(x, rect.bottom()));
    }
    painter->setPen(QPen(palette().color(QPalette::Midlight), 0));
    for (int y = HeaderHeight + std::max(0, int((rect.top() - HeaderHeight) / RowHeight)) * RowHeight;
         y <= rect.bottom(); y += RowHeight)
        painter->drawLine(QPointF(rect.left(), y), QPointF(rect.right(), y));
}
void GanttView::drawForeground(QPainter *painter, const QRectF &rect) {
    if (!dayCount_)
        return;
    const auto top = mapToScene(QPoint(0, 0)).y();
    painter->fillRect(QRectF(rect.left(), top, rect.width(), HeaderHeight), palette().brush(QPalette::AlternateBase));
    painter->setPen(palette().color(QPalette::Text));
    auto first = std::max(0, int(rect.left() / dayWidth_));
    auto last = std::min(dayCount_ - 1, int(rect.right() / dayWidth_));
    for (int i = first; i <= last; ++i) {
        const auto d = firstDay_.addDays(i);
        const auto x = i * dayWidth_;
        if (dayWidth_ >= 64 || d.dayOfWeek() == 1 || i == 0) {
            painter->drawText(QRectF(x + 3, top + 3, 90, 18), Qt::AlignLeft,
                              d.toString(dayWidth_ >= 64 ? "MM-dd" : "yyyy-MM-dd"));
            painter->drawText(QRectF(x + 3, top + 23, 90, 17), Qt::AlignLeft, d.toString("ddd"));
        }
    }
    painter->drawLine(QPointF(rect.left(), top + HeaderHeight), QPointF(rect.right(), top + HeaderHeight));
}
} // namespace jobs
