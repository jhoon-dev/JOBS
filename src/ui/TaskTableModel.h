#pragma once
#include "application/Document.h"
#include <QAbstractTableModel>
#include <QStringList>
#include <unordered_map>

namespace jobs {
class TaskTableModel final : public QAbstractTableModel {
public:
    explicit TaskTableModel(Document& document, QObject* parent = nullptr);
    int rowCount(const QModelIndex& parent = {}) const override;
    int columnCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex&, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex&) const override;
    bool setData(const QModelIndex&, const QVariant&, int role = Qt::EditRole) override;
    void synchronize(const std::vector<std::size_t>& rows);
    std::function<bool(std::string, int, QString)> editRequested;
private:
    Document& document_;
    PlanInput plan_;
    std::vector<std::size_t> rows_;
    std::optional<ScheduleResult> result_;
    std::string documentId_;
    std::uint64_t session_ = 0, revision_ = 0, calculationGeneration_ = 0;
    bool initialized_ = false, readOnly_ = false;
    std::unordered_map<std::string, int> numbers_;
    std::unordered_map<std::string, TaskSchedule> schedules_;
    std::unordered_map<std::string, QString> assignments_, issues_;
    mutable std::shared_ptr<EvaluationCache> renderCache_ = std::make_shared<EvaluationCache>();
    QString text(const Task&, int column) const;
    const TaskSchedule* schedule(const Task&) const;
};
}
