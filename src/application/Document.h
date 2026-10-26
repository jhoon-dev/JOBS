#pragma once
#include "application/ProjectStore.h"
#include <optional>

namespace jobs {
class Document {
public:
    explicit Document(ProjectStore& store) : store_(store) {}
    const std::optional<Project>& project() const { return project_; }
    const std::string& path() const { return path_; }
    bool dirty() const { return dirty_; }
    bool readOnly() const { return readOnly_; }
    void create(std::string name, std::string startDate, const std::vector<std::string>& tasks);
    void open(const std::string& path, bool readOnly = false);
    void renameTask(std::size_t row, std::string name);
    void addTask(std::string name);
    void save();
    void saveAs(const std::string& path);
private:
    ProjectStore& store_;
    std::optional<Project> project_;
    std::string path_, fingerprint_;
    std::unique_ptr<WriteLease> lease_;
    bool dirty_ = false, readOnly_ = false;
};
}
