#include "application/Document.h"
#include "application/ProjectStore.h"
#include <algorithm>
#include <stdexcept>
#include <type_traits>
#include <unordered_set>
#include <numeric>

namespace jobs {
namespace {
// A worker can issue its reserved fact ID, but can never access project files.
class PreparationStore final : public ProjectStore {
public:
    explicit PreparationStore(std::string factId, const Project& source, bool multiple)
        : factId_(std::move(factId)), multiple_(multiple) {
        for (const auto& task : source.tasks) occupied_.insert(task.id);
        for (const auto& node : source.wbsNodes) occupied_.insert(node.id);
        for (const auto& resource : source.resources) occupied_.insert(resource.id);
    }
    std::string newId() const override {
        if (multiple_) {
            if (factId_.empty()) throw std::logic_error("작업 ID seed가 없습니다.");
            std::string id;
            do { calculationCheckpoint(); id = factId_ + "-" + std::to_string(++counter_); } while (occupied_.contains(id));
            occupied_.insert(id); return id;
        }
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
    bool multiple_;
    mutable std::size_t counter_ = 0;
    mutable std::unordered_set<std::string> occupied_;
};
std::size_t taskRow(const Project& p, const std::string& id) {
    const auto found = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto& t) { return t.id == id; });
    if (found == p.tasks.end()) throw std::runtime_error("편집할 작업이 없습니다.");
    return std::size_t(found - p.tasks.begin());
}
std::shared_ptr<BulkView> prepareView(const Project& before, const Project& after, std::uint64_t session) {
    auto view = std::make_shared<BulkView>();
    view->input = after; view->before = before.revision; view->after = after.revision;
    view->documentId = after.documentId; view->session = session; view->oldSize = before.tasks.size();
    view->rows.resize(after.tasks.size()); std::iota(view->rows.begin(), view->rows.end(), std::size_t{0});
    const auto common = std::min(before.tasks.size(), after.tasks.size());
    while (view->prefix < common && before.tasks[view->prefix].id == after.tasks[view->prefix].id) ++view->prefix;
    while (view->suffix < common - view->prefix && before.tasks[before.tasks.size()-1-view->suffix].id == after.tasks[after.tasks.size()-1-view->suffix].id) ++view->suffix;
    view->sameOrder = before.tasks.size() == after.tasks.size() && view->prefix == common;
    std::unordered_map<std::string, const Task*> old;
    for (const auto& task : before.tasks) old.emplace(task.id, &task);
    for (std::size_t i = 0; i < after.tasks.size(); ++i) {
        calculationCheckpoint(); const auto& task = after.tasks[i]; view->numbers.emplace(task.id, int(i+1)); view->positions.emplace(i,int(i));
        const auto found = old.find(task.id);
        if (found != old.end() && *found->second == task) continue;
        if (!view->changedRanges.empty() && view->changedRanges.back().second == int(i)-1) view->changedRanges.back().second = int(i);
        else view->changedRanges.emplace_back(int(i), int(i));
    }
    std::unordered_map<std::string, std::string> names;
    for (const auto& resource : after.resources) names[resource.id] = resource.name;
    for (const auto& assignment : after.assignments) {
        calculationCheckpoint(); auto& label = view->assignmentLabels[assignment.taskId];
        if (!label.empty()) label += ", ";
        label += names.at(assignment.resourceId) + " " + std::to_string(assignment.units) + "%";
    }
    view->summary = summarizeWbs(after, nullptr, {});
    return view;
}
}
Document::EditRequest Document::makeEditRequest(EditCommand command) const {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    const bool reserve = std::holds_alternative<ProgressEdit>(command) || std::holds_alternative<InsertTasksEdit>(command) ||
        std::holds_alternative<DuplicateTasksEdit>(command) || std::holds_alternative<DeferredTaskEdit>(command);
    auto request = EditRequest(project_, savedState_, dirtyState_, std::move(command), sessionGeneration_, reserve ? store_.newId() : std::string{});
    if (auto* replay = std::get_if<HistoryEdit>(&request.command_)) {
        if (!historyNeedsWorker(replay->redo)) throw std::runtime_error("준비할 복합 이력이 없습니다.");
        request.replay_ = history_[replay->redo ? historyCursor_ : historyCursor_-1];
    }
    return request;
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
        if (auto* deferred = std::get_if<DeferredTaskEdit>(&request.command_)) {
            auto decoded = deferred->decode(*request.snapshot_);
            request.command_ = std::visit([](auto&& command) -> EditCommand { return std::move(command); }, std::move(decoded));
        }
        const bool bulk = std::holds_alternative<InsertTasksEdit>(request.command_) || std::holds_alternative<RemoveTasksEdit>(request.command_) ||
            std::holds_alternative<DuplicateTasksEdit>(request.command_) || std::holds_alternative<BatchTaskEdit>(request.command_) || request.replay_.has_value();
        std::vector<std::string> created;
        bool clearSelection = false;
        PreparationStore store(std::move(request.factId_), *request.snapshot_, !std::holds_alternative<ProgressEdit>(request.command_));
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
            else if constexpr (std::is_same_v<T, InsertTasksEdit>)
                created = trial.insertTasks(command.names, command.beforeId, command.wbsId);
            else if constexpr (std::is_same_v<T, RemoveTasksEdit>) {
                trial.removeTasks(command.ids); clearSelection = true;
            } else if constexpr (std::is_same_v<T, DuplicateTasksEdit>)
                created = trial.duplicateTasks(command.ids);
            else if constexpr (std::is_same_v<T, HistoryEdit>) {
                trial.applyHistory(*request.replay_, command.redo); result->replayForward_ = command.redo;
            }
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
            if (bulk) {
                result->view_ = prepareView(*request.snapshot_, *result->proposal_, result->session_);
                result->view_->createdIds = std::move(created); result->view_->clearSelection = clearSelection;
            }
            calculationCheckpoint();
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
    if (result.replayForward_) {
        if (*result.replayForward_) ++historyCursor_; else --historyCursor_;
    } else if (result.history_) pushHistory(std::move(*result.history_));
    else clearHistory(std::move(result.notice_));
    auto previous = std::move(project_);
    project_ = std::move(result.proposal_);
    result.proposal_ = std::move(previous);
    result.consumed_ = true;
    dirtyState_ = dirty; dirty_ = dirty.dirty(); dirtyComparisons_ = 0;
    taskChange_ = result.taskChange_;
    staleCalculation(); bulkView_ = std::move(result.view_); notify(Event::Edited);
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
