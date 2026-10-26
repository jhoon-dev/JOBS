#include "application/Document.h"
#include "calculation/Resources.h"
#include "calculation/WorkCalendar.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCryptographicHash>
#include <algorithm>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool c, const char* message) { if (!c) throw std::runtime_error(message); }
void rejects(const std::function<void()>& f) { bool bad = false; try { f(); } catch (const std::exception&) { bad = true; } check(bad, "expected resource rejection"); }
jobs::Project project() {
    jobs::Project p; p.documentId = "resources"; p.name = "F05"; p.startDate = "2026-10-26";
    p.tasks = {{"a", "A", 480, {}}, {"b", "B", 480, {}}}; p.resources = {{"r", "팀", "labor", 100, {}}};
    p.assignments = {{"a", "r", 100}, {"b", "r", 100}}; return p;
}
std::int64_t work(const jobs::ResourceLoad& l) { std::int64_t n = 0; for (const auto& d : l.days) n += d.demandUnitMinutes; return n; }
QByteArray read(const QString& path) { QFile f(path); check(f.open(QIODevice::ReadOnly), "read failed"); return f.readAll(); }
void write(const QString& path, const QByteArray& bytes) { QFile f(path); check(f.open(QIODevice::WriteOnly), "write failed"); check(f.write(bytes) == bytes.size(), "write incomplete"); }
QByteArray sign(QJsonObject root, QJsonObject payload) {
    root["payload"] = payload; root["sha256"] = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(payload).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()); return QJsonDocument(root).toJson();
}
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        auto p = project(); const auto input = p;
        auto r = jobs::levelResources(p, "task-order", 260);
        check(r.before.status == jobs::CalculationStatus::Conflict && r.before.days.size() == 1 && r.before.days[0].peakUnits == 200 &&
            r.before.days[0].overloadMinutes == 480 && work(r.before) == 96000, "F05 peak200/work16h mismatch");
        check(r.leveled.status == jobs::CalculationStatus::Valid && r.after.status == jobs::CalculationStatus::Valid &&
            r.leveled.tasks[0].start == "2026-10-26 09:00" && r.leveled.tasks[1].start == "2026-10-27 09:00" &&
            r.leveled.projectFinish == "2026-10-27 18:00" && work(r.after) == work(r.before) && p == input, "F05 leveling did not preserve original/work/duration");
        check(jobs::levelResources(p, "task-order", 260).leveled.tasks == r.leveled.tasks, "nondeterministic leveling");
        p.tasks[0].durationMinutes = p.tasks[1].durationMinutes = 20; r = jobs::levelResources(p, "task-order", 10);
        check(r.before.days[0].overloadMinutes == 20 && r.before.days[0].peakUnits == 200 && r.before.days[0].demandUnitMinutes == 4000 &&
            r.after.days[0].overloadMinutes == 0 && r.leveled.tasks[1].start == "2026-10-26 09:20", "daily averaging concealed short overlap");
        p = project(); p.assignments[0].units = p.assignments[1].units = 50; r = jobs::levelResources(p, "task-order", 10);
        check(r.before.status == jobs::CalculationStatus::Valid && r.leveled.tasks == jobs::levelResources(p, "shortest-first", 10).leveled.tasks &&
            r.leveled.projectFinish == "2026-10-26 18:00", "fractional assignments falsely delayed");
        p = project(); p.resources[0].maxUnits = 0; r = jobs::levelResources(p, "task-order", 3);
        check(r.leveled.status == jobs::CalculationStatus::Conflict && r.leveled.tasks.empty() && !r.leveled.projectFinish && r.before.status == jobs::CalculationStatus::Conflict, "zero capacity treated as unknown or partial success");
        p.resources[0].maxUnits.reset(); r = jobs::levelResources(p, "task-order", 3);
        check(r.before.status == jobs::CalculationStatus::Incomplete && r.leveled.status == jobs::CalculationStatus::Incomplete && r.leveled.tasks.empty(), "unknown capacity silently treated as zero/available");
        auto tiny = project(); tiny.tasks[0].durationMinutes = tiny.tasks[1].durationMinutes = 1; tiny.resources[0].maxUnits = 0;
        check(jobs::levelResources(tiny, "task-order", 5200).leveled.status == jobs::CalculationStatus::Conflict, "long unavailable horizon produced a false schedule");
        p.resources[0].exceptions = {{"2026-10-27", 200}}; r = jobs::levelResources(p, "task-order", 3);
        check(r.leveled.status == jobs::CalculationStatus::Valid && r.after.status == jobs::CalculationStatus::Valid && r.leveled.tasks[0].start == "2026-10-27 09:00", "known capacity exception not used after unknown day");
        p = project(); p.holidays = {"2026-10-27"}; p.resources[0].exceptions = {{"2026-10-26", 0}}; r = jobs::levelResources(p, "task-order", 10);
        check(r.leveled.tasks[0].start == "2026-10-28 09:00" && r.leveled.tasks[1].start == "2026-10-29 09:00", "calendar holiday/capacity zero ignored");
        p = project(); p.resources.push_back({"machine", "공유 장비", "equipment", 100, {{"2026-10-26", 0}}});
        p.assignments.push_back({"a", "machine", 100}); p.assignments.push_back({"b", "machine", 100}); r = jobs::levelResources(p, "task-order", 10);
        check(r.leveled.tasks[0].start == "2026-10-27 09:00" && r.leveled.tasks[1].start == "2026-10-28 09:00" && r.after.status == jobs::CalculationStatus::Valid, "multiple simultaneous resource constraints ignored");
        for (const auto& resource : p.resources) {
            std::int64_t beforeWork = 0, afterWork = 0;
            for (const auto& d : r.before.days) if (d.resourceId == resource.id) beforeWork += d.demandUnitMinutes;
            for (const auto& d : r.after.days) if (d.resourceId == resource.id) afterWork += d.demandUnitMinutes;
            check(beforeWork == afterWork && beforeWork == 96000, "per-resource work not preserved");
        }
        p = project(); p.tasks[0].durationMinutes = 960; p.tasks.push_back({"c", "C", 960, {"b"}});
        auto order = jobs::levelResources(p, "task-order", 20), shortest = jobs::levelResources(p, "shortest-first", 20);
        check(order.leveled.projectFinish == "2026-10-30 18:00" && shortest.leveled.projectFinish == "2026-10-28 18:00" &&
            shortest.leveled.tasks[2].earlyStartMinutes >= shortest.leveled.tasks[1].earlyFinishMinutes && work(order.after) == work(shortest.after), "priority scenario/FS/work comparison failed");
        p = project(); p.statusAt = "2026-10-26 18:00";
        jobs::ProgressEntry e; e.id = "report"; e.taskId = "b"; e.statusAt = *p.statusAt; e.recordedAt = "2026-10-26T09:00:00Z";
        e.reporter = "PM"; e.reason = "reported"; e.sourceDocumentId = p.documentId; e.recordedRevision = 1; e.planDurationMinutes = 480;
        e.actualStart = "2026-10-26 09:00"; e.actualDurationMinutes = 120; e.remainingMinutes = 480; p.progress.push_back(e);
        r = jobs::levelResources(p, "task-order", 20);
        check(r.leveled.tasks[1].start == *e.actualStart && r.leveled.tasks[1].remainingStart == "2026-10-28 09:00" &&
            r.leveled.tasks[1].finish == "2026-10-28 18:00" && work(r.before) == 96000 && work(r.after) == 96000 && p.progress[0] == e,
            "leveling moved actual start or invented historical demand");
        p.progress[0].actualFinish = "2026-10-26 18:00"; p.progress[0].remainingMinutes = 0; r = jobs::levelResources(p, "task-order", 20);
        check(r.leveled.tasks[1].completedActual && r.leveled.tasks[1].finish == "2026-10-26 18:00" && work(r.before) == 48000, "completed actual allocated future demand");
        // Independent minute-by-minute oracle for mixed assignments, FS chains and daily capacities.
        for (int seed = 0; seed < 12; ++seed) {
            p = project(); p.tasks.clear(); p.assignments.clear(); p.resources[0].exceptions = {{"2026-10-27", 50}};
            for (int i = 0; i < 8; ++i) {
                auto id = "t" + std::to_string(i); std::vector<std::string> preds;
                if (i > 1 && (i + seed) % 3 == 0) preds.push_back("t" + std::to_string(i - 2));
                p.tasks.push_back({id, id, 60 * (1 + (i * 7 + seed) % 8), preds}); p.assignments.push_back({id, "r", 25 * (1 + (i + seed) % 4)});
            }
            r = jobs::levelResources(p, seed % 2 ? "shortest-first" : "longest-first", 30);
            check(r.leveled.status == jobs::CalculationStatus::Valid && work(r.before) == work(r.after), "oracle fixture failed/work changed");
            std::vector<int> demand(30 * 480, 0);
            for (std::size_t i = 0; i < p.tasks.size(); ++i) {
                const auto& row = r.leveled.tasks[i]; check(row.earlyFinishMinutes - row.earlyStartMinutes == *p.tasks[i].durationMinutes, "duration changed");
                check(row.earlyStartMinutes >= r.original.tasks[i].earlyStartMinutes, "leveling advanced original task");
                for (const auto& pred : p.tasks[i].predecessors) for (const auto& pr : r.leveled.tasks) if (pr.taskId == pred) check(row.earlyStartMinutes >= pr.earlyFinishMinutes, "leveling broke FS");
                for (auto m = row.earlyStartMinutes; m < row.earlyFinishMinutes; ++m) demand.at(std::size_t(m)) += p.assignments[i].units;
            }
            for (std::size_t m = 0; m < demand.size(); ++m) check(demand[m] <= (m / 480 == 1 ? 50 : 100), "independent overload oracle failed");
        }
        QTemporaryDir dir; check(dir.isValid(), "temporary folder failed"); jobs::JsonProjectStore store; jobs::Document doc(store);
        doc.create("자원 검증", "2026-10-26", {"A", "B"}); doc.setDuration(0, 480); doc.setDuration(1, 480);
        const auto a = doc.project()->tasks[0].id, b = doc.project()->tasks[1].id;
        auto rid = doc.putResource({{}, "개발팀", "labor", 100, {{"2026-10-27", 200}, {"2026-10-28", std::nullopt}}});
        doc.setAssignments(a, {{a, rid, 100}}); doc.setAssignments(b, {{b, rid, 100}}); auto before = *doc.project();
        rejects([&] { doc.removeResource(rid); }); rejects([&] { doc.setAssignments(a, {{a, "missing", 100}}); });
        rejects([&] { doc.setAssignments(a, {{a, rid, 50}, {a, rid, 50}}); });
        auto bad = before.resources[0]; bad.exceptions.push_back(bad.exceptions[0]); rejects([&] { doc.putResource(bad); });
        check(*doc.project() == before, "invalid resource edit changed revision/data");
        doc.applyCalculation(jobs::calculate(*doc.project())); auto preparedRevision = doc.project()->revision;
        auto scenarioId = doc.saveScenario("최초 A", "task-order", 260, "2026-10-26T00:00:00Z", doc.project()->documentId, preparedRevision);
        auto frozen = doc.project()->scenarios[0]; check(doc.calculation()->status == jobs::CalculationStatus::Stale && frozen.input.resources == before.resources, "scenario snapshot/cache mismatch");
        rejects([&] { doc.saveScenario("stale", "task-order", 260, "2026-10-26T00:00:00Z", doc.project()->documentId, preparedRevision); });
        doc.captureBaseline("resource baseline", "PM", "approval", "2026-10-26T00:00:00Z");
        doc.setAssignments(a, {{a, rid, 50}});
        auto assignmentComparison = jobs::compareToBaseline(*doc.project(), doc.project()->baselines[0].id);
        check(std::find(assignmentComparison.tasks[0].changes.begin(), assignmentComparison.tasks[0].changes.end(), "자원 배정 변경") != assignmentComparison.tasks[0].changes.end() &&
              assignmentComparison.tasks[0].baselineAssignments != assignmentComparison.tasks[0].planAssignments, "baseline assignment diff omitted");
        bad = doc.project()->resources[0]; bad.maxUnits = 200; doc.putResource(bad);
        check(doc.project()->scenarios[0] == frozen && doc.project()->baselines[0].input.resources[0].maxUnits == 100, "resource edit changed frozen scenario/baseline");
        auto comparison = jobs::compareToBaseline(*doc.project(), doc.project()->baselines[0].id);
        check(std::find(comparison.contextChanges.begin(), comparison.contextChanges.end(), "자원 정의/날짜별 가용량 변경") != comparison.contextChanges.end(), "baseline comparison omitted capacity change");
        auto path = dir.filePath("resources α.jobs"); doc.saveAs(path.toStdString()); auto bytes = read(path);
        check(store.load(doc.path()).project == *doc.project(), "resource/scenario file roundtrip failed");
        auto restored = jobs::scenarioProject(store.load(doc.path()).project.scenarios[0]);
        check(jobs::levelResources(restored, frozen.priority, frozen.horizonWorkDays).leveled.projectFinish == "2026-10-27 18:00", "saved scenario recomputation mismatch");
        jobs::Document clone(store); clone.open(doc.path(), true); rejects([&] { clone.putResource({{}, "blocked", "labor", 100, {}}); });
        clone.saveAs(dir.filePath("copy.jobs").toStdString()); check(clone.project()->scenarios[0].id == scenarioId && clone.project()->scenarios[0] == frozen && read(path) == bytes, "scenario provenance rewritten by copy");
        // A genuine schema-4 shape, including an old baseline input, migrates through a new file.
        auto root = QJsonDocument::fromJson(bytes).object(), payload = root["payload"].toObject(); auto legacy = payload;
        legacy.remove("resources"); legacy.remove("assignments"); legacy.remove("scenarios");
        auto baselines = legacy["baselines"].toArray(); for (int i = 0; i < baselines.size(); ++i) { auto item = baselines[i].toObject(), plan = item["input"].toObject(); plan.remove("resources"); plan.remove("assignments"); item["input"] = plan; baselines[i] = item; } legacy["baselines"] = baselines;
        root["schemaVersion"] = 4; const auto oldBytes = sign(root, legacy); auto oldPath = dir.filePath("v4.jobs"); write(oldPath, oldBytes);
        jobs::Document migrated(store); migrated.open(oldPath.toStdString()); check(migrated.requiresSaveAs() && migrated.project()->resources.empty() && migrated.project()->scenarios.empty(), "schema-4 defaults failed");
        rejects([&] { migrated.save(); }); migrated.saveAs(dir.filePath("v5.jobs").toStdString()); check(read(oldPath) == oldBytes && store.load(migrated.path()).project == *migrated.project(), "schema-4 original-preserving migration failed");
        root = QJsonDocument::fromJson(bytes).object(); auto malformed = payload; auto assignments = malformed["assignments"].toArray(); auto assignment = assignments[0].toObject(); assignment["resourceId"] = "missing"; assignments[0] = assignment; malformed["assignments"] = assignments;
        auto badPath = dir.filePath("bad.jobs"); write(badPath, sign(root, malformed)); rejects([&] { store.load(badPath.toStdString()); });
        malformed = payload; auto scenarios = malformed["scenarios"].toArray(); auto scenario = scenarios[0].toObject(); scenario["policy"] = "unknown-v99"; scenarios[0] = scenario; malformed["scenarios"] = scenarios;
        write(badPath, sign(root, malformed)); rejects([&] { store.load(badPath.toStdString()); });
        std::cout << "PASS: F05 peak/work, short overlap, fractional/unknown/zero/date capacity, deterministic priority/FS/horizon, retained actuals, independent minute oracle, transactional commands, immutable snapshots, persistence/provenance, v4 migration\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
