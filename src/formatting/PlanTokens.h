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
    throw std::runtime_error("지원하지 않는 계획 enum 값입니다.");
}
template <class E, std::size_t N>
E decode(std::string_view text, const std::array<std::pair<E, std::string_view>, N> &table, const char *error) {
    for (const auto &[key, token] : table)
        if (token == text)
            return key;
    throw std::runtime_error(error);
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
    return plantokens::decode(text, plantokens::dependency, "선행관계 종류·근무분 지연/선행·중복 속성 오류입니다.");
}
inline ConstraintType constraintTypeFromToken(std::string_view text) {
    return plantokens::decode(text, plantokens::constraint,
                              "작업 제약 종류·제약 시각·마감일이 잘못되었습니다. 날짜는 YYYY-MM-DD HH:mm입니다.");
}
inline TaskType taskTypeFromToken(std::string_view text) {
    return plantokens::decode(text, plantokens::task, "작업 유형·계산 기준·공수 값이 일치하지 않습니다.");
}
inline std::vector<std::string> dependencyLabels(const Task &task) {
    std::vector<std::string> out;
    for (const auto &d : dependencies(task))
        out.push_back(d.predecessorId + " " + enumToken(d.type) + (d.lagMinutes >= 0 ? "+" : "") +
                      std::to_string(d.lagMinutes) + "m");
    return out;
}
} // namespace jobs
