#include "formatting/QtDiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "ui/TaskTableModel.h"
#include "calculation/WorkCalendar.h"
#include <QBrush>
#include <QColor>
#include <QPointer>
#include <algorithm>
#include <unordered_set>
#include <QtConcurrent/QtConcurrentRun>

namespace jobs {
namespace {
QString qs(const std::string &value) {
    return QString::fromUtf8(value);
}
const QStringList headers{"작업명",
                          "기간(h / m)",
                          "선행(행·종류±지연)",
                          "시작",
                          "완료",
                          "총여유(h)",
                          "임계/상태",
                          "자원",
                          "안정 ID",
                          "작업 달력",
                          "제약",
                          "마감일",
                          "작업 유형 · 입력 공수(h)",
                          "분할 중단"};
} // namespace
TaskTableModel::TaskTableModel(Document &document, QObject *parent) : QAbstractTableModel(parent), document_(document) {
    setObjectName("taskModel");
}
int TaskTableModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : int(rows_.size());
}
int TaskTableModel::columnCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : 14;
}
QVariant TaskTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole || section < 0)
        return {};
    if (orientation == Qt::Horizontal)
        return section < headers.size() ? QVariant(headers[section]) : QVariant{};
    return section < rowCount() ? QVariant(QString::number(rows_[std::size_t(section)] + 1)) : QVariant{};
}
Qt::ItemFlags TaskTableModel::flags(const QModelIndex &index) const {
    if (!index.isValid() || index.model() != this || index.row() >= rowCount() || index.column() >= 14)
        return Qt::NoItemFlags;
    auto flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (!readOnly_ && index.column() < 3)
        flags |= Qt::ItemIsEditable;
    return flags;
}
const TaskSchedule *TaskTableModel::schedule(const Task &task) const {
    const auto found = schedules_.find(task.id);
    return found == schedules_.end() ? nullptr : &found->second;
}
QString TaskTableModel::text(const Task &task, int column) const {
    const auto *row = schedule(task);
    switch (column) {
    case 0:
        return qs(task.name);
    case 1:
        if (usesWork(task)) {
            const auto planned = plannedDates_.find(task.id);
            if (planned == plannedDates_.end())
                return {};
            EvaluationScope scope({}, renderCache_);
            WorkCalendar calendar(plan_, task);
            return QString::number(calendar.offset(planned->second.finish) - calendar.offset(planned->second.start)) +
                   "m";
        }
        return !task.durationMinutes            ? QString{}
               : *task.durationMinutes % 3 == 0 ? QString::number(double(*task.durationMinutes) / 60, 'g', 12)
                                                : QString::number(*task.durationMinutes) + "m";
    case 2: {
        QStringList edges;
        for (const auto &edge : dependencies(task)) {
            auto value = numbers_.contains(edge.predecessorId) ? QString::number(numbers_.at(edge.predecessorId))
                                                               : "누락:" + qs(edge.predecessorId);
            if (edge.type != DependencyType::FS || edge.lagMinutes)
                value += qs(enumToken(edge.type)) + (edge.lagMinutes >= 0 ? "+" : "") +
                         QString::number(edge.lagMinutes) + "m";
            edges << value;
        }
        return edges.join(',');
    }
    case 3:
        return row ? qs(row->start) : "미계산";
    case 4:
        return row ? qs(row->finish) : "미계산";
    case 5:
        return row && !result_->isForecast ? QString::number(double(row->totalFloatMinutes) / 60, 'g', 12) : "—";
    case 6:
        return !row                  ? "—"
               : result_->isForecast ? (row->completedActual ? "실제 완료" : "예측")
               : row->critical       ? "임계"
                                     : "여유";
    case 7:
        return assignments_.contains(task.id) ? assignments_.at(task.id) : "미설정";
    case 8:
        return qs(task.id);
    case 9:
        return qs(taskCalendarName(plan_, task));
    case 10:
        return qs(enumToken(task.constraintType)) + (task.constraintAt ? " " + qs(*task.constraintAt) : QString{});
    case 11:
        return task.deadlineAt ? qs(*task.deadlineAt) : "없음";
    case 12:
        return qs(taskTypeName(task.taskType)) +
               (task.workUnitMinutes ? " · " + QString::number(double(*task.workUnitMinutes) / 6000.0, 'f', 4)
                                     : " · 기간 기준");
    case 13:
        return QString::number(task.breaks.size());
    default:
        return {};
    }
}
QVariant TaskTableModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.model() != this || index.row() < 0 || index.row() >= rowCount() ||
        index.column() < 0 || index.column() >= 14)
        return {};
    const auto &task = plan_.tasks.at(rows_[std::size_t(index.row())]);
    const int column = index.column();
    if (role == Qt::DisplayRole || role == Qt::EditRole)
        return text(task, column);
    if (role == Qt::UserRole)
        return qs(task.id);
    const auto *row = schedule(task);
    const bool conflict = column == 0 && issues_.contains(task.id);
    const bool late = column == 11 && task.deadlineAt && row && row->finish > *task.deadlineAt;
    if (role == Qt::BackgroundRole)
        return conflict ? QVariant(QBrush(QColor("#ffddd7"))) : late ? QVariant(QBrush(QColor("#fff1cc"))) : QVariant{};
    if (role == Qt::ForegroundRole)
        return conflict ? QVariant(QBrush(QColor("#662015"))) : late ? QVariant(QBrush(QColor("#653b00"))) : QVariant{};
    if (role == Qt::ToolTipRole) {
        if (conflict)
            return "<qt>" + issues_.at(task.id).toHtmlEscaped() + "</qt>";
        if (late)
            return "마감 초과 · 마감일은 경고 기준이며 날짜를 강제로 이동하지 않습니다.";
        if (column == 10)
            return qs(constraintName(task.constraintType)) + " · " + text(task, 10);
        return "작업 ID: " + qs(task.id);
    }
    return {};
}
bool TaskTableModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role != Qt::EditRole || !(flags(index) & Qt::ItemIsEditable) || !editRequested || !document_.project() ||
        document_.project()->documentId != documentId_ || document_.project()->revision != revision_ ||
        document_.sessionGeneration() != session_)
        return false;
    const auto &task = plan_.tasks.at(rows_[std::size_t(index.row())]);
    const auto input = value.toString().trimmed();
    if (input == text(task, index.column()))
        return true;
    const auto taskId = task.id;
    const int row = index.row(), column = index.column();
    QPointer<TaskTableModel> guard(this);
    const bool accepted = editRequested(taskId, column, input);
    if (!guard)
        return accepted;
    // Canonical values also replace rejected/no-op drafts. A callback may reset
    // the filtered view, so recreate the index only if it is still in range.
    if (row < rowCount())
        emit dataChanged(this->index(row, column), this->index(row, column));
    return accepted;
}
bool TaskTableModel::synchronizeTask() {
    const auto &hint = document_.taskChange();
    const auto &project = document_.project();
    if (!initialized_ || !project || !hint || hint->before != revision_ || hint->after != project->revision ||
        session_ != document_.sessionGeneration() || documentId_ != project->documentId ||
        calculationGeneration_ != document_.calculationGeneration() || readOnly_ != document_.readOnly())
        return false;
    plan_.tasks.at(hint->row) = project->tasks.at(hint->row);
    lastTaskVisits_ = 1;
    revision_ = project->revision;
    if (!hint->nameOnly)
        renderCache_ = std::make_shared<EvaluationCache>();
    if (const auto found = positions_.find(hint->row); found != positions_.end()) {
        const int last = hint->nameOnly ? 0 : 13;
        emit dataChanged(index(found->second, 0), index(found->second, last));
    }
    return true;
}
bool TaskTableModel::synchronizeBulk(BulkView &view) {
    const auto &project = document_.project();
    if (!initialized_ || !project || view.before != revision_ || view.after != project->revision ||
        view.session != session_ || session_ != document_.sessionGeneration() || view.documentId != documentId_ ||
        readOnly_ != document_.readOnly() || view.input.tasks.size() != project->tasks.size() ||
        rows_.size() != plan_.tasks.size())
        return false;
    for (std::size_t i = 0; i < rows_.size(); ++i)
        if (rows_[i] != i)
            return false;
    if (document_.calculation() && document_.calculation()->status == CalculationStatus::Valid)
        return false;
    const auto oldSize = view.oldSize, newSize = view.input.tasks.size();
    const bool insert = newSize > oldSize && view.prefix + view.suffix == oldSize;
    const bool remove = newSize < oldSize && view.prefix + view.suffix == newSize;
    const bool reset = !view.sameOrder && !insert && !remove;
    const bool dates = calculationGeneration_ != document_.calculationGeneration();
    if (reset)
        beginResetModel();
    else if (insert)
        beginInsertRows({}, int(view.prefix), int(view.prefix + newSize - oldSize - 1));
    else if (remove)
        beginRemoveRows({}, int(view.prefix), int(view.prefix + oldSize - newSize - 1));
    std::swap(plan_, view.input);
    rows_.swap(view.rows);
    positions_.swap(view.positions);
    numbers_.swap(view.numbers);
    auto previousAssignments = std::move(assignments_);
    assignments_.clear();
    for (const auto &[id, label] : view.assignmentLabels)
        assignments_.emplace(id, qs(label));
    schedules_.clear();
    plannedDates_.clear();
    issues_.clear();
    result_.reset();
    renderCache_ = std::make_shared<EvaluationCache>();
    revision_ = project->revision;
    calculationGeneration_ = document_.calculationGeneration();
    lastTaskVisits_ = 0;
    if (reset)
        endResetModel();
    else if (insert)
        endInsertRows();
    else if (remove)
        endRemoveRows();
    // Old model storage is independent from QObject state and dies off the GUI thread.
    (void)QtConcurrent::run([input = std::move(view.input), numbers = std::move(view.numbers),
                             rows = std::move(view.rows), positions = std::move(view.positions),
                             assignments = std::move(previousAssignments)] {});
    if (!reset && rowCount()) {
        if (dates)
            emit dataChanged(index(0, 0), index(rowCount() - 1, 13));
        else
            for (const auto &[first, last] : view.changedRanges)
                if (first < rowCount())
                    emit dataChanged(index(first, 0), index(std::min(last, rowCount() - 1), 13));
        if (insert || remove) {
            emit dataChanged(index(0, 2), index(rowCount() - 1, 2));
            emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
        }
    }
    return true;
}
bool TaskTableModel::synchronizeProjected(BulkView &view) {
    const auto &p = document_.project();
    if (!initialized_ || !p || !view.projected || !view.contextValid || view.before != revision_ ||
        view.after != p->revision || view.session != session_ || session_ != document_.sessionGeneration() ||
        view.documentId != documentId_ || readOnly_ != document_.readOnly() || view.previousRows != rows_ ||
        view.sourceCalculationGeneration != calculationGeneration_ || view.input.tasks.size() != p->tasks.size() ||
        (document_.calculation() && document_.calculation()->status == CalculationStatus::Valid))
        return false;
    const bool reset = view.removals.size() + view.insertions.size() > 128;
    const bool dates = calculationGeneration_ != document_.calculationGeneration();
    decltype(assignments_) previousAssignments;
    decltype(schedules_) previousSchedules;
    decltype(issues_) previousIssues;
    std::shared_ptr<EvaluationCache> previousCache;
    // Keep old row data valid during remove/layout-about notifications.
    auto adopt = [&] {
        previousAssignments = std::move(assignments_);
        previousSchedules = std::move(schedules_);
        previousIssues = std::move(issues_);
        previousCache = std::move(renderCache_);
        std::swap(plan_, view.input);
        numbers_.swap(view.numbers);
        positions_.swap(view.positions);
        assignments_.clear();
        for (const auto &[id, label] : view.assignmentLabels)
            assignments_.emplace(id, qs(label));
        schedules_.clear();
        plannedDates_.clear();
        issues_.clear();
        result_.reset();
        renderCache_ = std::make_shared<EvaluationCache>();
        revision_ = p->revision;
        calculationGeneration_ = document_.calculationGeneration();
        lastTaskVisits_ = 0;
    };
    if (reset) {
        beginResetModel();
        adopt();
        rows_.swap(view.rows);
        endResetModel();
    } else {
        for (auto it = view.removals.rbegin(); it != view.removals.rend(); ++it) {
            beginRemoveRows({}, it->first, it->second);
            rows_.erase(rows_.begin() + it->first, rows_.begin() + it->second + 1);
            endRemoveRows();
        }
        adopt();
        rows_ = std::move(view.survivorOldOrderRows);
        if (view.reordered) {
            // Publish edited values before changing layout: persistent items must
            // keep the same data throughout the layout notification pair.
            if (rowCount())
                emit dataChanged(index(0, 0), index(rowCount() - 1, 13));
            emit layoutAboutToBeChanged();
            const auto from = persistentIndexList();
            QModelIndexList to;
            to.reserve(from.size());
            for (const auto &index : from) {
                const auto &id = plan_.tasks[rows_.at(std::size_t(index.row()))].id;
                to.push_back(this->index(view.survivorPositions.at(id), index.column()));
            }
            rows_ = std::move(view.survivorRows);
            changePersistentIndexList(from, to);
            emit layoutChanged();
        }
        for (const auto &[first, last] : view.insertions) {
            beginInsertRows({}, first, last);
            rows_.insert(rows_.begin() + first, view.rows.begin() + first, view.rows.begin() + last + 1);
            endInsertRows();
        }
        if (rowCount()) {
            if (dates)
                emit dataChanged(index(0, 0), index(rowCount() - 1, 13));
            else
                for (const auto &[first, last] : view.changedRanges)
                    emit dataChanged(index(first, 0), index(last, 13));
            if (!view.sameOrder)
                emit dataChanged(index(0, 2), index(rowCount() - 1, 2));
            if (!view.sameOrder || view.reordered || !view.removals.empty() || !view.insertions.empty())
                emit headerDataChanged(Qt::Vertical, 0, rowCount() - 1);
        }
    }
    (void)QtConcurrent::run([input = std::move(view.input), numbers = std::move(view.numbers),
                             positions = std::move(view.positions), assignments = std::move(previousAssignments),
                             schedules = std::move(previousSchedules), issues = std::move(previousIssues),
                             cache = std::move(previousCache)] {});
    return true;
}
void TaskTableModel::synchronize(const std::vector<std::size_t> &rows) {
    const auto &project = document_.project();
    const bool newSession = !initialized_ || session_ != document_.sessionGeneration();
    bool structure = project ? plan_.tasks.size() != project->tasks.size() : !plan_.tasks.empty();
    if (project && !structure)
        for (std::size_t i = 0; i < plan_.tasks.size(); ++i)
            if (plan_.tasks[i].id != project->tasks[i].id) {
                structure = true;
                break;
            }
    const bool reset = newSession || structure || rows_ != rows;
    if (reset)
        beginResetModel();
    if (!project) {
        plan_ = {};
        rows_.clear();
        positions_.clear();
        result_.reset();
        schedules_.clear();
        plannedDates_.clear();
        numbers_.clear();
        assignments_.clear();
        issues_.clear();
        documentId_.clear();
        session_ = document_.sessionGeneration();
        revision_ = 0;
        initialized_ = true;
        if (reset)
            endResetModel();
        return;
    }
    const auto &p = *project;
    lastTaskVisits_ = p.tasks.size();
    const bool metadata = !initialized_ || plan_.startDate != p.startDate || plan_.calendar != p.calendar ||
                          plan_.holidays != p.holidays || plan_.calculationProfile != p.calculationProfile ||
                          plan_.resources != p.resources || plan_.assignments != p.assignments ||
                          plan_.calendars != p.calendars;
    const bool calculation = calculationGeneration_ != document_.calculationGeneration();
    const bool all = reset || metadata || calculation || readOnly_ != document_.readOnly();
    std::unordered_set<std::size_t> changed;
    if (structure || newSession)
        plan_.tasks = p.tasks;
    else
        for (std::size_t i = 0; i < p.tasks.size(); ++i)
            if (plan_.tasks[i] != p.tasks[i]) {
                plan_.tasks[i] = p.tasks[i];
                changed.insert(i);
            }
    plan_.name = p.name;
    plan_.startDate = p.startDate;
    plan_.calendar = p.calendar;
    plan_.calculationProfile = p.calculationProfile;
    plan_.currency = p.currency;
    plan_.costProfile = p.costProfile;
    if (plan_.holidays != p.holidays)
        plan_.holidays = p.holidays;
    if (plan_.wbsNodes != p.wbsNodes)
        plan_.wbsNodes = p.wbsNodes;
    if (plan_.resources != p.resources)
        plan_.resources = p.resources;
    if (plan_.assignments != p.assignments)
        plan_.assignments = p.assignments;
    if (plan_.taskCosts != p.taskCosts)
        plan_.taskCosts = p.taskCosts;
    if (plan_.calendars != p.calendars)
        plan_.calendars = p.calendars;
    if (structure || newSession) {
        numbers_.clear();
        for (std::size_t i = 0; i < p.tasks.size(); ++i)
            numbers_[p.tasks[i].id] = int(i + 1);
    }
    if (metadata || newSession) {
        assignments_.clear();
        std::unordered_map<std::string, QString> names;
        for (const auto &resource : p.resources)
            names[resource.id] = qs(resource.name);
        for (const auto &assignment : p.assignments) {
            auto &label = assignments_[assignment.taskId];
            if (!label.isEmpty())
                label += ", ";
            label += names[assignment.resourceId] + " " + QString::number(assignment.units) + "%";
        }
    }
    if (calculation || newSession) {
        schedules_.clear();
        plannedDates_.clear();
        issues_.clear();
        result_.reset();
        if (document_.calculation()) {
            const auto &source = *document_.calculation();
            result_.emplace();
            result_->status = source.status;
            result_->isForecast = source.isForecast;
            if (source.status == CalculationStatus::Valid) {
                for (const auto &schedule : source.tasks)
                    schedules_[schedule.taskId] = schedule;
                // Forecast dates serve the result columns; duration edits use
                // the worker's plan, without recalculating on the GUI thread.
                for (const auto &planned : source.isForecast ? source.plannedTasks : source.tasks) {
                    const auto number = numbers_.find(planned.taskId);
                    if (number != numbers_.end() && usesWork(plan_.tasks[std::size_t(number->second - 1)]))
                        plannedDates_[planned.taskId] = {planned.start, planned.finish};
                }
            }
            if (source.status == CalculationStatus::Conflict)
                for (const auto &diagnostic : source.diagnostics)
                    for (const auto &id : diagnostic.taskIds)
                        if (!issues_.contains(id))
                            issues_[id] = qs(diagnosticMessage(diagnostic));
        }
    }
    if (metadata || !changed.empty() || reset)
        renderCache_ = std::make_shared<EvaluationCache>();
    rows_ = rows;
    documentId_ = p.documentId;
    session_ = document_.sessionGeneration();
    revision_ = p.revision;
    if (reset) {
        positions_.clear();
        for (int row = 0; row < int(rows_.size()); ++row)
            positions_.emplace(rows_[std::size_t(row)], row);
    }
    readOnly_ = document_.readOnly();
    calculationGeneration_ = document_.calculationGeneration();
    initialized_ = true;
    if (reset) {
        endResetModel();
        return;
    }
    for (int i = 0; i < rowCount();) {
        if (!all && !changed.contains(rows_[std::size_t(i)])) {
            ++i;
            continue;
        }
        const int first = i++;
        while (i < rowCount() && (all || changed.contains(rows_[std::size_t(i)])))
            ++i;
        emit dataChanged(index(first, 0), index(i - 1, 13));
    }
}
} // namespace jobs
