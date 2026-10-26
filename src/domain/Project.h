#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include "domain/ScheduleTypes.h"

namespace jobs {
struct Task {
    std::string id;
    std::string name;
    std::optional<std::int64_t> durationMinutes;
    std::vector<std::string> predecessors;
    std::string wbsId = {};
    bool operator==(const Task&) const = default;
};
struct WbsNode {
    std::string id, name, parentId;
    bool operator==(const WbsNode&) const = default;
};
struct PlanInput {
    std::string name;
    std::string startDate;
    std::string calendar = "standard-mon-fri-8h-v1";
    std::vector<Task> tasks;
    std::vector<std::string> holidays;
    std::string calculationProfile = "native-fs0-workminutes-v1";
    std::vector<WbsNode> wbsNodes;
    bool operator==(const PlanInput&) const = default;
};
struct Baseline {
    std::string id, name, approvedBy, reason, createdAt;
    std::string sourceDocumentId;
    std::uint64_t inputRevision = 0, version = 0;
    PlanInput input;
    std::string engineVersion, projectFinish;
    std::vector<TaskSchedule> schedule;
    bool operator==(const Baseline&) const = default;
};
struct ProgressEntry {
    std::string id, taskId, statusAt, recordedAt, reporter, reason, previousEntryId;
    std::string sourceDocumentId;
    std::uint64_t recordedRevision = 0;
    std::optional<std::int64_t> planDurationMinutes;
    std::optional<std::string> actualStart, actualFinish;
    std::int64_t actualDurationMinutes = 0;
    std::int64_t remainingMinutes = 0;
    bool operator==(const ProgressEntry&) const = default;
};
struct Project : PlanInput {
    std::string documentId;
    std::string derivedFrom;
    std::uint64_t revision = 1;
    std::vector<Baseline> baselines;
    std::vector<ProgressEntry> progress;
    std::optional<std::string> statusAt;
    bool operator==(const Project&) const = default;
};
// Domain uses ISO civil dates, without a UI locale or implicit timezone.
bool validDate(const std::string& date);
bool validStamp(const std::string& stamp);
bool validUtcStamp(const std::string& stamp);
void validatePlan(const PlanInput& plan);
void validate(const Project& project);
// Report-date selection, with later appended corrections winning a tie.
const ProgressEntry* progressAsOf(const Project& project, const std::string& taskId);
}
