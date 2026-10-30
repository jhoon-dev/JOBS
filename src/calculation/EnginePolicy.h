#pragma once
#include "domain/Project.h"
namespace jobs {
// Thread-local replay policy, scoped to one immutable historical calculation.
class EnginePolicyScope {
    bool previous_;

public:
    explicit EnginePolicyScope(bool legacy);
    ~EnginePolicyScope();
    EnginePolicyScope(const EnginePolicyScope &) = delete;
    EnginePolicyScope &operator=(const EnginePolicyScope &) = delete;
};
bool legacyMilestones();
std::string calculationEngineFor(const PlanInput &);
std::string calculationLevelingPolicyFor(const PlanInput &);
} // namespace jobs
