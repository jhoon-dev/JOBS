#pragma once
#include "domain/Diagnostics.h"

namespace jobs {
enum class DependencyType { FS, SS, FF, SF };
enum class ConstraintType { ASAP, ALAP, SNET, SNLT, FNET, FNLT, MSO, MFO };
enum class TaskType { FixedDuration, FixedWork, FixedUnits };

constexpr bool isValid(DependencyType value) {
    switch (value) {
    case DependencyType::FS:
    case DependencyType::SS:
    case DependencyType::FF:
    case DependencyType::SF:
        return true;
    }
    return false;
}
constexpr bool isValid(ConstraintType value) {
    switch (value) {
    case ConstraintType::ASAP:
    case ConstraintType::ALAP:
    case ConstraintType::SNET:
    case ConstraintType::SNLT:
    case ConstraintType::FNET:
    case ConstraintType::FNLT:
    case ConstraintType::MSO:
    case ConstraintType::MFO:
        return true;
    }
    return false;
}
constexpr bool isValid(TaskType value) {
    switch (value) {
    case TaskType::FixedDuration:
    case TaskType::FixedWork:
    case TaskType::FixedUnits:
        return true;
    }
    return false;
}
constexpr bool dependencyFromFinish(DependencyType value) {
    if (!isValid(value))
        throw AppError(ErrorCode::DEPENDENCY_RULE_INVALID);
    return value == DependencyType::FS || value == DependencyType::FF;
}
constexpr bool dependencyToFinish(DependencyType value) {
    if (!isValid(value))
        throw AppError(ErrorCode::DEPENDENCY_RULE_INVALID);
    return value == DependencyType::FF || value == DependencyType::SF;
}
} // namespace jobs
