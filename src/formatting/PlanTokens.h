#pragma once
// Canonical wire/presentation tokens. No Qt, I/O or scheduling decisions here.
#include "domain/Project.h"
#include <array>
#include <string_view>
#include <utility>

namespace jobs {
namespace plantokens {
inline constexpr std::array dependency{
    std::pair{DependencyType::FS, std::string_view{"FS"}}, std::pair{DependencyType::SS, std::string_view{"SS"}},
    std::pair{DependencyType::FF, std::string_view{"FF"}}, std::pair{DependencyType::SF, std::string_view{"SF"}}};
inline constexpr std::array constraint{std::pair{ConstraintType::ASAP, std::string_view{"ASAP"}},
                                       std::pair{ConstraintType::ALAP, std::string_view{"ALAP"}},
                                       std::pair{ConstraintType::SNET, std::string_view{"SNET"}},
                                       std::pair{ConstraintType::SNLT, std::string_view{"SNLT"}},
                                       std::pair{ConstraintType::FNET, std::string_view{"FNET"}},
                                       std::pair{ConstraintType::FNLT, std::string_view{"FNLT"}},
                                       std::pair{ConstraintType::MSO, std::string_view{"MSO"}},
                                       std::pair{ConstraintType::MFO, std::string_view{"MFO"}}};
inline constexpr std::array task{std::pair{TaskType::FixedDuration, std::string_view{"fixed-duration"}},
                                 std::pair{TaskType::FixedWork, std::string_view{"fixed-work"}},
                                 std::pair{TaskType::FixedUnits, std::string_view{"fixed-units"}}};
template <class E, std::size_t N>
std::string encode(E value, const std::array<std::pair<E, std::string_view>, N> &table) {
    for (const auto &[key, text] : table)
        if (value == key)
            return std::string(text);
    throw AppError(ErrorCode::PLAN_ENUM_INVALID);
}
template <class E, std::size_t N>
E decode(std::string_view text, const std::array<std::pair<E, std::string_view>, N> &table, ErrorCode error) {
    for (const auto &[key, token] : table)
        if (token == text)
            return key;
    throw AppError(error);
}
} // namespace plantokens
inline std::string enumToken(DependencyType value) {
    return plantokens::encode(value, plantokens::dependency);
}
inline std::string enumToken(ConstraintType value) {
    return plantokens::encode(value, plantokens::constraint);
}
inline std::string enumToken(TaskType value) {
    return plantokens::encode(value, plantokens::task);
}
inline DependencyType dependencyTypeFromToken(std::string_view text) {
    return plantokens::decode(text, plantokens::dependency, ErrorCode::DEPENDENCY_RULE_INVALID);
}
inline ConstraintType constraintTypeFromToken(std::string_view text) {
    return plantokens::decode(text, plantokens::constraint, ErrorCode::CONSTRAINT_INPUT_INVALID);
}
inline TaskType taskTypeFromToken(std::string_view text) {
    return plantokens::decode(text, plantokens::task, ErrorCode::TASK_EFFORT_INVALID);
}
inline std::vector<std::string> dependencyLabels(const Task &task) {
    std::vector<std::string> out;
    for (const auto &d : dependencies(task))
        out.push_back(d.predecessorId + " " + enumToken(d.type) + (d.lagMinutes >= 0 ? "+" : "") +
                      std::to_string(d.lagMinutes) + "m");
    return out;
}

inline std::string taskTypeName(TaskType t) {
    if (!isValid(t))
        throw AppError(ErrorCode::TASK_TYPE_UNSUPPORTED, {});
    return t == TaskType::FixedWork ? "고정 공수" : t == TaskType::FixedUnits ? "고정 단위" : "고정 기간";
}
inline std::string constraintName(ConstraintType type) {
    if (!isValid(type))
        throw AppError(ErrorCode::CONSTRAINT_TYPE_UNSUPPORTED, {});
    if (type == ConstraintType::ASAP)
        return "가능한 빨리";
    if (type == ConstraintType::ALAP)
        return "가능한 늦게";
    if (type == ConstraintType::SNET)
        return "시작 이후";
    if (type == ConstraintType::SNLT)
        return "시작 이전";
    if (type == ConstraintType::FNET)
        return "완료 이후";
    if (type == ConstraintType::FNLT)
        return "완료 이전";
    if (type == ConstraintType::MSO)
        return "시작 고정";
    if (type == ConstraintType::MFO)
        return "완료 고정";
    return "알 수 없는 제약";
}
inline std::string taskCalendarName(const PlanInput &p, const Task &task) {
    for (const auto &c : p.calendars)
        if (c.id == task.calendarId)
            return c.name;
    return "프로젝트 기본 (월–금 09–12 / 13–18)";
}
} // namespace jobs
