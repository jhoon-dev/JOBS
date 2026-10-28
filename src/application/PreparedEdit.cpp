#include "application/Document.h"
#include "application/ProjectStore.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>

namespace jobs {
namespace {
// A worker can issue its reserved fact ID, but can never access project files.
class PreparationStore final : public ProjectStore {
public:
    explicit PreparationStore(std::string factId) : factId_(std::move(factId)) {}
    std::string newId() const override {
        if (factId_.empty() || issued_) throw std::logic_error("편집 준비에 예약된 ID가 없습니다.");
        issued_ = true;
        return factId_;
    }
    std::string normalizePath(const std::string&) const override { throw std::logic_error("편집 준비는 파일을 열 수 없습니다."); }
    std::unique_ptr<WriteLease> lock(const std::string&) override { throw std::logic_error("편집 준비는 파일을 잠글 수 없습니다."); }
    StoredProject load(const std::string&) const override { throw std::logic_error("편집 준비는 파일을 읽을 수 없습니다."); }
    std::string save(const std::string&, const Project&, const std::string&) override { throw std::logic_error("편집 준비는 파일을 저장할 수 없습니다."); }
private:
    std::string factId_;
    mutable bool issued_ = false;
};
std::size_t taskRow(const Project& p, const std::string& id) {
    const auto found = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto& t) { return t.id == id; });
    if (found == p.tasks.end()) throw std::runtime_error("편집할 작업이 없습니다.");
    return std::size_t(found - p.tasks.begin());
}
}
Document::EditRequest Document::makeEditRequest(EditCommand command) const {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto factId = std::holds_alternative<ProgressEdit>(command) ? store_.newId() : std::string{};
    return EditRequest(project_, savedState_, dirtyState_, std::move(command), sessionGeneration_, std::move(factId));
}
std::shared_ptr<Document::PreparedEdit> Document::prepareEdit(EditRequest request, std::shared_ptr<CalculationControl> control) {
    auto result = std::make_shared<PreparedEdit>();
    result->documentId_ = request.snapshot_->documentId;
    result->revision_ = request.snapshot_->revision;
    result->savedIdentity_ = request.saved_.identity();
    // Keep the identity owner alive: allocator address reuse must never make a
    // later save look like the captured save point.
    result->retired_ = request.saved_;
    result->session_ = request.session_;
    result->control_ = control;
    try {
        EvaluationScope evaluation(std::move(control));
        calculationProgress(CalculationStage::Validation, 0, request.snapshot_->tasks.size());
        PreparationStore store(std::move(request.factId_));
        Document trial(store);
        // The deep source copy happens on this worker, never in request capture.
        trial.project_ = Project(*request.snapshot_);
        trial.savedState_ = request.saved_; trial.dirtyState_ = request.dirty_;
        // Gantt editing also requires the exact source plan. It is prepared here,
        // rather than reusing live UI cache pointers in a worker.
        if (std::holds_alternative<GanttTaskEdit>(request.command_)) trial.applyCalculation(calculate(*trial.project_));
        std::visit([&](auto&& command) {
            using T = std::decay_t<decltype(command)>;
            if constexpr (std::is_same_v<T, DurationEdit>)
                trial.setDuration(taskRow(*trial.project_, command.taskId), command.minutes);
            else if constexpr (std::is_same_v<T, EffortEdit>)
                trial.setTaskEffort(taskRow(*trial.project_, command.taskId), std::move(command.type), command.workInput,
                                    command.duration, command.work, std::move(command.breaks));
            else if constexpr (std::is_same_v<T, AssignmentEdit>)
                trial.setAssignments(command.taskId, std::move(command.assignments));
            else if constexpr (std::is_same_v<T, ProgressEdit>)
                trial.recordProgress(std::move(command.entry));
            else if constexpr (std::is_same_v<T, BatchTaskEdit>)
                trial.editTasks(command.changes);
            else if constexpr (std::is_same_v<T, GanttTaskEdit>)
                trial.editGantt(result->documentId_, result->revision_, command.taskId, command.resize, command.target);
        }, std::move(request.command_));
        calculationCheckpoint();
        if (trial.project_->documentId != result->documentId_ ||
            (trial.project_->revision != result->revision_ && trial.project_->revision != result->revision_ + 1) || trial.history_.size() > 1)
            throw std::logic_error("편집 준비는 하나의 문서 변경이어야 합니다.");
        if (trial.project_->revision == result->revision_) result->state_ = EditState::NoChange;
        else {
            result->notice_ = std::move(trial.historyNotice_);
            if (!trial.history_.empty()) result->history_.emplace(std::move(trial.history_.back()));
            result->proposal_ = std::move(trial.project_);
            result->dirty_ = trial.dirtyState_;
            result->sourceDirty_ = compareSavedInput(*result->proposal_, request.snapshot_);
            calculationCheckpoint();
            result->taskChange_ = trial.taskChange_;
            result->state_ = EditState::Ready;
        }
    } catch (const CalculationCancelled&) {
        result->state_ = EditState::Cancelled;
    } catch (const std::exception& error) {
        result->state_ = EditState::Failed;
        result->error_ = error.what();
    }
    return result;
}
EditState Document::applyPreparedEdit(PreparedEdit& result) {
    if (result.consumed_) return EditState::Stale;
    if (result.control_ && result.control_->cancelRequested.load()) return EditState::Cancelled;
    if (!project_ || readOnly_ || project_->documentId != result.documentId_ || project_->revision != result.revision_ ||
        sessionGeneration_ != result.session_) return EditState::Stale;
    if (result.state_ != EditState::Ready && result.state_ != EditState::NoChange) return result.state_;
    if (result.state_ == EditState::NoChange) { result.consumed_ = true; return EditState::NoChange; }
    DirtyState dirty;
    if (savedState_.identity() == result.savedIdentity_) dirty = result.dirty_;
    else if (savedState_ && savedState_->documentId == result.documentId_ && savedState_->revision == result.revision_)
        dirty = result.sourceDirty_; // A normal save saved the exact request source.
    else return EditState::Stale;
    // Validation and delta construction completed in the isolated worker. Nothing
    // can change the live document between this final check and GUI-thread commit.
    if (result.history_) pushHistory(std::move(*result.history_));
    else clearHistory(std::move(result.notice_));
    auto previous = std::move(project_);
    project_ = std::move(result.proposal_);
    result.proposal_ = std::move(previous);
    result.consumed_ = true;
    dirtyState_ = dirty; dirty_ = dirty.dirty(); dirtyComparisons_ = 0;
    taskChange_ = result.taskChange_;
    staleCalculation(); notify(Event::Edited);
    return EditState::Applied;
}
std::vector<std::shared_ptr<const Project>> Document::PreparedEdit::takeRetiredProjects() {
    consumed_ = true;
    std::vector<std::shared_ptr<const Project>> snapshots;
    if (retired_) snapshots.push_back(retired_.snapshot());
    if (proposal_) snapshots.push_back(proposal_.snapshot());
    retired_.reset(); proposal_.reset();
    return snapshots;
}
}
