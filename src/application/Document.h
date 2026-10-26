#pragma once
#include "application/ProjectStore.h"
#include <optional>
#include "calculation/Scheduler.h"
#include "calculation/Tracking.h"
#include "application/Recovery.h"
#include <functional>
#include <map>

namespace jobs {
struct TaskEdit {
    std::string taskId;
    std::optional<std::string> name;
    std::optional<std::optional<std::int64_t>> duration;
    std::optional<std::vector<Dependency>> edges;
};
struct TaskRemoval {
    std::size_t tasks=0, links=0, assignments=0, costs=0;
    std::vector<std::string> protectedTaskIds;
};
class Document {
public:
    enum class Event { Edited, Saved, Replaced };
    explicit Document(ProjectStore& store) : store_(store) {}
    const std::optional<Project>& project() const { return project_; }
    const std::string& path() const { return path_; }
    bool dirty() const { return dirty_; }
    bool revisionSaved() const { return !dirty_ && project_ && savedState_ && project_->revision == savedState_->revision; }
    bool readOnly() const { return readOnly_; }
    bool requiresSaveAs() const { return requiresSaveAs_; }
    const std::optional<ScheduleResult>& calculation() const { return calculation_; }
    bool canUndo() const { return !readOnly_ && historyCursor_ > 0; }
    bool canRedo() const { return !readOnly_ && historyCursor_ < history_.size(); }
    std::string undoLabel() const;
    std::string redoLabel() const;
    const std::string& historyNotice() const { return historyNotice_; }
    void undo();
    void redo();
    std::uint64_t sessionGeneration() const { return sessionGeneration_; }
    std::uint64_t subscribe(std::function<void(Event)> listener);
    void unsubscribe(std::uint64_t token);
    RecoverySnapshot recoverySnapshot(std::string capturedAt) const;
    void recover(const RecoverySnapshot&);
    const std::optional<RecoveryOrigin>& recoveryOrigin() const { return recoveryOrigin_; }
    void create(std::string name, std::string startDate, const std::vector<std::string>& tasks);
    void open(const std::string& path, bool readOnly = false);
    void renameTask(std::size_t row, std::string name);
    void addTask(std::string name, std::string wbsId = {});
    std::vector<std::string> insertTasks(const std::vector<std::string>& names, const std::string& beforeId={}, const std::string& wbsId={});
    TaskRemoval previewRemoveTasks(const std::vector<std::string>& ids) const;
    void removeTasks(const std::vector<std::string>& ids);
    std::vector<std::string> duplicateTasks(const std::vector<std::string>& ids);
    void editTasks(const std::vector<TaskEdit>& changes);
    std::string addWbs(std::string name, std::string parentId = {});
    void renameWbs(const std::string& id, std::string name);
    void moveWbs(const std::string& id, std::string parentId);
    void removeWbs(const std::string& id);
    void assignWbs(std::size_t taskRow, std::string wbsId);
    void setDuration(std::size_t row, std::optional<std::int64_t> minutes);
    void setTaskEffort(std::size_t row,std::string type,bool workInput,std::optional<std::int64_t> duration,std::optional<std::int64_t> work,std::vector<TaskBreak> breaks);
    void setPredecessors(std::size_t row, std::vector<std::string> ids);
    void setDependencies(std::size_t row, std::vector<Dependency>);
    std::string putCalendar(TaskCalendar);
    void removeCalendar(const std::string&);
    void setTaskCalendar(std::size_t, std::string);
    void setTaskTiming(std::size_t, std::string type, std::optional<std::string> at, std::optional<std::string> deadline);
    void editGantt(const std::string& documentId, std::uint64_t revision, const std::string& taskId, bool resize, const std::string& target);
    void setHolidays(std::vector<std::string> dates);
    void setStatusAt(std::optional<std::string> stamp);
    std::string appendBaseline(Baseline prepared);
    std::string captureBaseline(std::string name, std::string approvedBy, std::string reason, std::string createdAt);
    void recordProgress(ProgressEntry entry);
    void setCurrency(std::string currency);
    void setTaskCost(TaskCost cost);
    void recordCost(CostEntry entry);
    std::string putResource(Resource resource);
    void removeResource(const std::string& id);
    void setAssignments(const std::string& taskId, std::vector<Assignment> assignments);
    std::string saveScenario(std::string name, std::string priority, int horizonWorkDays, std::string createdAt,
                             const std::string& sourceId, std::uint64_t sourceRevision);
    void applyCalculation(ScheduleResult result);
    void save();
    void saveAs(const std::string& path);
private:
    ProjectStore& store_;
    std::optional<Project> project_;
    std::string path_, fingerprint_;
    std::unique_ptr<WriteLease> lease_;
    std::optional<ScheduleResult> calculation_;
    struct InputState { PlanInput plan; std::optional<std::string> statusAt; };
    struct HistoryStep { InputState before, after; std::string label; std::size_t weight = 0; };
    std::vector<HistoryStep> history_;
    std::size_t historyCursor_ = 0, historyWeight_ = 0;
    std::optional<Project> savedState_;
    std::optional<RecoveryOrigin> recoveryOrigin_;
    std::string historyNotice_;
    std::uint64_t sessionGeneration_ = 0, listenerToken_ = 0;
    std::map<std::uint64_t, std::function<void(Event)>> listeners_;
    void notify(Event) noexcept;
    void clearHistory(std::string notice = {});
    void updateDirty();
    void applyHistory(const InputState&);
    void edit(Project project, std::string label = "입력 변경");
    void staleCalculation();
    bool dirty_ = false, readOnly_ = false;
    bool requiresSaveAs_ = false;
};
}
