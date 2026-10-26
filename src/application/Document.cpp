#include "application/Document.h"
#include <stdexcept>
#include <algorithm>

namespace jobs {
void Document::create(std::string name, std::string date, const std::vector<std::string>& tasks) {
    Project p;
    p.documentId = store_.newId(); p.name = std::move(name); p.startDate = std::move(date);
    for (const auto& name : tasks) p.tasks.push_back({store_.newId(), name, std::nullopt, {}});
    validate(p);
    project_ = std::move(p); path_.clear(); fingerprint_.clear(); lease_.reset();
    dirty_ = true; readOnly_ = false;
    calculation_.reset(); requiresSaveAs_ = false;
}
void Document::open(const std::string& path, bool readOnly) {
    auto normalized = store_.normalizePath(path);
    if (normalized == path_ && (lease_ || readOnly)) {
        auto loaded = store_.load(normalized);
        project_ = std::move(loaded.project); fingerprint_ = std::move(loaded.fingerprint);
        requiresSaveAs_ = loaded.requiresSaveAs; calculation_.reset();
        if (readOnly) lease_.reset();
        dirty_ = false; readOnly_ = readOnly; return;
    }
    auto lease = readOnly ? nullptr : store_.lock(normalized);
    auto loaded = store_.load(normalized);
    project_ = std::move(loaded.project); path_ = std::move(normalized);
    fingerprint_ = std::move(loaded.fingerprint); lease_ = std::move(lease);
    requiresSaveAs_ = loaded.requiresSaveAs; calculation_.reset();
    dirty_ = false; readOnly_ = readOnly;
}
void Document::renameTask(std::size_t row, std::string name) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).name = std::move(name);
    edit(std::move(p));
}
void Document::addTask(std::string name, std::string wbsId) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.push_back({store_.newId(), std::move(name), std::nullopt, {}, std::move(wbsId)});
    edit(std::move(p));
}
std::string Document::addWbs(std::string name, std::string parentId) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; auto id = store_.newId();
    p.wbsNodes.push_back({id, std::move(name), std::move(parentId)}); edit(std::move(p)); return id;
}
void Document::renameWbs(const std::string& id, std::string name) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto& n) { return n.id == id; });
    if (it == p.wbsNodes.end()) throw std::runtime_error("WBS가 존재하지 않습니다.");
    it->name = std::move(name); edit(std::move(p));
}
void Document::moveWbs(const std::string& id, std::string parentId) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto& n) { return n.id == id; });
    if (it == p.wbsNodes.end()) throw std::runtime_error("WBS가 존재하지 않습니다.");
    it->parentId = std::move(parentId); edit(std::move(p));
}
void Document::removeWbs(const std::string& id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    for (const auto& n : p.wbsNodes) if (n.parentId == id) throw std::runtime_error("하위 WBS를 먼저 이동하거나 제거하세요.");
    for (const auto& t : p.tasks) if (t.wbsId == id) throw std::runtime_error("소속 작업을 먼저 다른 WBS로 이동하세요.");
    const auto count = std::erase_if(p.wbsNodes, [&](const auto& n) { return n.id == id; });
    if (!count) throw std::runtime_error("WBS가 존재하지 않습니다.");
    edit(std::move(p));
}
void Document::assignWbs(std::size_t row, std::string id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).wbsId = std::move(id); edit(std::move(p));
}
void Document::setDuration(std::size_t row, std::optional<std::int64_t> minutes) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).durationMinutes = minutes; edit(std::move(p));
}
void Document::setPredecessors(std::size_t row, std::vector<std::string> ids) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).predecessors = std::move(ids); edit(std::move(p));
}
void Document::setHolidays(std::vector<std::string> dates) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.holidays = std::move(dates); edit(std::move(p));
}
void Document::setStatusAt(std::optional<std::string> stamp) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.statusAt = std::move(stamp); edit(std::move(p));
}
std::string Document::appendBaseline(Baseline b) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    if (b.sourceDocumentId != project_->documentId || b.inputRevision != project_->revision || b.input != static_cast<const PlanInput&>(*project_))
        throw std::runtime_error("기준선 준비 중 문서가 변경되었습니다. 최신 계획으로 다시 저장하세요.");
    verifyBaseline(b);
    auto p = *project_; b.id = store_.newId(); b.version = p.baselines.size() + 1; auto id = b.id;
    p.baselines.push_back(std::move(b)); edit(std::move(p)); return id;
}
std::string Document::captureBaseline(std::string name, std::string author, std::string reason, std::string utc) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    return appendBaseline(prepareBaseline(*project_, std::move(name), std::move(author), std::move(reason), std::move(utc)));
}
void Document::recordProgress(ProgressEntry entry) {
    if (!project_ || readOnly_ || !project_->statusAt) throw std::runtime_error("실적 보고 전에 상태일을 설정하세요.");
    auto p = *project_; entry.id = store_.newId(); entry.statusAt = *p.statusAt; entry.recordedRevision = p.revision + 1;
    entry.sourceDocumentId = p.documentId;
    const auto task = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto& t) { return t.id == entry.taskId; });
    if (task == p.tasks.end()) throw std::runtime_error("보고할 작업이 존재하지 않습니다.");
    entry.planDurationMinutes = task->durationMinutes;
    entry.previousEntryId.clear();
    for (auto it = p.progress.rbegin(); it != p.progress.rend(); ++it) if (it->taskId == entry.taskId) { entry.previousEntryId = it->id; break; }
    p.progress.push_back(std::move(entry)); edit(std::move(p));
}
void Document::staleCalculation() {
    if (calculation_) calculation_->status = CalculationStatus::Stale;
}
std::string Document::putResource(Resource r) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    if (r.id.empty()) { r.id = store_.newId(); p.resources.push_back(r); }
    else {
        auto it = std::find_if(p.resources.begin(), p.resources.end(), [&](const auto& item) { return item.id == r.id; });
        if (it == p.resources.end()) throw std::runtime_error("자원이 존재하지 않습니다.");
        *it = r;
    }
    auto id = r.id; edit(std::move(p)); return id;
}
void Document::removeResource(const std::string& id) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    for (const auto& a : p.assignments) if (a.resourceId == id) throw std::runtime_error("작업 배정을 먼저 해제하세요.");
    if (!std::erase_if(p.resources, [&](const auto& r) { return r.id == id; })) throw std::runtime_error("자원이 존재하지 않습니다.");
    edit(std::move(p));
}
void Document::setAssignments(const std::string& id, std::vector<Assignment> assignments) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_;
    if (std::none_of(p.tasks.begin(), p.tasks.end(), [&](const auto& t) { return t.id == id; })) throw std::runtime_error("작업이 존재하지 않습니다.");
    for (auto& a : assignments) { if (a.taskId != id) throw std::runtime_error("다른 작업의 배정이 포함되었습니다."); }
    std::erase_if(p.assignments, [&](const auto& a) { return a.taskId == id; });
    p.assignments.insert(p.assignments.end(), assignments.begin(), assignments.end()); edit(std::move(p));
}
std::string Document::saveScenario(std::string name, std::string priority, int horizon, std::string utc,
                                  const std::string& sourceId, std::uint64_t sourceRevision) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    if (sourceId != project_->documentId || sourceRevision != project_->revision)
        throw std::runtime_error("비교 중 문서가 변경되었습니다. 최신 입력으로 다시 계산하세요.");
    auto p = *project_; LevelingScenario s;
    s.id = store_.newId(); s.name = std::move(name); s.createdAt = std::move(utc); s.sourceDocumentId = p.documentId; s.inputRevision = p.revision;
    s.priority = std::move(priority); s.horizonWorkDays = horizon;
    s.input = static_cast<const PlanInput&>(p); s.statusAt = p.statusAt; s.progress = p.progress;
    auto id = s.id; p.scenarios.push_back(std::move(s)); edit(std::move(p)); return id;
}
void Document::edit(Project p) {
    if (p == *project_) return;
    ++p.revision; validate(p); project_ = std::move(p); dirty_ = true; staleCalculation();
}
void Document::applyCalculation(ScheduleResult result) {
    if (!project_) return;
    if (result.documentId != project_->documentId || result.inputRevision != project_->revision || result.profile != project_->calculationProfile || result.statusAt != project_->statusAt)
        result.status = CalculationStatus::Stale;
    calculation_ = std::move(result);
}
void Document::save() {
    if (!project_ || path_.empty() || readOnly_ || requiresSaveAs_) throw std::runtime_error("다른 이름으로 저장하세요. 이전 형식의 원본은 보존합니다.");
    auto fingerprint = store_.save(path_, *project_, fingerprint_);
    fingerprint_ = std::move(fingerprint); dirty_ = false;
}
void Document::saveAs(const std::string& path) {
    if (!project_) throw std::runtime_error("열린 프로젝트가 없습니다.");
    auto normalized = store_.normalizePath(path);
    if (normalized == path_) {
        throw std::runtime_error("독립 사본은 현재 파일과 다른 경로에 저장하세요.");
    }
    auto lease = store_.lock(normalized);
    auto p = *project_;
    if (!path_.empty()) { p.derivedFrom = p.documentId; p.documentId = store_.newId(); }
    auto fingerprint = store_.save(normalized, p, {});
    project_ = std::move(p); path_ = std::move(normalized); fingerprint_ = std::move(fingerprint);
    lease_ = std::move(lease); dirty_ = false; readOnly_ = false;
    requiresSaveAs_ = false;
    if (calculation_ && calculation_->documentId != project_->documentId) staleCalculation();
}
}
