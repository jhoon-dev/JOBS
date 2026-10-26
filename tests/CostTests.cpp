#include "application/Document.h"
#include "calculation/Costs.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QCryptographicHash>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool c, const char* m) { if (!c) throw std::runtime_error(m); }
void rejects(const std::function<void()>& f) { bool bad = false; try { f(); } catch (const std::exception&) { bad = true; } check(bad, "expected cost rejection"); }
QByteArray read(const QString& path) { QFile f(path); check(f.open(QIODevice::ReadOnly), "read failed"); return f.readAll(); }
void write(const QString& path, const QByteArray& bytes) { QFile f(path); check(f.open(QIODevice::WriteOnly), "write failed"); check(f.write(bytes) == bytes.size(), "write incomplete"); }
QByteArray sign(QJsonObject root, QJsonObject payload) { root["payload"] = payload; root["sha256"] = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(payload).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()); return QJsonDocument(root).toJson(); }
jobs::CostEntry report(const std::string& id, jobs::Money ac, std::optional<int> percent) {
    jobs::CostEntry e; e.taskId = id; e.actualMinor = ac; e.physicalBasisPoints = percent; e.reporter = "PM"; e.reason = "invoice / measured work"; e.recordedAt = "2026-10-26T09:00:00Z"; return e;
}
QJsonObject oldPlan(QJsonObject p) {
    p.remove("costProfile"); p.remove("currency"); p.remove("taskCosts"); auto resources = p["resources"].toArray();
    for (int i = 0; i < resources.size(); ++i) { auto r = resources[i].toObject(); r.remove("rates"); resources[i] = r; } p["resources"] = resources; return p;
}
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir; jobs::JsonProjectStore store; jobs::Document doc(store); doc.create("F10", "2026-10-26", {"A"}); doc.setDuration(0, 960);
        auto id = doc.project()->tasks[0].id; doc.setTaskCost({id, 1000, "uniform", "physical"});
        auto baseline = doc.captureBaseline("cost approval", "PM", "budget", "2026-10-26T00:00:00Z");
        doc.setStatusAt("2026-10-26 18:00"); doc.recordCost(report(id, 600, 4000));
        auto r = jobs::analyzeCosts(*doc.project(), baseline, "cpi-v1");
        check(r.evm.bac == 1000 && r.evm.pv == 500 && r.evm.ev == 400 && r.evm.ac == 600 && r.evm.cv == -200 && r.evm.sv == -100 &&
            r.evm.cpi && std::abs(*r.evm.cpi - 2.0 / 3) < 1e-12 && r.evm.spi == 0.8 && r.evm.eac == 1500 && r.evm.etc == 900, "F10 values/formula failed");
        check(r.days.size() == 2 && r.days[0].plan == 500 && r.days[0].pv == 500 && r.days[0].ev == 400 && !r.days[1].ev && !r.days[1].ac, "timephased PV / no future actual failed");
        check(jobs::analyzeCosts(*doc.project(), baseline).evm.eac == 1200 && jobs::analyzeCosts(*doc.project(), baseline, "cpi-spi-v1").evm.eac == 1725, "EAC assumptions not separated");
        const auto first = doc.project()->costEntries[0]; const auto frozen = doc.project()->baselines[0];
        doc.setTaskCost({id, 2000, "start", "zero-hundred"}); r = jobs::analyzeCosts(*doc.project(), baseline);
        check(r.plan == 2000 && r.evm.bac == 1000 && r.evm.ev == 400 && r.tasks[0].credit == "physical" && doc.project()->baselines[0] == frozen, "current budget/credit edit revalued baseline/EV");
        doc.setStatusAt("2026-10-27 18:00"); doc.recordCost(report(id, 800, 6000));
        doc.setStatusAt("2026-10-26 09:00"); doc.recordCost(report(id, 100, 1000));
        doc.setStatusAt("2026-10-27 18:00"); check(jobs::analyzeCosts(*doc.project(), baseline).evm.ac == 800, "backdated append superseded later report");
        doc.setStatusAt("2026-10-26 18:00"); doc.recordCost(report(id, 500, 5000));
        check(doc.project()->costEntries[0] == first && jobs::analyzeCosts(*doc.project(), baseline).evm.ac == 500 && doc.project()->costEntries.back().previousEntryId == doc.project()->costEntries[2].id, "correction erased ledger or tie/asof selection failed");
        auto before = *doc.project(); rejects([&] { doc.recordCost(report(id, -1, 0)); }); rejects([&] { doc.recordCost(report(id, 0, 10001)); }); rejects([&] { doc.setCurrency("USD"); });
        rejects([&] { doc.setTaskCost({id, 0, "invalid", "physical"}); }); check(*doc.project() == before, "invalid financial edit changed document/revision");
        doc.recordCost(report(id, 0, 0)); r = jobs::analyzeCosts(*doc.project(), baseline, "cpi-v1");
        check(!r.evm.cpi && r.evm.spi == 0 && !r.evm.eac, "zero denominator produced a ratio/EAC");
        doc.recordCost(report(id, {}, {})); r = jobs::analyzeCosts(*doc.project(), baseline);
        check(!r.evm.ev && !r.evm.ac && !r.evm.cpi && !r.evm.eac && r.evCoverage == 0, "unknown cost/progress silently zero-filled");
        doc.recordCost(report(id, 600, 4000));
        doc.saveScenario("cost snapshot", "task-order", 260, "2026-10-26T00:00:00Z", doc.project()->documentId, doc.project()->revision);
        auto path = dir.filePath("costs α.jobs"); doc.saveAs(path.toStdString()); auto bytes = read(path);
        check(store.load(doc.path()).project == *doc.project() && store.load(doc.path()).project.costEntries[0] == first, "cost/credit/currency/snapshot roundtrip failed");
        jobs::Document clone(store); clone.open(doc.path(), true); rejects([&] { clone.recordCost(report(id, 1, 100)); }); clone.saveAs(dir.filePath("copy.jobs").toStdString());
        check(clone.project()->costEntries == doc.project()->costEntries && read(path) == bytes && clone.project()->documentId != doc.project()->documentId, "clone rewrote actual cost provenance");
        // Rate changes are effective dated; baseline and booked actuals never use a new rate.
        jobs::Document rates(store); rates.create("rates", "2026-10-26", {"R"}); rates.setDuration(0, 960); auto tid = rates.project()->tasks[0].id;
        rates.setTaskCost({tid, 20, "uniform", "physical"}); auto rid = rates.putResource({{}, "resource", "labor", 100, {}, {{"2026-10-26", 10}, {"2026-10-27", 20}}}); rates.setAssignments(tid, {{tid, rid, 100}});
        auto rb = rates.captureBaseline("rates", "PM", "approval", "2026-10-26T00:00:00Z"); rates.setStatusAt("2026-10-26 14:00"); rates.recordCost(report(tid, 123, 2500));
        r = jobs::analyzeCosts(*rates.project(), rb); check(r.plan == 260 && r.evm.bac == 260 && r.evm.pv == 45 && r.days[0].budget == 90 && r.days[1].budget == 170, "effective rates/daily accrual/intraday PV failed");
        auto resource = rates.project()->resources[0]; resource.rates[0].hourlyMinor = 100; rates.putResource(resource); r = jobs::analyzeCosts(*rates.project(), rb);
        check(r.plan == 980 && r.evm.bac == 260 && r.evm.ac == 123 && r.evm.ev == 65, "rate edit revalued AC/baseline or changed physical credit");
        resource.rates[1].hourlyMinor.reset(); rates.putResource(resource); auto unknownBaseline = rates.captureBaseline("unknown future", "PM", "test", "2026-10-26T00:00:00Z");
        r = jobs::analyzeCosts(*rates.project(), unknownBaseline); check(!r.evm.bac && r.evm.pv == 405 && !r.evm.ev && r.evm.ac == 123, "future unknown rate poisoned known PV prefix or produced BAC");
        resource.rates = {{"2026-10-26", std::nullopt}, {"2026-10-27", 20}}; rates.putResource(resource);
        r = jobs::analyzeCosts(*rates.project(), rb); check(!r.days[0].plan && r.days[1].plan == 170, "unknown earlier cost hid a known later daily amount");
        rates.setHolidays({"2026-10-27"}); resource.rates = {{"2026-10-26", 10}, {"2026-10-28", 30}}; rates.putResource(resource);
        check(jobs::analyzeCosts(*rates.project(), rb).plan == 340, "holiday / rate effective date not applied on workday");
        rates.saveAs(dir.filePath("rates.jobs").toStdString()); check(store.load(rates.path()).project == *rates.project(), "rate schedule did not persist");
        // Aggregate CPI must use summed money, with WBS descendants counted once.
        jobs::Document groups(store); groups.create("groups", "2026-10-26", {"one", "two"}); auto root = groups.addWbs("root"), child = groups.addWbs("child", root);
        for (int i = 0; i < 2; ++i) { groups.setDuration(i, 480); groups.assignWbs(i, i ? child : root); groups.setTaskCost({groups.project()->tasks[std::size_t(i)].id, 100, "uniform", "physical"}); }
        auto gb = groups.captureBaseline("groups", "PM", "approval", "2026-10-26T00:00:00Z"); groups.setStatusAt("2026-10-26 18:00");
        groups.recordCost(report(groups.project()->tasks[0].id, 25, 5000)); groups.recordCost(report(groups.project()->tasks[1].id, 200, 10000));
        r = jobs::analyzeCosts(*groups.project(), gb); check(r.evm.bac == 200 && r.evm.ev == 150 && r.evm.ac == 225 && r.evm.cpi && std::abs(*r.evm.cpi - 2.0 / 3) < 1e-12 && r.baselineWbs[0].evm.bac == 200 && r.baselineWbs[1].evm.bac == 100, "WBS duplicate / mean-of-ratios error");
        groups.addTask("outside baseline"); groups.setDuration(2, 0); auto added = groups.project()->tasks[2].id; groups.setTaskCost({added, 20, "start", "physical"}); groups.recordCost(report(added, 30, 10000));
        r = jobs::analyzeCosts(*groups.project(), gb); check(r.plan == 220 && r.actual == 255 && r.evm.ac == 225 && r.evm.bac == 200 && !r.tasks[2].baselineMember, "baseline cohort mixed added-task budget/AC");
        // Fixed-cost accrual and exact half-up rounding in minor currency units.
        jobs::Document rounding(store); rounding.create("round", "2026-10-26", {"tiny"}); rounding.setCurrency("USD"); rounding.setDuration(0, 490); auto tiny = rounding.project()->tasks[0].id;
        rounding.setTaskCost({tiny, 1, "uniform", "physical"}); auto rr = rounding.putResource({{}, "fraction", "labor", 100, {}, {{"2026-10-26", 3}}}); rounding.setAssignments(tiny, {{tiny, rr, 100}});
        auto bround = rounding.captureBaseline("round", "PM", "approval", "2026-10-26T00:00:00Z"); rounding.setStatusAt("2026-10-26 18:00"); rounding.recordCost(report(tiny, 0, 0));
        r = jobs::analyzeCosts(*rounding.project(), bround); check(r.plan == 26 && r.days[0].budget == 25 && r.days[1].budget == 1 && r.evm.pv == 25, "per-assignment/day half-up or fixed-cost conservation failed");
        jobs::Document milestone(store); milestone.create("milestone", "2026-10-26", {"acceptance"}); milestone.setDuration(0, 0); auto mid = milestone.project()->tasks[0].id; milestone.setTaskCost({mid, 100, "finish", "zero-hundred"});
        auto mb = milestone.captureBaseline("milestone", "PM", "approval", "2026-10-26T00:00:00Z"); milestone.setStatusAt("2026-10-26 09:00"); milestone.recordCost(report(mid, 0, 10000));
        check(!jobs::analyzeCosts(*milestone.project(), mb).evm.ev, "0/100 used physical % without completion evidence");
        jobs::ProgressEntry pe; pe.taskId = mid; pe.actualStart = pe.actualFinish = "2026-10-26 09:00"; pe.reporter = "PM"; pe.reason = "accepted"; pe.recordedAt = "2026-10-26T00:00:00Z"; milestone.recordProgress(pe);
        check(jobs::analyzeCosts(*milestone.project(), mb).evm.ev == 100 && jobs::analyzeCosts(*milestone.project(), mb).evm.pv == 100, "milestone fixed credit/PV not earned at finish");
        jobs::Document accrual(store); accrual.create("accrual", "2026-10-26", {"fixed"}); accrual.setDuration(0, 960); auto aid = accrual.project()->tasks[0].id;
        accrual.setTaskCost({aid, 1000, "start", "physical"}); auto startBase = accrual.captureBaseline("start", "PM", "test", "2026-10-26T00:00:00Z"); accrual.setStatusAt("2026-10-26 09:00");
        check(jobs::analyzeCosts(*accrual.project(), startBase).evm.pv == 1000, "start fixed cost not accrued at start");
        accrual.setTaskCost({aid, 1000, "finish", "physical"}); auto finishBase = accrual.captureBaseline("finish", "PM", "test", "2026-10-26T00:00:00Z"); accrual.setStatusAt("2026-10-26 18:00");
        check(jobs::analyzeCosts(*accrual.project(), finishBase).evm.pv == 0, "finish fixed cost accrued early"); accrual.setStatusAt("2026-10-27 18:00");
        check(jobs::analyzeCosts(*accrual.project(), finishBase).evm.pv == 1000, "finish fixed cost missing at completion boundary");
        jobs::Document foreign(store); foreign.create("foreign", "2026-10-26", {"task"}); foreign.setDuration(0, 480); auto fb = foreign.captureBaseline("old currency", "PM", "test", "2026-10-26T00:00:00Z");
        foreign.setCurrency("USD"); foreign.setTaskCost({foreign.project()->tasks[0].id, 100, "uniform", "physical"}); foreign.setStatusAt("2026-10-26 18:00"); foreign.recordCost(report(foreign.project()->tasks[0].id, 10, 5000));
        auto mismatch = jobs::analyzeCosts(*foreign.project(), fb); check(mismatch.plan == 100 && mismatch.actual == 10 && !mismatch.evm.bac && !mismatch.evm.ac && !mismatch.evm.cpi, "cross-currency EVM mixed monetary amounts");
        groups.setTaskCost({groups.project()->tasks[0].id, jobs::maxMoney, "uniform", "physical"}); groups.setTaskCost({groups.project()->tasks[1].id, jobs::maxMoney, "uniform", "physical"}); check(!jobs::analyzeCosts(*groups.project(), gb).plan, "money sum overflow silently wrapped");
        jobs::Document empty(store); empty.create("empty group", "2026-10-26", {"task"}); empty.setDuration(0, 480); empty.addWbs("empty"); empty.setTaskCost({empty.project()->tasks[0].id, 0, "uniform", "physical"});
        auto eb = empty.captureBaseline("empty", "PM", "test", "2026-10-26T00:00:00Z");
        auto er = jobs::analyzeCosts(*empty.project(), eb); check(er.evm.bac == 0 && !er.evm.pv && !er.evm.ev && !er.evm.ac && !er.evm.eac && !er.baselineWbs[0].evm.pv, "empty WBS produced status-less EVM zeros");
        // Genuine schema-5 input, including nested baseline and scenario, migrates without reinterpreting missing costs as 0.
        auto json = QJsonDocument::fromJson(bytes).object(), payload = json["payload"].toObject(); auto legacy = oldPlan(payload); legacy.remove("costEntries");
        for (const auto& field : {"baselines", "scenarios"}) { auto a = legacy[field].toArray(); for (int i = 0; i < a.size(); ++i) { auto o = a[i].toObject(); o["input"] = oldPlan(o["input"].toObject()); a[i] = o; } legacy[field] = a; }
        json["schemaVersion"] = 5; auto oldBytes = sign(json, legacy); auto oldPath = dir.filePath("v5.jobs"); write(oldPath, oldBytes); jobs::Document migrated(store); migrated.open(oldPath.toStdString());
        auto oldAnalysis = jobs::analyzeCosts(*migrated.project(), migrated.project()->baselines[0].id);
        check(migrated.requiresSaveAs() && migrated.project()->costEntries.empty() && migrated.project()->taskCosts.empty() && !oldAnalysis.evm.bac && !oldAnalysis.evm.pv, "schema-5 absent budgets became zero");
        rejects([&] { migrated.save(); }); migrated.saveAs(dir.filePath("v6.jobs").toStdString()); check(read(oldPath) == oldBytes && store.load(migrated.path()).project == *migrated.project(), "schema-5 original-preserving copy migration failed");
        json = QJsonDocument::fromJson(bytes).object(); auto bad = payload; auto entries = bad["costEntries"].toArray(); auto e = entries[0].toObject(); e["currency"] = "USD"; entries[0] = e; bad["costEntries"] = entries;
        auto badPath = dir.filePath("bad.jobs"); write(badPath, sign(json, bad)); rejects([&] { store.load(badPath.toStdString()); });
        bad = payload; entries = bad["costEntries"].toArray(); e = entries[0].toObject(); e["previousEntryId"] = "missing"; entries[0] = e; bad["costEntries"] = entries; write(badPath, sign(json, bad)); rejects([&] { store.load(badPath.toStdString()); });
        std::cout << "PASS: F10, EAC assumptions/zero denominator, asof/corrections, frozen budgets/rates/AC, effective dates/holidays, exact money rounding, WBS/cohort ratio, explicit physical/0-100 credit, overflow, persistence/provenance/schema5 migration\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
