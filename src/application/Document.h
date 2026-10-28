#pragma once
#include "application/ProjectStore.h"
#include "application/InputDelta.h"
#include "application/EditCommand.h"
#include "application/ProjectState.h"
#include <optional>
#include "calculation/Scheduler.h"
#include "calculation/Tracking.h"
#include "application/Recovery.h"
#include <functional>
#include <map>
#include <variant>
#include <unordered_map>

namespace jobs {
struct TaskRemoval {
    std::size_t tasks = 0, links = 0, assignments = 0, costs = 0;
    std::vector<std::string> protectedTaskIds;
};
struct BulkView {
    PlanInput input;
    std::unordered_map<std::string, int> numbers;
    std::unordered_map<std::string, std::string> assignmentLabels;
    std::vector<std::pair<int, int>> changedRanges;
    std::vector<std::size_t> rows;
    std::unordered_map<std::size_t, int> positions;
    WbsSummary summary;
    std::vector<std::string> createdIds;
    bool clearSelection = false, sameOrder = false;
    std::size_t prefix = 0, suffix = 0, oldSize = 0;
    std::uint64_t before = 0, after = 0, session = 0;
    std::string documentId;
    bool projected = false, contextValid = true, reordered = false;
    std::string viewToken;
    std::uint64_t sourceCalculationGeneration = 0;
    std::size_t scopeTaskCount = 0;
    std::vector<std::size_t> previousRows, survivorRows, survivorOldOrderRows;
    std::vector<std::pair<int, int>> removals, insertions;
    std::unordered_map<std::string, int> survivorPositions;
};
class Document {
public:
    class EditRequest;
    class PreparedEdit;
    using ViewPreparation = std::function<void(BulkView &, const Project &, const Project &)>;
    EditRequest makeEditRequest(EditCommand command, ViewPreparation viewPreparation = {}) const;
    static std::shared_ptr<PreparedEdit> prepareEdit(EditRequest request, std::shared_ptr<CalculationControl> control);
    EditState applyPreparedEdit(PreparedEdit &result);
    enum class Event { Edited, Saved, Replaced };
    explicit Document(ProjectStore &store) : store_(store) {}
    const std::optional<Project> &project() const {
        return project_.value();
    }
    // Owning, read-only snapshot; borrowed project() references last until mutation.
    std::shared_ptr<const Project> snapshot() const {
        return project_.snapshot();
    }
    bool snapshotShared() const {
        return project_.shared();
    }
    struct TaskChange {
        std::uint64_t before = 0, after = 0;
        std::size_t row = 0;
        bool nameOnly = false;
    };
    const std::optional<TaskChange> &taskChange() const {
        return taskChange_;
    }
    std::size_t lastDirtyTaskComparisons() const {
        return dirtyComparisons_;
    }
    const std::string &path() const {
        return path_;
    }
    bool dirty() const {
        return dirty_;
    }
    bool revisionSaved() const {
        return !dirty_ && project_ && savedState_ && project_->revision == savedState_->revision;
    }
    bool readOnly() const {
        return readOnly_;
    }
    bool requiresSaveAs() const {
        return requiresSaveAs_;
    }
    const std::optional<ScheduleResult> &calculation() const {
        return calculation_;
    }
    std::uint64_t calculationGeneration() const {
        return calculationGeneration_;
    }
    bool canUndo() const {
        return !readOnly_ && historyCursor_ > 0;
    }
    bool canRedo() const {
        return !readOnly_ && historyCursor_ < history_.size();
    }
    std::string undoLabel() const;
    std::string redoLabel() const;
    const std::string &historyNotice() const {
        return historyNotice_;
    }
    std::size_t historyBytes() const {
        return historyWeight_;
    }
    bool historyNeedsWorker(bool redo) const;
    std::shared_ptr<BulkView> bulkView() const {
        return bulkView_;
    }
    void undo();
    void redo();
    std::uint64_t sessionGeneration() const {
        return sessionGeneration_;
    }
    std::uint64_t subscribe(std::function<void(Event)> listener);
    void unsubscribe(std::uint64_t token);
    RecoverySnapshot recoverySnapshot(std::string capturedAt) const;
    std::function<RecoverySnapshot()> recoverySnapshotRequest(std::string capturedAt) const;
    void recover(const RecoverySnapshot &);
    const std::optional<RecoveryOrigin> &recoveryOrigin() const {
        return recoveryOrigin_;
    }
    void create(std::string name, std::string startDate, const std::vector<std::string> &tasks);
    void open(const std::string &path, bool readOnly = false);
    void renameTask(std::size_t row, std::string name);
    void addTask(std::string name, std::string wbsId = {});
    std::vector<std::string> insertTasks(const std::vector<std::string> &names, const std::string &beforeId = {},
                                         const std::string &wbsId = {});
    TaskRemoval previewRemoveTasks(const std::vector<std::string> &ids) const;
    static TaskRemoval previewRemoveTasks(const Project &, const std::vector<std::string> &ids);
    void removeTasks(const std::vector<std::string> &ids);
    std::vector<std::string> duplicateTasks(const std::vector<std::string> &ids);
    void editTasks(const std::vector<TaskEdit> &changes);
    std::string addWbs(std::string name, std::string parentId = {});
    void renameWbs(const std::string &id, std::string name);
    void moveWbs(const std::string &id, std::string parentId);
    void removeWbs(const std::string &id);
    void assignWbs(std::size_t taskRow, std::string wbsId);
    void setDuration(std::size_t row, std::optional<std::int64_t> minutes);
    void setTaskEffort(std::size_t row, TaskType type, bool workInput, std::optional<std::int64_t> duration,
                       std::optional<std::int64_t> work, std::vector<TaskBreak> breaks);
    void setPredecessors(std::size_t row, std::vector<std::string> ids);
    void setDependencies(std::size_t row, std::vector<Dependency>);
    std::string putCalendar(TaskCalendar);
    void removeCalendar(const std::string &);
    void setTaskCalendar(std::size_t, std::string);
    void setTaskTiming(std::size_t, ConstraintType type, std::optional<std::string> at,
                       std::optional<std::string> deadline);
    void editGantt(const std::string &documentId, std::uint64_t revision, const std::string &taskId, bool resize,
                   const std::string &target);
    void setHolidays(std::vector<std::string> dates);
    void setStatusAt(std::optional<std::string> stamp);
    std::string appendBaseline(Baseline prepared);
    std::string captureBaseline(std::string name, std::string approvedBy, std::string reason, std::string createdAt);
    void recordProgress(ProgressEntry entry);
    void setCurrency(std::string currency);
    void setTaskCost(TaskCost cost);
    void recordCost(CostEntry entry);
    std::string putResource(Resource resource);
    void removeResource(const std::string &id);
    void setAssignments(const std::string &taskId, std::vector<Assignment> assignments);
    std::string saveScenario(std::string name, std::string priority, int horizonWorkDays, std::string createdAt,
                             const std::string &sourceId, std::uint64_t sourceRevision);
    void applyCalculation(ScheduleResult result);
    void save();
    void saveAs(const std::string &path);

private:
    bool isEditable() const;
    void requireEditable() const;
    ProjectStore &store_;
    ProjectState project_;
    std::string path_, fingerprint_;
    std::unique_ptr<WriteLease> lease_;
    std::optional<ScheduleResult> calculation_;
    std::uint64_t calculationGeneration_ = 0;
    struct TaskNameDelta {
        std::size_t row;
        std::string taskId, before, after;
    };
    struct HistoryStep {
        struct TaskValue {
            std::size_t row;
            std::shared_ptr<const std::pair<Task, Task>> values;
        };
        std::variant<TaskNameDelta, TaskValue, std::shared_ptr<const InputDelta>> change;
        std::string label;
        std::size_t weight = 0;
        bool planOnly = false;
    };
    std::vector<HistoryStep> history_;
    std::size_t historyCursor_ = 0, historyWeight_ = 0;
    ProjectState savedState_;
    DirtyState dirtyState_;
    std::size_t dirtyComparisons_ = 0;
    std::optional<TaskChange> taskChange_;
    std::optional<RecoveryOrigin> recoveryOrigin_;
    std::string historyNotice_;
    std::uint64_t sessionGeneration_ = 0, listenerToken_ = 0;
    std::map<std::uint64_t, std::function<void(Event)>> listeners_;
    void notify(Event) noexcept;
    void clearHistory(std::string notice = {});
    void updateDirty();
    void updateTaskDirty(std::size_t, const Task &before);
    void applyHistory(const HistoryStep &, bool forward);
    void pushHistory(HistoryStep);
    void edit(Project project, std::string label = "입력 변경");
    void staleCalculation();
    void editPlan(Project, std::string label, bool validated = false);
    std::shared_ptr<BulkView> bulkView_;
    bool dirty_ = false, readOnly_ = false;
    bool requiresSaveAs_ = false;
};
class Document::EditRequest {
public:
    EditRequest(EditRequest &&) = default;
    EditRequest &operator=(EditRequest &&) = default;
    EditRequest(const EditRequest &) = delete;

private:
    friend class Document;
    EditRequest(ProjectState snapshot, ProjectState saved, DirtyState dirty, EditCommand command, std::uint64_t session,
                std::string factId)
        : snapshot_(std::move(snapshot)), saved_(std::move(saved)), dirty_(dirty), command_(std::move(command)),
          session_(session), factId_(std::move(factId)) {}
    ProjectState snapshot_, saved_;
    DirtyState dirty_;
    EditCommand command_;
    std::uint64_t session_;
    std::string factId_;
    std::optional<HistoryStep> replay_;
    ViewPreparation viewPreparation_;
};
class Document::PreparedEdit {
public:
    PreparedEdit() = default;
    PreparedEdit(const PreparedEdit &) = delete;
    PreparedEdit &operator=(const PreparedEdit &) = delete;
    EditState state() const {
        return state_;
    }
    const std::string &error() const {
        return error_;
    }
    const std::optional<Diagnostic> &failure() const {
        return failure_;
    }
    std::vector<std::shared_ptr<const Project>> takeRetiredProjects();

private:
    friend class Document;
    EditState state_ = EditState::Failed;
    std::string error_, documentId_, notice_;
    std::optional<Diagnostic> failure_;
    std::uint64_t revision_ = 0, session_ = 0;
    std::shared_ptr<CalculationControl> control_;
    ProjectState proposal_, retired_;
    DirtyState dirty_, sourceDirty_;
    const void *savedIdentity_ = nullptr;
    std::optional<TaskChange> taskChange_;
    std::optional<HistoryStep> history_;
    bool consumed_ = false;
    std::optional<bool> replayForward_;
    std::shared_ptr<BulkView> view_;
};
} // namespace jobs
