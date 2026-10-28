#include "application/Document.h"
#include "calculation/Reports.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <iostream>
namespace {
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
QByteArray bytes(const std::string &path) {
    QFile f(QString::fromStdString(path));
    check(f.open(QIODevice::ReadOnly), "read failed");
    return f.readAll();
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("Integrated project", "2026-10-26", {"Design", "Review", "Build"});
        const auto a = doc.project()->tasks[0].id, b = doc.project()->tasks[1].id, c = doc.project()->tasks[2].id;
        const auto wbs = doc.addWbs("Engineering");
        for (int i = 0; i < 3; ++i)
            doc.assignWbs(i, wbs);
        doc.setDuration(0, 60);
        doc.setDuration(1, 60);
        doc.setDuration(2, 120);
        doc.setDependencies(1, {{a, "FS", 0}});
        const auto cal =
            doc.putCalendar({{}, "Weekday engineering", {1, 2, 3, 4, 5}, {{540, 720}, {780, 1080}}, {"2026-10-27"}});
        jobs::Resource resource;
        resource.name = "Engineer";
        resource.rates = {{"2026-10-26", 6000}};
        resource.calendarId = cal;
        const auto rid = doc.putResource(resource);
        for (const auto &id : {a, b, c})
            doc.setAssignments(id, {{id, rid, 100}});
        doc.setTaskEffort(0, "fixed-work", true, 60, 6000, {{"2026-10-26 09:30", "2026-10-26 10:30"}});
        doc.setTaskEffort(2, "fixed-units", false, 120, {}, {});
        doc.setTaskCost({a, 1000, "uniform", "physical"});
        doc.setTaskCost({b, 0, "uniform", "physical"});
        doc.setTaskCost({c, 0, "uniform", "physical"});
        auto plan = jobs::calculate(*doc.project());
        check(plan.status == jobs::CalculationStatus::Valid && plan.tasks[0].finish == "2026-10-26 11:00" &&
                  plan.tasks[1].start == "2026-10-26 11:00" && plan.projectFinish == "2026-10-26 12:00",
              "integrated plan dates");
        doc.applyCalculation(plan);
        const auto baseline = doc.captureBaseline("Approved", "PM", "Scope approved", "2026-10-26T00:00:00Z");
        const auto frozen = doc.project()->baselines;
        doc.setStatusAt("2026-10-26 10:00");
        jobs::ProgressEntry progress;
        progress.taskId = a;
        progress.actualStart = "2026-10-26 09:00";
        progress.actualDurationMinutes = 30;
        progress.remainingWorkUnitMinutes = 3000;
        progress.reporter = "Engineer";
        progress.reason = "Measured work";
        progress.recordedAt = "2026-10-26T01:00:00Z";
        doc.recordProgress(progress);
        for (const auto &id : {a, b, c}) {
            jobs::CostEntry e;
            e.taskId = id;
            e.actualMinor = id == a ? 4000 : id == b ? 0 : 6000;
            e.physicalBasisPoints = id == b ? 0 : 5000;
            e.reporter = "PM";
            e.reason = "Measured cost";
            e.recordedAt = "2026-10-26T01:00:00Z";
            doc.recordCost(e);
        }
        const auto facts = doc.project()->progress;
        const auto costs = doc.project()->costEntries;
        auto forecast = jobs::calculate(*doc.project());
        check(forecast.status == jobs::CalculationStatus::Valid && forecast.tasks[0].start == "2026-10-26 09:00" &&
                  forecast.tasks[0].finish == "2026-10-26 11:00",
              "forecast altered actual/work");
        auto comparison = jobs::compareToBaseline(*doc.project(), baseline);
        check(comparison.baselineFinish == plan.projectFinish &&
                  comparison.forecast.status == jobs::CalculationStatus::Valid,
              "comparison failed");
        auto cost = jobs::analyzeCosts(*doc.project(), baseline);
        check(cost.plan == 25000 && cost.evm.bac == 25000 && cost.evm.pv == 9500 && cost.evm.ev == 9500 &&
                  cost.evm.ac == 10000 && cost.evm.cv == -500 && cost.evm.sv == 0,
              "independent EVM expectation failed");
        auto leveled = jobs::levelResources(*doc.project(), "task-order", 20);
        check(leveled.leveled.status == jobs::CalculationStatus::Valid &&
                  leveled.after.status == jobs::CalculationStatus::Valid,
              "leveling failed");
        doc.saveScenario("Resource alternative", "task-order", 20, "2026-10-26T01:00:00Z", doc.project()->documentId,
                         doc.project()->revision);
        jobs::ReportOptions options;
        options.baselineId = baseline;
        options.reportId = "integrated-report";
        options.generatedAt = "2026-10-26T01:00:00Z";
        auto report = jobs::buildReport(*doc.project(), options);
        check(report.scheduleStatus == jobs::CalculationStatus::Valid && report.tables.size() >= 4 &&
                  !report.kpis.empty(),
              "report missing integrated data");
        doc.saveAs(dir.filePath("integrated.jobs").toStdString());
        const auto disk = bytes(doc.path());
        auto control = std::make_shared<jobs::CalculationControl>();
        control->cancel();
        doc.applyCalculation(jobs::calculate(*doc.project(), control));
        check(doc.calculation()->status == jobs::CalculationStatus::Cancelled && !doc.calculation()->projectFinish &&
                  doc.calculation()->tasks.empty() && bytes(doc.path()) == disk && !doc.dirty(),
              "cancel mutated saved file");
        check(doc.project()->baselines == frozen && doc.project()->progress == facts &&
                  doc.project()->costEntries == costs,
              "derived operations changed approved/actual facts");
        jobs::Document reopened(store);
        reopened.open(doc.path(), true);
        check(*reopened.project() == *doc.project() &&
                  jobs::calculate(*reopened.project()).projectFinish == forecast.projectFinish,
              "save/reopen mismatch");
        doc.renameTask(2, "Unsaved recovery edit");
        jobs::JsonRecoveryStore recovery(dir.filePath("recovery"));
        auto session = recovery.newSession();
        session->checkpoint(doc.recoverySnapshot("2026-10-26T01:00:01Z"));
        session.reset();
        auto scan = recovery.scan();
        check(scan.candidates.size() == 1 && scan.candidates[0].valid, "recovery candidate missing");
        auto claim = recovery.claim(scan.candidates[0]);
        jobs::Document recovered(store);
        recovered.recover(claim->snapshot);
        check(recovered.project()->tasks[2].name == "Unsaved recovery edit" &&
                  recovered.project()->baselines == frozen && recovered.project()->progress == facts &&
                  recovered.project()->costEntries == costs && bytes(doc.path()) == disk,
              "recovery lost facts or changed source");
        recovered.saveAs(dir.filePath("recovered.jobs").toStdString());
        check(store.load(recovered.path()).project == *recovered.project(), "recovery copy roundtrip");
        claim->session->retire();
        std::cout << "PASS: create/WBS/calendar/effort/split/resource/cost, "
                     "plan/baseline/actual/forecast/EVM/level/report, cancel/save/reopen/recovery\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
