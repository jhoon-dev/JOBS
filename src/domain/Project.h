#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

namespace jobs {
struct Task {
    std::string id;
    std::string name;
    std::optional<std::int64_t> durationMinutes;
    std::vector<std::string> predecessors;
    bool operator==(const Task&) const = default;
};
struct Project {
    std::string documentId;
    std::string derivedFrom;
    std::uint64_t revision = 1;
    std::string name;
    std::string startDate;
    std::string calendar = "standard-mon-fri-8h-v1";
    std::vector<Task> tasks;
    std::vector<std::string> holidays;
    std::string calculationProfile = "native-fs0-workminutes-v1";
    bool operator==(const Project&) const = default;
};
// Domain uses ISO civil dates, without a UI locale or implicit timezone.
bool validDate(const std::string& date);
void validate(const Project& project);
}
