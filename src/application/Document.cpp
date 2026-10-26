#include "application/Document.h"
#include <stdexcept>

namespace jobs {
void Document::create(std::string name, std::string date, const std::vector<std::string>& tasks) {
    Project p;
    p.documentId = store_.newId(); p.name = std::move(name); p.startDate = std::move(date);
    for (const auto& name : tasks) p.tasks.push_back({store_.newId(), name});
    validate(p);
    project_ = std::move(p); path_.clear(); fingerprint_.clear(); lease_.reset();
    dirty_ = true; readOnly_ = false;
}
void Document::open(const std::string& path, bool readOnly) {
    auto normalized = store_.normalizePath(path);
    if (normalized == path_ && (lease_ || readOnly)) {
        auto loaded = store_.load(normalized);
        project_ = std::move(loaded.project); fingerprint_ = std::move(loaded.fingerprint);
        if (readOnly) lease_.reset();
        dirty_ = false; readOnly_ = readOnly; return;
    }
    auto lease = readOnly ? nullptr : store_.lock(normalized);
    auto loaded = store_.load(normalized);
    project_ = std::move(loaded.project); path_ = std::move(normalized);
    fingerprint_ = std::move(loaded.fingerprint); lease_ = std::move(lease);
    dirty_ = false; readOnly_ = readOnly;
}
void Document::renameTask(std::size_t row, std::string name) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.at(row).name = std::move(name);
    if (p == *project_) return;
    ++p.revision; validate(p); project_ = std::move(p); dirty_ = true;
}
void Document::addTask(std::string name) {
    if (!project_ || readOnly_) throw std::runtime_error("편집 가능한 프로젝트가 없습니다.");
    auto p = *project_; p.tasks.push_back({store_.newId(), std::move(name)});
    ++p.revision; validate(p); project_ = std::move(p); dirty_ = true;
}
void Document::save() {
    if (!project_ || path_.empty() || readOnly_) throw std::runtime_error("다른 이름으로 저장하세요.");
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
}
}
