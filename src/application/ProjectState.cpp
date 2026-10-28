#include "application/ProjectState.h"

namespace jobs {
DirtyState compareSavedInput(const Project& a, const ProjectState& saved) {
    if (!saved) return {};
    const auto& b = *saved;
    DirtyState result;
    result.aligned = a.tasks.size() == b.tasks.size();
    if (result.aligned) for (std::size_t i = 0; i < a.tasks.size(); ++i) {
        // Also used after synchronous commit: dirty bookkeeping must not unwind
        // a live edit. Worker callers check cancellation around this pure comparison.
        if (a.tasks[i].id != b.tasks[i].id) result.aligned = false;
        if (a.tasks[i] != b.tasks[i]) ++result.differentTasks;
    }
    // Revision is an audit sequence, not an input-content save point.
    result.other = !result.aligned ||
        std::tie(a.name, a.startDate, a.calendar, a.holidays, a.calculationProfile, a.wbsNodes, a.resources,
                 a.assignments, a.currency, a.costProfile, a.taskCosts, a.calendars, a.statusAt, a.documentId,
                 a.derivedFrom, a.baselines, a.progress, a.scenarios, a.costEntries) !=
        std::tie(b.name, b.startDate, b.calendar, b.holidays, b.calculationProfile, b.wbsNodes, b.resources,
                 b.assignments, b.currency, b.costProfile, b.taskCosts, b.calendars, b.statusAt, b.documentId,
                 b.derivedFrom, b.baselines, b.progress, b.scenarios, b.costEntries);
    return result;
}
}
