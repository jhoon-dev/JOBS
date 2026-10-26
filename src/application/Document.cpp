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
void Document::staleCalculation() {
    if (calculation_) calculation_->status = CalculationStatus::Stale;
}
void Document::edit(Project p) {
    if (p == *project_) return;
    ++p.revision; validate(p); project_ = std::move(p); dirty_ = true; staleCalculation();
}
void Document::applyCalculation(ScheduleResult result) {
    if (!project_) return;
    if (result.documentId != project_->documentId || result.inputRevision != project_->revision || result.profile != project_->calculationProfile)
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
