#pragma once
#include "calculation/Scheduler.h"
#include <QGraphicsView>
#include <QDate>
#include <QSet>
#include <functional>
namespace jobs {
class GanttView final : public QGraphicsView {
public:
    static constexpr int RowHeight = 28, HeaderHeight = 44;
    explicit GanttView(QWidget* parent = nullptr);
    void setPlan(const Project* project, const ScheduleResult* result, const std::vector<std::size_t>& rows);
    void setSelectedTask(const std::string& id);
    void setDayWidth(int width);
    void setBaseline(const Baseline* baseline);
    std::function<void(const std::string&)> taskSelected;
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void drawBackground(QPainter* painter, const QRectF& rect) override;
    void drawForeground(QPainter* painter, const QRectF& rect) override;
private:
    QDate firstDay_;
    QDate planFirstDay_, planLastDay_;
    struct BaselineDate { std::string id; QString start, finish; };
    std::vector<BaselineDate> baselineDates_;
    bool isForecast_ = false;
    QSet<QDate> holidays_;
    int dayWidth_ = 64, dayCount_ = 0;
    std::size_t displayRowCount_ = 0;
    std::string selectedId_;
    struct Row { std::string id; QString name, start, finish; bool critical; std::vector<std::string> predecessors, driving;
        std::optional<std::string> remainingStart; bool completedActual = false;
        std::vector<Dependency> edges = {}; QString calendar = {}; };
    std::vector<Row> rows_;
    void rebuild();
    qreal position(const QString& stamp) const;
};
}
