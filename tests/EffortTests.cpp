#include "application/Document.h"
#include "calculation/Effort.h"
#include "calculation/Costs.h"
#include "calculation/Tracking.h"
#include "calculation/Reports.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <iostream>
namespace {
void check(bool b, const char *text) {
    if (!b)
        throw std::runtime_error(text);
}
template <class F> void rejects(F f) {
    bool bad = false;
    try {
        f();
    } catch (const std::exception &) {
        bad = true;
    }
    check(bad, "expected rejection");
}
jobs::Project sample() {
    jobs::Project p;
    p.documentId = "effort";
    p.name = "공수 시험";
    p.startDate = "2026-10-26";
    p.calculationProfile = jobs::effortProfile;
    p.tasks = {{"A", "설계", 480, {}}};
    p.resources = {{"R", "설계자", "labor", 100, {}, {}}};
    p.resources[0].rates = {{"2026-10-26", 6000}};
    p.assignments = {{"A", "R", 100}};
    p.taskCosts = {{"A", 0, "uniform", "physical"}};
    return p;
}
void work(jobs::Task &t, std::int64_t n) {
    t.taskType = "fixed-work";
    t.workDriven = true;
    t.workUnitMinutes = n;
}
jobs::TaskSchedule row(const jobs::Project &p, std::size_t i = 0) {
    auto r = jobs::calculatePlan(p);
    if (r.status != jobs::CalculationStatus::Valid) {
        for (const auto &d : r.diagnostics)
            std::cerr << d.code << " " << d.message << '\n';
        throw std::runtime_error("expected valid effort schedule");
    }
    return r.tasks.at(i);
}
std::int64_t loadWork(const jobs::Project &p, const jobs::ScheduleResult &schedule) {
    std::int64_t result = 0;
    for (const auto &d : jobs::resourceLoad(p, schedule).days)
        result += d.demandUnitMinutes;
    return result;
}
QByteArray read(const QString &path) {
    QFile f(path);
    check(f.open(QIODevice::ReadOnly), "read failed");
    return f.readAll();
}
void write(const QString &path, const QJsonObject &object) {
    QFile f(path);
    check(f.open(QIODevice::WriteOnly), "write failed");
    f.write(QJsonDocument(object).toJson());
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        auto p = sample();
        check(row(p).finish == "2026-10-26 18:00", "fixed duration dates");
        p.assignments[0].units = 50;
        auto r = row(p);
        check(r.finish == "2026-10-26 18:00" && jobs::TaskExecution(p, p.tasks[0]).work(r) == 24000,
              "fixed duration did not retain D");
        work(p.tasks[0], 48000);
        r = row(p);
        check(r.finish == "2026-10-27 18:00" && jobs::TaskExecution(p, p.tasks[0]).duration(r) == 960,
              "fixed work 50% did not double D");
        p.assignments[0].units = 200;
        check(row(p).finish == "2026-10-26 14:00", "fixed work 200% did not halve D");
        p = sample();
        jobs::changeWork(p, 0, 24000);
        check(p.assignments[0].units == 50 && row(p).finish == "2026-10-26 18:00",
              "fixed duration work edit did not solve U");
        p = sample();
        work(p.tasks[0], 48000);
        jobs::changeDuration(p, 0, 960);
        check(p.assignments[0].units == 50 && p.tasks[0].workUnitMinutes == 48000 &&
                  row(p).finish == "2026-10-27 18:00",
              "fixed work duration edit did not solve U");
        p = sample();
        p.tasks[0].taskType = "fixed-units";
        jobs::changeWork(p, 0, 96000);
        check(p.assignments[0].units == 100 && row(p).finish == "2026-10-27 18:00", "fixed units work edit changed U");
        jobs::changeDuration(p, 0, 240);
        check(!p.tasks[0].workDriven && p.assignments[0].units == 100 &&
                  jobs::TaskExecution(p, p.tasks[0]).work(row(p)) == 24000,
              "fixed units duration edit did not solve W");
        // Heterogeneous resource calendars: 3 morning + 4 afternoon resource-hours/day.
        p = sample();
        work(p.tasks[0], 48000);
        p.calendars = {{"am", "오전", {1, 2, 3, 4, 5}, {{540, 720}}, {}},
                       {"pm", "오후", {1, 2, 3, 4, 5}, {{780, 1020}}, {}}};
        p.resources[0].calendarId = "am";
        auto second = p.resources[0];
        second.id = "S";
        second.calendarId = "pm";
        p.resources.push_back(second);
        p.assignments.push_back({"A", "S", 100});
        r = row(p);
        check(r.finish == "2026-10-27 10:00" && loadWork(p, jobs::calculate(p)) == 48000,
              "resource calendar intersection/integral wrong");
        check(jobs::analyzeCosts(p, {}).plan == 48000, "cost did not use resource work");
        p.calendars[0].holidays = {"2026-10-26"};
        check(row(p).start == "2026-10-26 13:00" && row(p).finish == "2026-10-27 14:00",
              "resource holiday was ignored");
        p.tasks[0].calendarId = "am";
        check(row(p).start == "2026-10-27 09:00" && row(p).finish == "2026-10-29 11:00",
              "task/resource intersection used union");
        auto noOverlap = p;
        noOverlap.assignments.erase(noOverlap.assignments.begin());
        check(jobs::calculatePlan(noOverlap).status == jobs::CalculationStatus::Conflict,
              "disjoint calendars accepted");
        p = sample();
        work(p.tasks[0], 48000);
        p.tasks[0].breaks = {{"2026-10-26 13:00", "2026-10-27 13:00"}};
        r = row(p);
        check(r.finish == "2026-10-27 18:00" && jobs::TaskExecution(p, p.tasks[0]).duration(r) == 480,
              "split counted pause as duration/work");
        auto load = jobs::resourceLoad(p, jobs::calculate(p));
        check(load.days.size() == 2 && load.days[0].demandUnitMinutes == 18000 &&
                  load.days[1].demandUnitMinutes == 30000,
              "split resource demand not clipped");
        check(jobs::analyzeCosts(p, {}).plan == 48000, "split cost accrued gap");
        p.tasks.push_back({"B", "중단 중 다른 작업", 240, {}});
        p.assignments.push_back({"B", "R", 100});
        p.tasks[1].constraintType = "SNET";
        p.tasks[1].constraintAt = "2026-10-26 13:00";
        auto level = jobs::levelResources(p, "task-order", 20);
        check(level.leveled.status == jobs::CalculationStatus::Valid &&
                  level.after.status == jobs::CalculationStatus::Valid &&
                  level.leveled.tasks[1].start == "2026-10-26 13:00",
              "leveling reserved split gap");
        // Final minute has 101 unit-minutes, conserved across two simultaneous resources.
        p = sample();
        work(p.tasks[0], 101);
        second = p.resources[0];
        second.id = "S";
        p.resources.push_back(second);
        p.assignments.push_back({"A", "S", 100});
        r = row(p);
        check(r.finish == "2026-10-26 09:01" && jobs::TaskExecution(p, p.tasks[0]).work(r) == 101 &&
                  loadWork(p, jobs::calculate(p)) == 101 && jobs::analyzeCosts(p, {}).plan == 101,
              "fractional final minute fabricated work/cost");
        auto spans = jobs::executionSpans(p, p.tasks[0], r);
        check(spans.size() == 2 && spans[0].units == 51 && spans[1].units == 50,
              "last minute allocation not deterministic");
        p = sample();
        work(p.tasks[0], 48000);
        p.tasks.push_back({"B", "후행", 60, {"A"}});
        p.tasks[1].dependencyRules = {{"A", "FF", 60}};
        check(row(p, 1).finish == "2026-10-27 10:00", "FF finish inverse ignored work model");
        p.tasks[1].dependencyRules = {{"A", "SS", 60}};
        check(row(p, 1).start == "2026-10-26 10:00", "SS lag ignored");
        p.tasks[1].dependencyRules = {{"A", "SF", 60}};
        check(row(p, 1).finish == "2026-10-26 10:00", "SF work inverse ignored");
        p.tasks[1].dependencyRules = {{"A", "FS", -60}};
        check(row(p, 1).start == "2026-10-26 17:00", "FS lead ignored");
        p = sample();
        work(p.tasks[0], 48000);
        p.tasks[0].constraintType = "MFO";
        p.tasks[0].constraintAt = "2026-10-27 14:00";
        r = row(p);
        check(r.start == "2026-10-26 14:00" && r.finish == "2026-10-27 14:00", "fixed finish work inverse wrong");
        p.tasks[0].constraintAt = "2026-10-27 12:30";
        check(jobs::calculatePlan(p).status == jobs::CalculationStatus::Conflict,
              "nonworking exact finish silently snapped");
        p = sample();
        work(p.tasks[0], 24000);
        p.tasks.push_back({"B", "긴 작업", 960, {}});
        p.tasks[0].constraintType = "ALAP";
        r = row(p);
        check(r.start == "2026-10-27 14:00" && r.finish == "2026-10-27 18:00" && r.totalFloatMinutes == 0,
              "work ALAP not late/critical");
        auto lateLevel = jobs::levelResources(p, "task-order", 20);
        check(lateLevel.leveled.tasks[0].start == r.start, "leveling advanced ALAP before original");
        auto chain = sample();
        work(chain.tasks[0], 12000);
        chain.tasks[0].constraintType = "ALAP";
        chain.tasks.push_back({"B", "늦은 후행", 120, {"A"}});
        chain.tasks[1].constraintType = "ALAP";
        chain.tasks.push_back({"C", "긴 작업", 960, {}});
        auto chainPlan = jobs::calculatePlan(chain);
        check(chainPlan.status == jobs::CalculationStatus::Valid && chainPlan.tasks[1].start == "2026-10-27 16:00" &&
                  chainPlan.tasks[1].drivingPredecessors == std::vector<std::string>{"A"},
              "ALAP lost driving chain");
        auto hard = sample();
        work(hard.tasks[0], 48000);
        hard.tasks.push_back({"B", "시작 상한", 60, {"A"}});
        hard.tasks[1].constraintType = "SNLT";
        hard.tasks[1].constraintAt = "2026-10-26 17:00";
        auto causal = jobs::calculatePlan(hard);
        check(causal.status == jobs::CalculationStatus::Conflict && causal.diagnostics[0].taskIds.size() == 2 &&
                  !causal.projectFinish,
              "effort constraint lost predecessor cause");
        p = sample();
        work(p.tasks[0], 48000);
        p.tasks.push_back({"B", "공유", 480, {}});
        p.assignments.push_back({"B", "R", 100});
        level = jobs::levelResources(p, "task-order", 20);
        check(level.leveled.status == jobs::CalculationStatus::Valid &&
                  level.leveled.tasks[1].start == "2026-10-27 09:00" &&
                  level.after.status == jobs::CalculationStatus::Valid,
              "work leveling did not release resource");
        p.tasks[1].constraintType = "MSO";
        p.tasks[1].constraintAt = "2026-10-26 09:00";
        check(jobs::levelResources(p, "task-order", 20).leveled.status == jobs::CalculationStatus::Conflict,
              "work leveling overrode hard start");
        auto bad = sample();
        bad.tasks[0].breaks = {{"2026-10-27 10:00", "2026-10-27 09:00"}};
        rejects([&] { jobs::validate(bad); });
        bad = sample();
        bad.resources[0].calendarId = "missing";
        rejects([&] { jobs::validate(bad); });
        bad = sample();
        work(bad.tasks[0], -1);
        rejects([&] { jobs::validate(bad); });
        bad = sample();
        work(bad.tasks[0], 48000);
        bad.assignments.clear();
        check(jobs::calculatePlan(bad).status == jobs::CalculationStatus::Incomplete,
              "unassigned work treated as zero duration");
        // Persistence, frozen EVM and explicit remaining work across edits and recovery.
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("입력 이력", "2026-10-26", {"A"});
        doc.setDuration(0, 480);
        const auto id = doc.project()->tasks[0].id;
        auto resource = sample().resources[0];
        resource.id.clear();
        auto rid = doc.putResource(resource);
        doc.setAssignments(id, {{id, rid, 100}});
        doc.setTaskCost({id, 0, "uniform", "physical"});
        doc.setTaskEffort(0, "fixed-work", true, {}, 48000, {{"2026-10-26 13:00", "2026-10-27 13:00"}});
        doc.captureBaseline("공수 승인", "PM", "승인", "2026-10-26T09:00:00Z");
        const auto baseline = doc.project()->baselines[0];
        doc.setAssignments(id, {{id, rid, 200}});
        check(jobs::calculatePlan(*doc.project()).projectFinish == "2026-10-27 14:00" &&
                  doc.project()->baselines[0] == baseline,
              "unit edit changed approved baseline");
        doc.undo();
        check(doc.project()->assignments[0].units == 100 && doc.project()->tasks[0].workUnitMinutes == 48000,
              "work edit undo changed work");
        doc.redo();
        doc.setAssignments(id, {{id, rid, 100}});
        doc.setTaskTiming(0, "ASAP", {}, "2026-10-27 18:00");
        check(jobs::isEffort(*doc.project()), "timing edit downgraded effort profile");
        doc.saveScenario("공수 대안", "shortest-first", 20, "2026-10-26T09:00:00Z", doc.project()->documentId,
                         doc.project()->revision);
        const auto &scenario = doc.project()->scenarios.back();
        check(scenario.policy == "serial-effort-split-v1" &&
                  jobs::levelResources(jobs::scenarioProject(scenario), scenario.priority, scenario.horizonWorkDays)
                          .leveled.status == jobs::CalculationStatus::Valid,
              "effort scenario replay failed");
        doc.setStatusAt("2026-10-26 18:00");
        jobs::ProgressEntry entry;
        entry.taskId = id;
        entry.actualStart = "2026-10-26 09:00";
        entry.actualDurationMinutes = 180;
        entry.remainingMinutes = 300;
        entry.reporter = "PM";
        entry.reason = "진척";
        entry.recordedAt = "2026-10-26T09:00:00Z";
        doc.recordProgress(entry);
        check(jobs::calculate(*doc.project()).status == jobs::CalculationStatus::Incomplete,
              "remaining effort silently inferred from duration");
        entry.remainingWorkUnitMinutes = 30000;
        doc.recordProgress(entry);
        auto forecast = jobs::calculate(*doc.project());
        check(forecast.status == jobs::CalculationStatus::Valid && forecast.tasks[0].start == "2026-10-26 09:00" &&
                  forecast.tasks[0].remainingStart == "2026-10-27 13:00" &&
                  forecast.tasks[0].finish == "2026-10-27 18:00" && loadWork(*doc.project(), forecast) == 30000,
              "remaining work/actual preservation failed");
        auto costs = jobs::analyzeCosts(*doc.project(), baseline.id);
        check(costs.evm.bac == 48000 && costs.evm.pv == 18000, "frozen BAC/PV changed with split or units");
        auto file = dir.filePath("공수.jobs");
        doc.saveAs(file.toStdString());
        check(store.load(file.toStdString()).project == *doc.project(),
              "schema9 did not roundtrip work/baseline/remaining");
        doc.setTaskEffort(0, "fixed-work", true, {}, 60000, {});
        jobs::JsonRecoveryStore recovery(dir.filePath("recovery"));
        auto session = recovery.newSession();
        session->checkpoint(doc.recoverySnapshot("2026-10-26T09:01:00Z"));
        session.reset();
        auto scan = recovery.scan();
        auto claim = recovery.claim(scan.candidates[0]);
        check(claim->snapshot.project == *doc.project(), "recovery lost effort");
        claim.reset();
        auto root = QJsonDocument::fromJson(read(file)).object();
        auto payload = root["payload"].toObject();
        auto tasks = payload["tasks"].toArray();
        auto task = tasks[0].toObject();
        task["workUnitMinutes"] = -100;
        tasks[0] = task;
        payload["tasks"] = tasks;
        root["payload"] = payload;
        root["sha256"] = QString::fromLatin1(
            QCryptographicHash::hash(QJsonDocument(payload).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256)
                .toHex());
        write(dir.filePath("bad.jobs"), root);
        rejects([&] { store.load(dir.filePath("bad.jobs").toStdString()); });
        jobs::Document legacy(store);
        legacy.create("schema8", "2026-10-26", {"old"});
        legacy.setDuration(0, 480);
        legacy.setTaskTiming(0, "SNET", "2026-10-27 09:00", {});
        auto oldFile = dir.filePath("v8.jobs");
        legacy.saveAs(oldFile.toStdString());
        auto old = QJsonDocument::fromJson(read(oldFile)).object();
        old["schemaVersion"] = 8;
        write(oldFile, old);
        const auto oldBytes = read(oldFile);
        jobs::Document migrate(store);
        migrate.open(oldFile.toStdString(), true);
        check(migrate.requiresSaveAs() && jobs::calculatePlan(*migrate.project()).engineVersion == "jobs-pdm-1.1",
              "schema8 engine was reinterpreted");
        migrate.saveAs(dir.filePath("v9.jobs").toStdString());
        check(read(oldFile) == oldBytes, "schema8 migration overwrote original");
        jobs::ReportOptions options;
        options.reportId = "effort";
        options.generatedAt = "2026-10-26T09:00:00Z";
        auto report = jobs::buildReport(*doc.project(), options);
        bool found = false;
        for (const auto &t : report.tables)
            found = found || t.id == "executionWork";
        check(found && report.tables.size() == 19, "work report missing new tables");
        jobs::Document drag(store);
        drag.create("공수 드래그", "2026-10-26", {"A"});
        const auto dragId = drag.project()->tasks[0].id;
        auto dragResource = resource;
        dragResource.id.clear();
        const auto dragRid = drag.putResource(dragResource);
        drag.setAssignments(dragId, {{dragId, dragRid, 100}});
        drag.setTaskEffort(0, "fixed-work", true, {}, 48000, {});
        drag.applyCalculation(jobs::calculate(*drag.project()));
        drag.editGantt(drag.project()->documentId, drag.project()->revision, dragId, true, "2026-10-27 18:00");
        check(drag.project()->assignments[0].units == 50 && drag.project()->tasks[0].workUnitMinutes == 48000,
              "work resize changed effort instead of units");
        drag.undo();
        drag.applyCalculation(jobs::calculate(*drag.project()));
        drag.editGantt(drag.project()->documentId, drag.project()->revision, dragId, false, "2026-10-28 09:00");
        check(jobs::isEffort(*drag.project()) &&
                  jobs::calculatePlan(*drag.project()).tasks[0].finish == "2026-10-28 18:00",
              "work move lost basis or dereferenced absent nominal duration");
        std::cout
            << "PASS: work/type edit matrix, heterogeneous calendars, splits, exact final minute, "
               "PDM/constraints/ALAP, resource leveling, frozen costs/PV, explicit remaining, undo/schema9/recovery\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
