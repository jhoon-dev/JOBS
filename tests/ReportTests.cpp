#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include "reporting/ReportExport.h"
#include <QApplication>
#include <QFile>
#include <QDir>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtEndian>
#include <functional>
#include <iostream>
#include <cmath>
namespace {
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
void rejects(const std::function<void()> &f) {
    bool bad = false;
    try {
        f();
    } catch (const std::exception &) {
        bad = true;
    }
    check(bad, "expected rejection");
}
const jobs::ReportTable &table(const jobs::ProjectReport &r, const std::string &id) {
    for (const auto &t : r.tables)
        if (t.id == id)
            return t;
    throw std::runtime_error("missing table");
}
const jobs::ReportCell &kpi(const jobs::ProjectReport &r, const std::string &id) {
    for (const auto &k : r.kpis)
        if (k.id == id)
            return k.value;
    throw std::runtime_error("missing KPI");
}
std::int64_t integer(const jobs::ReportCell &c) {
    return std::get<std::int64_t>(c.value);
}
bool unknown(const jobs::ReportCell &c) {
    return std::holds_alternative<std::monostate>(c.value);
}
QByteArray read(const QString &path) {
    QFile f(path);
    check(f.open(QIODevice::ReadOnly), "read failed");
    return f.readAll();
}
// Independent local-header/XML check. Python zipfile also verifies CRC and central directory in QA.
QByteArray sheetXml(const QByteArray &bytes, const QByteArray &wanted) {
    qsizetype at = 0;
    QByteArray found;
    while (at + 30 <= bytes.size() && qFromLittleEndian<quint32>(bytes.constData() + at) == 0x04034b50) {
        check(qFromLittleEndian<quint16>(bytes.constData() + at + 8) == 0, "ZIP method unsupported");
        auto size = qFromLittleEndian<quint32>(bytes.constData() + at + 18);
        auto names = qFromLittleEndian<quint16>(bytes.constData() + at + 26);
        auto extra = qFromLittleEndian<quint16>(bytes.constData() + at + 28);
        auto name = bytes.mid(at + 30, names);
        at += 30 + names + extra;
        check(at + size <= bytes.size(), "truncated ZIP");
        auto part = bytes.mid(at, size);
        at += size;
        QXmlStreamReader xml(part);
        while (!xml.atEnd())
            xml.readNext();
        check(!xml.hasError(), "invalid OOXML");
        if (name == wanted)
            found = part;
    }
    check(!found.isEmpty(), "missing OOXML part");
    return found;
}
} // namespace
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    try {
        QTemporaryDir temporary;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("시공 통합 대시보드", "2026-10-26", {"=HYPERLINK(\"https://example.test\")", "전기 공사"});
        doc.setCurrency("USD");
        auto wbs = doc.addWbs("토목 공사");
        doc.assignWbs(0, wbs);
        auto empty = doc.addWbs("비어 있는 단계");
        const auto one = doc.project()->tasks[0].id, two = doc.project()->tasks[1].id;
        doc.setDuration(0, 960);
        doc.setDuration(1, 960);
        doc.setTaskCost({one, 200, "uniform", "physical"});
        doc.setTaskCost({two, 400, "uniform", "physical"});
        auto resource = doc.putResource({{}, "현장 공유팀", "labor", 100, {}, {{"2026-10-26", 50}}});
        doc.setAssignments(one, {{one, resource, 100}});
        doc.setAssignments(two, {{two, resource, 100}});
        auto baseline = doc.captureBaseline("최초 승인 예산", "현장 PM", "원가 기준 승인", "2026-10-26T00:00:00Z");
        jobs::ReportOptions options;
        options.reportId = "report-contract-fixture";
        options.generatedAt = "2026-10-26T09:00:00Z";
        options.baselineId = baseline;
        options.unsaved = true;
        const auto original = *doc.project();
        auto report = jobs::buildReport(original, options);
        check(integer(kpi(report, "plan")) == 2200 && unknown(kpi(report, "actual")) &&
                  unknown(kpi(report, "completed")) && integer(kpi(report, "overload")) == 960,
              "whole project KPI mismatch");
        options.wbsId = wbs;
        report = jobs::buildReport(original, options);
        check(integer(kpi(report, "tasks")) == 1 && integer(kpi(report, "plan")) == 1000 &&
                  integer(kpi(report, "bac")) == 1000,
              "WBS cost/KPI scope mismatch");
        const auto &demand = table(report, "resourceLoad").rows;
        check(demand.size() == 2 && integer(demand[0][3]) == 48000 && integer(demand[0][4]) == 96000 &&
                  integer(demand[0][6]) == 200 && integer(kpi(report, "overload")) == 960,
              "shared resource conflict hidden by WBS");
        options.fromDate = options.toDate = "2026-10-27";
        auto filtered = jobs::buildReport(original, options);
        check(table(filtered, "resourceLoad").rows.size() == 1 && table(filtered, "costDays").rows.size() == 1 &&
                  integer(kpi(filtered, "overload")) == 480 && integer(kpi(filtered, "plan")) == 1000 &&
                  integer(kpi(filtered, "tasks")) == 1,
              "daily filter modified totals or excluded endpoint");
        options.fromDate.reset();
        options.toDate.reset();
        options.wbsId = empty;
        auto zero = jobs::buildReport(original, options);
        check(integer(kpi(zero, "tasks")) == 0 && integer(kpi(zero, "plan")) == 0 && integer(kpi(zero, "bac")) == 0 &&
                  table(zero, "costs").rows.empty(),
              "empty WBS became whole project");
        auto bad = options;
        bad.wbsId = "missing";
        rejects([&] { jobs::buildReport(original, bad); });
        bad = options;
        bad.fromDate = "2026-11-01";
        bad.toDate = "2026-10-01";
        rejects([&] { jobs::buildReport(original, bad); });
        options.wbsId = wbs;
        auto incomplete = original;
        incomplete.tasks[1].durationMinutes.reset();
        auto incompleteReport = jobs::buildReport(incomplete, options);
        check(unknown(kpi(incompleteReport, "finish")) && unknown(table(incompleteReport, "schedule").rows[0][4]) &&
                  !table(incompleteReport, "diagnostics").rows.empty(),
              "invalid global schedule reused stale dates");
        auto unbounded = original;
        unbounded.resources[0].maxUnits.reset();
        check(unknown(kpi(jobs::buildReport(unbounded, options), "overload")), "unknown capacity became zero overload");
        doc.setStatusAt("2026-10-26 18:00");
        auto cost = [&](const std::string &id, std::int64_t amount, int physical) {
            jobs::CostEntry e;
            e.taskId = id;
            e.actualMinor = amount;
            e.physicalBasisPoints = physical;
            e.reporter = "PM";
            e.reason = "청구/완료량 확인";
            e.recordedAt = "2026-10-26T09:00:00Z";
            doc.recordCost(e);
        };
        cost(one, 600, 4000);
        cost(two, 0, 0);
        options.formula = "cpi-v1";
        report = jobs::buildReport(*doc.project(), options);
        check(integer(kpi(report, "actual")) == 600 && integer(kpi(report, "ev")) == 400 &&
                  integer(kpi(report, "eac")) == 1500 &&
                  std::abs(std::get<double>(kpi(report, "cpi").value) - 2.0 / 3) < 1e-12,
              "F10 EVM mismatch");
        check(integer(table(report, "costDays").rows[0][6]) == 600 && unknown(table(report, "costDays").rows.back()[6]),
              "current daily AC/future unknown wrong");
        options.baselineId.clear();
        auto noBaseline = jobs::buildReport(*doc.project(), options);
        check(integer(kpi(noBaseline, "actual")) == 600 && unknown(kpi(noBaseline, "bac")) &&
                  integer(table(noBaseline, "costDays").rows[0][6]) == 600 &&
                  unknown(table(noBaseline, "costDays").rows[0][5]),
              "current actual depends on baseline");
        options.baselineId = baseline;
        options.wbsId.clear();
        report = jobs::buildReport(*doc.project(), options);
        const auto before = *doc.project();
        auto output = QDir::current().filePath("reports-smoke.xlsx");
        auto pdf = QDir::current().filePath("reports-smoke.pdf");
        QFile::remove(output);
        QFile::remove(pdf);
        jobs::exportXlsx(report, output);
        auto bytes = read(output);
        auto xml = sheetXml(bytes, "xl/worksheets/sheet2.xml");
        check(xml.contains("=HYPERLINK") && !xml.contains("<f>") && xml.contains("inlineStr") && xml.contains("frozen"),
              "text became executable formula / missing frozen headers");
        auto costsXml = sheetXml(bytes, "xl/worksheets/sheet8.xml");
        check(costsXml.contains("<v>6.00</v>") && costsXml.contains("<v>0.00</v>"),
              "numeric monetary values not stored as numbers");
        jobs::PdfOptions pdfOptions;
        pdfOptions.font = QApplication::font();
        const auto pages = jobs::exportPdf(report, pdf, jobs::ReportFamily::All, pdfOptions);
        check(pages >= 3 && read(pdf).startsWith("%PDF-"), "PDF absent/not paginated");
        rejects([&] { jobs::exportXlsx(report, output); });
        check(read(output) == bytes, "export overwrote existing XLSX");
        auto pdfBytes = read(pdf);
        rejects([&] { jobs::exportPdf(report, pdf); });
        check(read(pdf) == pdfBytes, "export overwrote PDF");
        rejects([&] { jobs::exportXlsx(report, temporary.filePath("wrong.jobs")); });
        rejects([&] { jobs::exportXlsx(report, temporary.filePath("missing/fail.xlsx")); });
        auto malformed = report;
        malformed.tables[0].rows[0].pop_back();
        rejects([&] { jobs::exportXlsx(malformed, temporary.filePath("bad.xlsx")); });
        check(!QFile::exists(temporary.filePath("bad.xlsx")), "failed export committed partial file");
        malformed = report;
        malformed.tables[0].rows[0][0].value = std::string(1, '\x01');
        rejects([&] { jobs::exportPdf(malformed, temporary.filePath("bad.pdf")); });
        malformed = report;
        malformed.tables[0].rows[0][0].value = std::string(33000, 'x');
        rejects([&] { jobs::exportXlsx(malformed, temporary.filePath("long.xlsx")); });
        check(!QFile::exists(temporary.filePath("long.xlsx")), "long cell truncated silently");
        auto longReport = report;
        longReport.tables.clear();
        jobs::ReportTable longTable{"long",
                                    "긴 행 나눔",
                                    "머리글과 마지막 내용을 보존하는 페이지 나눔 검증",
                                    jobs::ReportFamily::All,
                                    {{"ID", 12}, {"내용", 50}},
                                    {}};
        std::string words;
        for (int i = 0; i < 700; ++i)
            words += "Segment_" + std::to_string(i) + " ";
        words += "END_SENTINEL_739";
        longTable.rows.push_back({{std::string("LONG_ROW")}, {words}});
        longReport.tables.push_back(longTable);
        longReport.costTrend.clear();
        auto longPdf = QDir::current().filePath("reports-long-portrait.pdf");
        QFile::remove(longPdf);
        pdfOptions.landscape = false;
        pdfOptions.fontPoints = 10;
        check(jobs::exportPdf(longReport, longPdf, jobs::ReportFamily::All, pdfOptions) >= 3,
              "long row did not continue over pages");
        auto a3 = QDir::current().filePath("reports-a3.pdf");
        QFile::remove(a3);
        pdfOptions.a3 = true;
        pdfOptions.landscape = true;
        jobs::exportPdf(report, a3, jobs::ReportFamily::Schedule, pdfOptions);
        auto resourceFile = QDir::current().filePath("reports-resources.xlsx");
        QFile::remove(resourceFile);
        jobs::exportXlsx(report, resourceFile, jobs::ReportFamily::Resources);
        auto workbook = sheetXml(read(resourceFile), "xl/workbook.xml");
        check(workbook.contains("출처와 설정") && workbook.contains("자원 배정") && !workbook.contains("작업별 원가"),
              "family filter lost provenance or mixed tables");
        check(*doc.project() == before, "report/export changed original/history");
        auto pdm = *doc.project();
        pdm.calculationProfile = jobs::pdmProfile;
        jobs::TaskCalendar shift;
        shift.id = "afternoon";
        shift.name = "PDM Afternoon";
        shift.periods = {{780, 1020}};
        pdm.calendars = {shift};
        pdm.tasks[1].calendarId = shift.id;
        pdm.tasks[1].predecessors = {one};
        pdm.tasks[1].dependencyRules = {{one, jobs::DependencyType::SS, -60}};
        auto pdmOptions = options;
        pdmOptions.wbsId.clear();
        pdmOptions.fromDate.reset();
        pdmOptions.toDate.reset();
        pdmOptions.reportId = "pdm-report-contract";
        auto pdmReport = jobs::buildReport(pdm, pdmOptions);
        auto pdmXlsx = QDir::current().filePath("reports-pdm.xlsx"),
             pdmPdf = QDir::current().filePath("reports-pdm.pdf");
        QFile::remove(pdmXlsx);
        QFile::remove(pdmPdf);
        jobs::exportXlsx(pdmReport, pdmXlsx);
        pdfOptions.a3 = false;
        jobs::exportPdf(pdmReport, pdmPdf, jobs::ReportFamily::Schedule, pdfOptions);
        auto constrained = pdm;
        constrained.calculationProfile = jobs::constraintProfile;
        constrained.tasks[0].constraintType = jobs::ConstraintType::SNET;
        constrained.tasks[0].constraintAt = "2026-10-27 09:00";
        constrained.tasks[0].deadlineAt = "2026-10-27 17:00";
        auto timingOptions = pdmOptions;
        timingOptions.reportId = "constraints-report-contract";
        auto timingReport = jobs::buildReport(constrained, timingOptions);
        auto timingXlsx = QDir::current().filePath("reports-constraints.xlsx"),
             timingPdf = QDir::current().filePath("reports-constraints.pdf");
        QFile::remove(timingXlsx);
        QFile::remove(timingPdf);
        jobs::exportXlsx(timingReport, timingXlsx);
        jobs::exportPdf(timingReport, timingPdf, jobs::ReportFamily::Schedule, pdfOptions);
        jobs::Project effort;
        effort.documentId = "effort-export";
        effort.name = "공수와 분할 출력";
        effort.startDate = "2026-10-26";
        effort.calculationProfile = jobs::effortProfile;
        effort.tasks = {{"design", "Effort design", {}, {}}, {"review", "Review", 120, {"design"}}};
        auto &design = effort.tasks[0];
        design.taskType = jobs::TaskType::FixedWork;
        design.workDriven = true;
        design.workUnitMinutes = 48000;
        design.breaks = {{"2026-10-26 13:00", "2026-10-27 13:00"}};
        effort.calendars = {
            {"designer-cal", "Designer Tuesday off", {1, 2, 3, 4, 5}, {{540, 720}, {780, 1080}}, {"2026-10-27"}}};
        effort.resources = {{"designer", "Designer", "labor", 100, {}, {}}};
        effort.resources[0].calendarId = "designer-cal";
        effort.resources[0].rates = {{"2026-10-26", 6000}};
        effort.assignments = {{"design", "designer", 100}};
        effort.taskCosts = {{"design", 0, "uniform", "physical"}, {"review", 0, "uniform", "physical"}};
        auto effortOptions = options;
        effortOptions.baselineId.clear();
        effortOptions.wbsId.clear();
        effortOptions.reportId = "effort-report-contract";
        effortOptions.fromDate.reset();
        effortOptions.toDate.reset();
        auto effortReport = jobs::buildReport(effort, effortOptions);
        auto effortXlsx = QDir::current().filePath("reports-effort.xlsx"),
             effortPdf = QDir::current().filePath("reports-effort.pdf");
        QFile::remove(effortXlsx);
        QFile::remove(effortPdf);
        jobs::exportXlsx(effortReport, effortXlsx);
        jobs::exportPdf(effortReport, effortPdf, jobs::ReportFamily::All, pdfOptions);
        std::cout << "PASS: snapshot/WBS/shared resources/empty scope/filters/unknown/stale inputs/F10/no-baseline "
                     "AC/typed OOXML/injection/atomic create-only/PDF pagination; "
                  << pages << " PDF pages\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
