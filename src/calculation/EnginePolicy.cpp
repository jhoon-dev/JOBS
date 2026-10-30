#include "calculation/EnginePolicy.h"
namespace jobs {
namespace {
thread_local bool legacy = false;
}
EnginePolicyScope::EnginePolicyScope(bool value) : previous_(legacy) {
    legacy = value;
}
EnginePolicyScope::~EnginePolicyScope() {
    legacy = previous_;
}
bool legacyMilestones() {
    return legacy;
}
std::string calculationEngineFor(const PlanInput &p) {
    return legacy ? legacyEngineFor(p) : engineFor(p);
}
std::string calculationLevelingPolicyFor(const PlanInput &p) {
    return legacy ? legacyLevelingPolicyFor(p) : levelingPolicyFor(p);
}
} // namespace jobs
