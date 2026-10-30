#pragma once
#include "calculation/Scheduler.h"
#include <QGraphicsView>
#include <QDate>
#include <QSet>
#include <functional>
#include <unordered_map>
class QGraphicsRectItem;
class QGraphicsSimpleTextItem;
namespace jobs {
class GanttView final : public QGraphicsView {
public:
    static constexpr int RowHeight = 28, HeaderHeight = 44;
    explicit GanttView(QWidget *parent = nullptr);
    void setPlan(const Project *project, const ScheduleResult *result, const std::vector<std::size_t> &rows,
                 bool editable = false, std::shared_ptr<const Project> snapshot = {},
                 std::uint64_t calculationGeneration = 0);
    void setSelectedTask(const std::string &id);
    void setDayWidth(int width);
    void setBaseline(const Baseline *baseline);
    bool hasSchedule() const;
    void focusSelectedTask(bool center = true);
    void fitSchedule();
    std::function<void(const std::string &)> taskSelected;
    std::function<void(const std::string &, std::uint64_t, const std::string &, bool, const std::string &)> taskEdited;
    std::function<void(const QString &)> editHint;

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    bool event(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;
    void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    QDate firstDay_;
    QDate planFirstDay_, planLastDay_;
    struct BaselineDate {
        std::string id;
        QString start, finish;
        bool operator==(const BaselineDate &) const = default;
    };
    std::vector<BaselineDate> baselineDates_;
    std::unordered_map<std::string, std::size_t> rowIndex_, baselineIndex_;
    bool isForecast_ = false;
    QSet<QDate> holidays_;
    qreal dayWidth_ = 64;
    int dayCount_ = 0;
    bool fitted_ = false;
    bool chartSelection_ = false;
    std::size_t displayRowCount_ = 0;
    std::string selectedId_;
    std::shared_ptr<const PlanInput> input_;
    std::weak_ptr<const Project> inputSource_;
    std::uint64_t renderedCalculation_ = 0;
    std::vector<std::size_t> displayedIndices_;
    std::shared_ptr<EvaluationCache> renderCache_;
    std::string documentId_;
    std::uint64_t revision_ = 0;
    bool editable_ = false;
    struct Drag {
        std::string taskId;
        QPoint press;
        qreal sceneX;
        bool resize = false, moved = false, valid = false;
        QString start, finish, target;
    };
    std::optional<Drag> drag_;
    QGraphicsRectItem *ghost_ = nullptr;
    QGraphicsSimpleTextItem *ghostLabel_ = nullptr;
    void cancelDrag();
    void updateDrag(const QPoint &);
    struct Row {
        std::string id;
        QString name, start, finish;
        bool critical;
        std::vector<std::string> predecessors, driving;
        std::optional<std::string> remainingStart;
        bool completedActual = false;
        std::vector<Dependency> edges = {};
        QString calendar = {};
        bool editable = false;
        ConstraintType constraintType = ConstraintType::ASAP;
        std::optional<std::string> constraintAt = {}, deadlineAt = {};
        std::vector<std::pair<std::string, std::string>> pieces = {};
        bool piecesReady = false;
        std::optional<Task> executionTask = {};
        std::optional<TaskSchedule> executionDates = {};
    };
    std::vector<Row> rows_;
    bool rebuilding_ = false;
    int renderedFirst_ = -1, renderedLast_ = -1;
    void rebuild();
    void ensureVisibleRows();
    qreal position(const QString &stamp) const;
};
} // namespace jobs
