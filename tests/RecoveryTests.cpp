#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <functional>
#include <iostream>
#include <algorithm>
namespace {
void check(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
void rejects(const std::function<void()>& f) { bool bad = false; try { f(); } catch (const std::exception&) { bad = true; } check(bad, "expected recovery rejection"); }
QByteArray read(const QString& p) { QFile f(p); check(f.open(QIODevice::ReadOnly), "read"); return f.readAll(); }
void write(const QString& p, const QByteArray& bytes) { QFile f(p); check(f.open(QIODevice::WriteOnly), "write"); check(f.write(bytes) == bytes.size(), "short write"); }
QString checkpoint(const QString& root, const QString& id, int seq) { return QDir(root).filePath(id + QString("/checkpoint-%1.jobrecovery").arg(seq, 9, 10, QChar('0'))); }
QByteArray sign(QJsonObject root) { auto body = root["payload"].toObject(); root["sha256"] = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(body).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex()); return QJsonDocument(root).toJson(); }
jobs::RecoveryCandidate candidate(const jobs::RecoveryScan& scan, int seq) { for (const auto& c : scan.candidates) if (c.valid && c.sequence == std::uint64_t(seq)) return c; throw std::runtime_error("valid checkpoint missing"); }
QString awaitReady(QProcess& child) {
    QByteArray output;
    for (int i = 0; i < 100 && !output.contains('\n'); ++i) { child.waitForReadyRead(100); output += child.readAllStandardOutput(); if (child.state() == QProcess::NotRunning) break; }
    if (!output.startsWith("READY:")) { auto err = child.readAllStandardError(); throw std::runtime_error("child not ready: " + err.toStdString() + output.toStdString()); }
    return QString::fromLatin1(output.mid(6).trimmed());
}
void kill(QProcess& child) { child.kill(); check(child.waitForFinished(10000), "failed to terminate crash helper"); check(child.exitStatus() == QProcess::CrashExit, "helper did not terminate abnormally"); }
int child(const QStringList& args) {
    jobs::JsonProjectStore store; jobs::Document doc(store); const bool pending = args[1] == "--pending"; bool armed = false; QString sessionId;
    jobs::JsonRecoveryStore recovery(args[2], [&] { if (armed) { std::cout << "READY:" << sessionId.toStdString() << '\n' << std::flush; for (;;) QThread::msleep(100); } });
    if (args[3] == "untitled") { doc.create("처음 저장 전 복구", "2026-10-26", {"task"}); doc.setDuration(0, 480); } else doc.open(args[3].toStdString());
    auto session = recovery.newSession(); sessionId = session->id();
    for (int i = 1; i <= 4; ++i) { doc.renameTask(0, "crash edit " + std::to_string(i)); doc.setDuration(0, 960 + i); session->checkpoint(doc.recoverySnapshot("2026-10-26T09:00:00Z")); }
    if (pending) {
        doc.renameTask(0, "UNCOMMITTED AFTER FLUSH");
        // This hook runs after the temporary file was fully flushed, before commit.
        armed = true;
        session->checkpoint(doc.recoverySnapshot("2026-10-26T09:00:01Z"));
    }
    std::cout << "READY:" << session->id().toStdString() << '\n' << std::flush;
    for (;;) QThread::msleep(100);
}
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        const auto args = app.arguments(); if (args.size() == 4 && (args[1] == "--live" || args[1] == "--pending")) return child(args);
        QTemporaryDir dir; check(dir.isValid(), "temp directory"); const auto source = dir.filePath("원본 α.jobs"), root = dir.filePath("복구 세대"); jobs::JsonProjectStore store; jobs::Project saved;
        {
            jobs::Document seed(store); seed.create("시공 복구 검증", "2026-10-26", {"설계"}); seed.setCurrency("USD"); seed.setDuration(0, 960); const auto id = seed.project()->tasks[0].id;
            auto wbs = seed.addWbs("토목"); seed.assignWbs(0, wbs); auto resource = seed.putResource({{}, "공유팀", "labor", 100, {{"2026-10-28", 50}}, {{"2026-10-26", 50}}}); seed.setAssignments(id, {{id, resource, 100}}); seed.setTaskCost({id, 200, "uniform", "physical"});
            seed.captureBaseline("최초 승인", "PM", "승인", "2026-10-26T00:00:00Z"); seed.setStatusAt("2026-10-26 18:00");
            jobs::ProgressEntry p; p.taskId = id; p.actualStart = "2026-10-26 09:00"; p.actualDurationMinutes = 480; p.remainingMinutes = 480; p.reporter = "PM"; p.reason = "measured"; p.recordedAt = "2026-10-26T09:00:00Z"; seed.recordProgress(p);
            jobs::CostEntry c; c.taskId = id; c.actualMinor = 600; c.physicalBasisPoints = 4000; c.reporter = "PM"; c.reason = "invoice"; c.recordedAt = p.recordedAt; seed.recordCost(c);
            seed.saveScenario("대안", "task-order", 260, p.recordedAt, seed.project()->documentId, seed.project()->revision); seed.saveAs(source.toStdString()); saved = *seed.project();
        }
        const auto sourceBytes = read(source); jobs::JsonRecoveryStore recovery(root);
        QProcess process; process.start(app.applicationFilePath(), {"--live", root, source}); check(process.waitForStarted(10000), "start helper"); auto id = awaitReady(process);
        auto active = recovery.scan(); check(active.activeSessions == 1 && active.candidates.empty(), "running process exposed recovery candidates"); rejects([&] { recovery.discardSession(id); }); rejects([&] { store.lock(store.normalizePath(source.toStdString())); });
        kill(process); auto scan = recovery.scan(); check(scan.activeSessions == 0 && scan.candidates.size() == 3, "stale process lock / three generation retention failed");
        auto newest = candidate(scan, 4); check(newest.differences.contains("변경 1"), "recovery preview lost changed-task difference"); auto claimed = recovery.claim(newest);
        check(claimed->snapshot.project.tasks[0].name == "crash edit 4" && claimed->snapshot.project.baselines == saved.baselines && claimed->snapshot.project.progress == saved.progress && claimed->snapshot.project.costEntries == saved.costEntries && claimed->snapshot.project.scenarios == saved.scenarios && claimed->snapshot.lastSavedRevision == saved.revision && newest.sourceState.contains("일치"), "crash recovery changed data/facts/source save point");
        check(recovery.scan().activeSessions == 1, "recovery claim not exclusive"); rejects([&] { recovery.claim(newest); }); rejects([&] { recovery.discardSession(id); });
        auto originalLease = store.lock(store.normalizePath(source.toStdString())); jobs::Document restored(store); restored.recover(claimed->snapshot);
        check(restored.dirty() && restored.path().empty() && restored.requiresSaveAs() && !restored.readOnly() && !restored.canUndo() && restored.project()->documentId != saved.documentId && restored.project()->derivedFrom == saved.documentId && restored.recoveryOrigin()->revision == newest.revision, "recovered project attached to original/old identity");
        check(read(source) == sourceBytes, "recovery rewrote original"); auto restoredBefore = *restored.project(); auto badSnapshot = claimed->snapshot; badSnapshot.project.startDate = "bad"; rejects([&] { restored.recover(badSnapshot); }); check(*restored.project() == restoredBefore, "failed recover mutated current session");
        auto copy = dir.filePath("복구 사본.jobs"); restored.saveAs(copy.toStdString()); check(store.load(copy.toStdString()).project == *restored.project() && read(source) == sourceBytes, "recovered copy not portable or source changed"); originalLease.reset(); claimed.reset();
        // Corrupt/newer/partial generations remain diagnostic; valid older ones remain selectable.
        auto latestPath = checkpoint(root, id, 4); const auto latestBytes = read(latestPath); write(latestPath, "{truncated"); scan = recovery.scan(); check(scan.candidates.size() == 3 && !scan.candidates.back().valid && candidate(scan, 3).valid, "corruption hid valid previous generation");
        const auto older = candidate(scan, 3); auto olderPath = checkpoint(root, id, 3); auto olderBytes = read(olderPath); auto object = QJsonDocument::fromJson(olderBytes).object(); auto payload = object["payload"].toObject(); payload["capturedAt"] = "2026-10-26T09:00:02Z"; object["payload"] = payload; write(olderPath, sign(object)); rejects([&] { recovery.claim(older); }); write(olderPath, olderBytes);
        auto fallback = recovery.claim(candidate(recovery.scan(), 3)); check(fallback->snapshot.project.tasks[0].name == "crash edit 3", "explicit previous-generation recovery failed"); fallback.reset();
        write(latestPath, latestBytes); object = QJsonDocument::fromJson(latestBytes).object(); object["complete"] = false; write(latestPath, QJsonDocument(object).toJson()); check(!recovery.scan().candidates.back().valid, "incomplete generation accepted");
        object["complete"] = true; object["schemaVersion"] = 99; write(latestPath, QJsonDocument(object).toJson()); check(!recovery.scan().candidates.back().valid, "future recovery schema accepted"); write(latestPath, latestBytes);
        write(QDir(root).filePath(id + "/checkpoint-000000005.jobrecovery.part"), latestBytes); check(recovery.scan().candidates.size() == 3, "partial temporary file treated as completed generation");
        write(source, sourceBytes + "\n"); check(candidate(recovery.scan(), 4).sourceState.contains("원본 변경"), "external original change not shown"); write(source, sourceBytes); check(QFile::rename(source, source + ".moved"), "move original"); check(candidate(recovery.scan(), 4).sourceState.contains("없음"), "missing original blocked independent recovery"); check(QFile::rename(source + ".moved", source), "restore original location");
        // Actually kill a second process between flush and atomic publish.
        const auto pendingRoot = dir.filePath("pending"); QProcess pending; pending.start(app.applicationFilePath(), {"--pending", pendingRoot, source}); check(pending.waitForStarted(10000), "pending start"); awaitReady(pending); kill(pending);
        jobs::JsonRecoveryStore interrupted(pendingRoot); auto interruptedScan = interrupted.scan(); check(interruptedScan.candidates.size() == 3 && candidate(interruptedScan, 4).valid, "kill before commit lost previous generations");
        auto committed = interrupted.claim(candidate(interruptedScan, 4)); check(committed->snapshot.project.tasks[0].name == "crash edit 4", "uncommitted temporary content recovered"); committed.reset();
        const auto untitledRoot = dir.filePath("untitled"); QProcess untitled; untitled.start(app.applicationFilePath(), {"--live", untitledRoot, "untitled"}); check(untitled.waitForStarted(10000), "untitled start"); awaitReady(untitled); kill(untitled); jobs::JsonRecoveryStore unsaved(untitledRoot); auto u = unsaved.claim(candidate(unsaved.scan(), 4)); check(u->snapshot.sourcePath.empty() && !u->snapshot.lastSavedRevision, "never-saved recovery required original file"); u.reset();
        // Failed writes and path attacks cannot replace a completed generation or remove unrelated files.
        const auto failureRoot = dir.filePath("write-failure"); bool failPublish = false; jobs::JsonRecoveryStore failures(failureRoot, [&] { if (failPublish) throw std::runtime_error("injected disk failure"); }); auto writer = failures.newSession();
        jobs::RecoverySnapshot snapshot{saved, source.toStdString(), store.load(source.toStdString()).fingerprint, "2026-10-26T09:00:00Z", false, false, saved.revision}; writer->checkpoint(snapshot); failPublish = true; rejects([&] { writer->checkpoint(snapshot); }); writer.reset(); check(failures.scan().candidates.size() == 1 && candidate(failures.scan(), 1).valid, "failed publish destroyed previous completed checkpoint");
        auto unrelated = dir.filePath("unrelated.txt"); write(unrelated, "KEEP"); rejects([&] { recovery.discardSession("../unrelated.txt"); }); check(read(unrelated) == "KEEP", "cleanup escaped recovery root"); recovery.discardSession(id); check(recovery.scan().candidates.empty() && read(source) == sourceBytes, "explicit discard affected original or left completed records");
        auto denied = dir.filePath("not-a-directory"); write(denied, "KEEP"); jobs::JsonRecoveryStore inaccessible(denied); rejects([&] { inaccessible.newSession(); }); check(read(denied) == "KEEP", "failed recovery folder creation destroyed file");
        // An unverifiable lock is diagnosed, never silently hidden as a live app.
        jobs::JsonRecoveryStore uncertain(dir.filePath("uncertain-lock")); auto uncertainWriter = uncertain.newSession(); auto uncertainId = uncertainWriter->id(); uncertainWriter->checkpoint(snapshot); uncertainWriter.reset();
        const auto lockPath = QDir(uncertain.directory()).filePath(uncertainId + "/session.lock"); write(lockPath, "broken-lock"); auto uncertainScan = uncertain.scan(); check(uncertainScan.activeSessions == 0 && !uncertainScan.candidates.empty(), "unverifiable lock silently hid recovery data");
        check(read(checkpoint(uncertain.directory(), uncertainId, 1)).size() > 0, "unverifiable lock destroyed checkpoint"); QFile::remove(lockPath);
        // Legacy project inputs are checkpointed in the current format without rewriting source.
        const auto legacyPath = dir.filePath("legacy.jobs"); { jobs::Document seed(store); seed.create("legacy", "2026-10-26", {"task"}); seed.setDuration(0, 480); seed.saveAs(legacyPath.toStdString()); }
        auto legacyRoot = QJsonDocument::fromJson(read(legacyPath)).object(); legacyRoot["schemaVersion"] = 5; auto legacyPayload = legacyRoot["payload"].toObject();
        for (const auto& key : {"currency", "costProfile", "taskCosts", "costEntries"}) legacyPayload.remove(key);
        legacyRoot["payload"] = legacyPayload; write(legacyPath, sign(legacyRoot)); const auto legacyBytes = read(legacyPath);
        jobs::Document legacy(store); legacy.open(legacyPath.toStdString()); legacy.setDuration(0, 960); jobs::JsonRecoveryStore legacyRecovery(dir.filePath("legacy-recovery")); auto legacySession = legacyRecovery.newSession(); legacySession->checkpoint(legacy.recoverySnapshot("2026-10-26T09:00:00Z")); legacySession.reset();
        auto legacyClaim = legacyRecovery.claim(candidate(legacyRecovery.scan(), 1)); check(legacyClaim->snapshot.sourceRequiresSaveAs && legacyClaim->snapshot.project == *legacy.project() && read(legacyPath) == legacyBytes, "legacy checkpoint changed original or lost conversion metadata");
        std::cout << "PASS: actual process kill and pre-commit kill, stale/live locks, three generations, never-saved and Unicode recovery, immutable facts, detached copy, corruption/future/partial/race, missing/changed original, failed publish, exclusive discard and path confinement\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
