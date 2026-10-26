#pragma once
#include "calculation/Costs.h"
#include "calculation/Resources.h"
#include "calculation/Tracking.h"
#include <variant>
namespace jobs {
enum class ReportFamily { All, Schedule, Resources, Costs };
enum class CellFormat { Text, Integer, Hours, WorkHours, Money, Units, Ratio, PhysicalPercent };
struct ReportCell {
    std::variant<std::monostate, std::string, std::int64_t, double> value;
    CellFormat format = CellFormat::Text;
};
struct ReportColumn { std::string title; int width = 18; };
struct ReportTable {
    std::string id, title, note;
    ReportFamily family = ReportFamily::All;
    std::vector<ReportColumn> columns;
    std::vector<std::vector<ReportCell>> rows;
};
struct ReportKpi { std::string id, title, unit, note; ReportCell value; };
struct ReportOptions {
    std::string wbsId, baselineId, formula = "budget-remaining-v1";
    std::optional<std::string> fromDate, toDate;
    std::string reportId, generatedAt;
    bool unsaved = false;
};
struct ProjectReport {
    std::string projectName, documentId, currency, scopeName;
    std::uint64_t revision = 0;
    ReportOptions options;
    std::optional<std::string> statusAt;
    CalculationStatus scheduleStatus = CalculationStatus::Incomplete;
    std::vector<ReportKpi> kpis;
    std::vector<ReportTable> tables;
    std::vector<CostDay> costTrend;
};
ProjectReport buildReport(const Project&, const ReportOptions&);
bool includeTable(const ReportTable&, ReportFamily);
}
