#pragma once
#include "calculation/Scheduler.h"
namespace jobs {
using Money = std::optional<std::int64_t>;
struct Evm {
    Money bac, pv, ev, ac, cv, sv, eac, etc;
    std::optional<double> cpi, spi;
};
struct CostTaskRow {
    std::string taskId, name, credit;
    Money plan;
    Evm evm;
    bool baselineMember = false;
    std::optional<int> physicalBasisPoints;
    std::vector<std::string> issues;
};
struct CostGroup { std::string id, name; Money plan, actual; Evm evm; };
struct CostDay { std::string date; Money plan, budget, pv, ev, ac, currentActual; };
struct CostAnalysis {
    std::string documentId, baselineId, currency, formula;
    std::uint64_t revision = 0;
    std::optional<std::string> statusAt;
    Money plan, actual;
    Evm evm;
    std::vector<CostTaskRow> tasks;
    std::vector<CostGroup> currentWbs, baselineWbs;
    std::vector<CostDay> days;
    std::vector<Diagnostic> diagnostics;
    std::size_t actualCoverage = 0, evCoverage = 0, baselineTasks = 0;
};
// All inputs and ledger amounts are integer minor units; no rate-based revaluation of AC.
CostAnalysis analyzeCosts(const Project&, const std::string& baselineId, const std::string& formula = "budget-remaining-v1",
                          const std::optional<std::vector<std::string>>& taskScope = std::nullopt);
}
