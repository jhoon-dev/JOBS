#pragma once
#include "domain/PlanTypes.h"
#include "domain/Diagnostics.h"
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "domain/ScheduleTypes.h"

namespace jobs {
inline constexpr const char *pdmProfile = "native-pdm-taskcal-v1";
inline constexpr const char *constraintProfile = "native-pdm-constraints-v1";
inline constexpr const char *effortProfile = "native-effort-split-v1";
inline constexpr std::int64_t maxWork = 500000000000LL;
struct TaskBreak {
    std::string start, finish;
    bool operator==(const TaskBreak &) const = default;
};
struct Dependency {
    std::string predecessorId;
    DependencyType type = DependencyType::FS;
    std::int64_t lagMinutes = 0;
    bool operator==(const Dependency &) const = default;
};
struct WorkPeriod {
    int begin = 540, end = 720;
    bool operator==(const WorkPeriod &) const = default;
};
struct TaskCalendar {
    std::string id, name;
    std::vector<int> weekdays = {1, 2, 3, 4, 5};
    std::vector<WorkPeriod> periods = {{540, 720}, {780, 1080}};
    std::vector<std::string> holidays;
    bool operator==(const TaskCalendar &) const = default;
};
struct Task {
    std::string id;
    std::string name;
    std::optional<std::int64_t> durationMinutes;
    std::vector<std::string> predecessors;
    std::string wbsId = {};
    // IDs are the graph; only non-FS0 edge attributes are stored as overrides.
    std::vector<Dependency> dependencyRules = {};
    std::string calendarId = {};
    ConstraintType constraintType = ConstraintType::ASAP;
    std::optional<std::string> constraintAt = {};
    std::optional<std::string> deadlineAt = {};
    TaskType taskType = TaskType::FixedDuration;
    // 100 = one resource-minute, 6000 = one resource-hour. Never inferred actual work.
    std::optional<std::int64_t> workUnitMinutes = {};
    bool workDriven = false;
    std::vector<TaskBreak> breaks = {};
    bool operator==(const Task &) const = default;
};
struct WbsNode {
    std::string id, name, parentId;
    bool operator==(const WbsNode &) const = default;
};
// Renewable capacity, in hundredths of a unit (100 = one person/machine).
// Unknown capacity is distinct from zero. Exceptions replace the daily maximum.
struct CapacityDay {
    std::string date;
    std::optional<int> units;
    bool operator==(const CapacityDay &) const = default;
};
struct Resource {
    std::string id, name;
    std::string type = "labor";
    std::optional<int> maxUnits = 100;
    std::vector<CapacityDay> exceptions;
    struct Rate {
        std::string effectiveDate;
        std::optional<std::int64_t> hourlyMinor;
        bool operator==(const Rate &) const = default;
    };
    std::vector<Rate> rates = {};
    std::string calendarId = {};
    bool operator==(const Resource &) const = default;
};
struct Assignment {
    std::string taskId, resourceId;
    int units = 100;
    bool operator==(const Assignment &) const = default;
};
inline constexpr std::int64_t maxMoney = 9000000000000LL;
struct TaskCost {
    std::string taskId;
    std::optional<std::int64_t> fixedMinor;
    std::string accrual = "uniform";
    std::string credit = "physical";
    bool operator==(const TaskCost &) const = default;
};
struct PlanInput {
    std::string name;
    std::string startDate;
    std::string calendar = "standard-mon-fri-8h-v1";
    std::vector<Task> tasks;
    std::vector<std::string> holidays;
    std::string calculationProfile = "native-fs0-workminutes-v1";
    std::vector<WbsNode> wbsNodes;
    std::vector<Resource> resources;
    std::vector<Assignment> assignments;
    std::string currency = "KRW";
    std::string costProfile = "cost-day-minor-half-up-v1";
    std::vector<TaskCost> taskCosts;
    std::vector<TaskCalendar> calendars;
    bool operator==(const PlanInput &) const = default;
};
struct Baseline {
    std::string id, name, approvedBy, reason, createdAt;
    std::string sourceDocumentId;
    std::uint64_t inputRevision = 0, version = 0;
    PlanInput input;
    std::string engineVersion, projectFinish;
    std::vector<TaskSchedule> schedule;
    bool operator==(const Baseline &) const = default;
};
struct ProgressEntry {
    std::string id, taskId, statusAt, recordedAt, reporter, reason, previousEntryId;
    std::string sourceDocumentId;
    std::uint64_t recordedRevision = 0;
    std::optional<std::int64_t> planDurationMinutes;
    std::optional<std::string> actualStart, actualFinish;
    std::int64_t actualDurationMinutes = 0;
    std::int64_t remainingMinutes = 0;
    std::optional<std::int64_t> remainingWorkUnitMinutes = {};
    bool operator==(const ProgressEntry &) const = default;
};
struct LevelingScenario {
    std::string id, name, createdAt, sourceDocumentId;
    std::uint64_t inputRevision = 0;
    std::string policy = "serial-no-split-v1";
    std::string priority = "task-order";
    int horizonWorkDays = 260;
    PlanInput input;
    std::optional<std::string> statusAt;
    std::vector<ProgressEntry> progress;
    bool operator==(const LevelingScenario &) const = default;
};
struct CostEntry {
    std::string id, taskId, statusAt, recordedAt, reporter, reason, previousEntryId, sourceDocumentId, currency;
    std::uint64_t recordedRevision = 0;
    std::optional<std::int64_t> actualMinor;
    std::optional<int> physicalBasisPoints;
    bool operator==(const CostEntry &) const = default;
};
struct Project : PlanInput {
    std::string documentId;
    std::string derivedFrom;
    std::uint64_t revision = 1;
    std::vector<Baseline> baselines;
    std::vector<ProgressEntry> progress;
    std::optional<std::string> statusAt;
    std::vector<LevelingScenario> scenarios;
    std::vector<CostEntry> costEntries;
    bool operator==(const Project &) const = default;
};
// Domain uses ISO civil dates, without a UI locale or implicit timezone.
bool validDate(const std::string &date);
bool validStamp(const std::string &stamp);
bool validUtcStamp(const std::string &stamp);
void validatePlan(const PlanInput &plan);
void validateTaskName(const std::string &name);
void validate(const Project &project);
// Report-date selection, with later appended corrections winning a tie.
const ProgressEntry *progressAsOf(const Project &project, const std::string &taskId);
const CostEntry *costAsOf(const Project &, const std::string &taskId, const std::string &statusAt);
int currencyDecimals(const std::string &currency);
std::vector<Dependency> dependencies(const Task &);
bool isPdm(const PlanInput &);
bool hasConstraints(const PlanInput &);
bool isEffort(const PlanInput &);
bool usesWork(const Task &);
std::string engineFor(const PlanInput &);
std::string levelingPolicyFor(const PlanInput &);
} // namespace jobs
