#pragma once
#include "application/ProjectStore.h"
#include <optional>
#include "calculation/Scheduler.h"
#include "calculation/Tracking.h"

namespace jobs {
class Document {
public:
    explicit Document(ProjectStore& store) : store_(store) {}
    const std::optional<Project>& project() const { return project_; }
    const std::string& path() const { return path_; }
    bool dirty() const { return dirty_; }
    bool readOnly() const { return readOnly_; }
    bool requiresSaveAs() const { return requiresSaveAs_; }
    const std::optional<ScheduleResult>& calculation() const { return calculation_; }
    void create(std::string name, std::string startDate, const std::vector<std::string>& tasks);
    void open(const std::string& path, bool readOnly = false);
    void renameTask(std::size_t row, std::string name);
    void addTask(std::string name, std::string wbsId = {});
    std::string addWbs(std::string name, std::string parentId = {});
    void renameWbs(const std::string& id, std::string name);
    void moveWbs(const std::string& id, std::string parentId);
    void removeWbs(const std::string& id);
    void assignWbs(std::size_t taskRow, std::string wbsId);
    void setDuration(std::size_t row, std::optional<std::int64_t> minutes);
    void setPredecessors(std::size_t row, std::vector<std::string> ids);
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
    void edit(Project project);
    void staleCalculation();
    bool dirty_ = false, readOnly_ = false;
    bool requiresSaveAs_ = false;
};
}
