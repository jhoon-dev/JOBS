#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool c, const char* m) { if (!c) throw std::runtime_error(m); }
void rejects(const std::function<void()>& f) { bool rejected = false; try { f(); } catch (const std::exception&) { rejected = true; } check(rejected, "expected tracking file rejection"); }
QByteArray read(const QString& path) { QFile f(path); check(f.open(QIODevice::ReadOnly), "read failed"); return f.readAll(); }
void write(const QString& path, const QByteArray& bytes) { QFile f(path); check(f.open(QIODevice::WriteOnly), "write failed"); check(f.write(bytes) == bytes.size(), "write incomplete"); }
QByteArray signedPayload(QJsonObject root, QJsonObject payload) {
    root["payload"] = payload; root["sha256"] = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(payload).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
    return QJsonDocument(root).toJson();
}
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir; check(dir.isValid(), "temporary folder failed"); jobs::JsonProjectStore store; jobs::Document doc(store);
        doc.create("기준선·실적", "2026-10-26", {"설계"}); doc.setDuration(0, 960); auto wbs = doc.addWbs("단계"); doc.assignWbs(0, wbs);
        const auto baselineId = doc.captureBaseline("최초", "PM", "승인", "2026-10-26T00:00:00Z"); const auto baseline = doc.project()->baselines[0];
        doc.setStatusAt("2026-10-26 18:00"); jobs::ProgressEntry e; e.taskId = doc.project()->tasks[0].id; e.actualStart = "2026-10-26 09:00";
        e.actualDurationMinutes = 480; e.remainingMinutes = 960; e.reporter = "보고자"; e.reason = "현장 보고"; e.recordedAt = "2026-10-26T09:00:00Z"; doc.recordProgress(e);
        const auto firstReport = doc.project()->progress[0];
        e.reason = "잔여 정정"; e.remainingMinutes = 1200; doc.recordProgress(e);
        auto path = dir.filePath("기준선 α.jobs"); doc.saveAs(path.toUtf8().toStdString()); const auto saved = read(path);
        auto loaded = store.load(path.toUtf8().toStdString());
        check(!loaded.requiresSaveAs && loaded.project == *doc.project() && loaded.project.baselines[0] == baseline && loaded.project.progress[0] == firstReport,
              "tracking input/snapshot/ledger roundtrip mismatch");
        auto comparison = jobs::compareToBaseline(loaded.project, baselineId);
        check(comparison.forecastFinishVariance == 720 && comparison.tasks[0].actualStart == "2026-10-26 09:00", "reload comparison mismatch");
        jobs::Document clone(store); clone.open(path.toUtf8().toStdString(), true); clone.saveAs(dir.filePath("copy.jobs").toStdString());
        check(clone.project()->documentId != doc.project()->documentId && clone.project()->baselines == doc.project()->baselines && clone.project()->progress == doc.project()->progress,
              "save as rewrote historical provenance");
        clone.setHolidays({"2026-10-27"}); clone.save(); check(read(path) == saved && store.load(clone.path()).project.baselines[0] == baseline, "copy changed source or frozen calendar");
        const auto root = QJsonDocument::fromJson(saved).object(); const auto payload = root["payload"].toObject(); const auto badPath = dir.filePath("bad.jobs");
        auto tamper = [&](const QJsonObject& body) { write(badPath, signedPayload(root, body)); rejects([&] { store.load(badPath.toStdString()); }); };
        auto changed = payload; auto baselines = changed["baselines"].toArray(); auto b = baselines[0].toObject();
        auto input = b["input"].toObject(); auto tasks = input["tasks"].toArray(); auto t = tasks[0].toObject(); t["durationMinutes"] = 1200; tasks[0] = t;
        input["tasks"] = tasks; b["input"] = input; baselines[0] = b; changed["baselines"] = baselines; tamper(changed);
        changed = payload; baselines = changed["baselines"].toArray(); b = baselines[0].toObject(); b["approvedBy"] = ""; baselines[0] = b; changed["baselines"] = baselines; tamper(changed);
        changed = payload; auto progress = changed["progress"].toArray(); auto report = progress[1].toObject(); report["previousEntryId"] = "missing";
        progress[1] = report; changed["progress"] = progress; tamper(changed);
        changed = payload; progress = changed["progress"].toArray(); report = progress[0].toObject(); report["actualFinish"] = "2026-10-29 18:00";
        progress[0] = report; changed["progress"] = progress; tamper(changed);
        changed = payload; changed["statusAt"] = 42; tamper(changed);
        // Real schema-3 shape: preserve WBS, tasks, revision; empty new history; never overwrite original.
        auto v3 = root; auto legacy = payload; legacy.remove("resources"); legacy.remove("assignments"); legacy.remove("scenarios"); legacy.remove("baselines"); legacy.remove("progress"); legacy.remove("statusAt"); v3["schemaVersion"] = 3;
        const auto legacyBytes = signedPayload(v3, legacy); const auto legacyPath = dir.filePath("v3.jobs"); write(legacyPath, legacyBytes);
        jobs::Document migrated(store); migrated.open(legacyPath.toStdString());
        check(migrated.requiresSaveAs() && migrated.project()->baselines.empty() && migrated.project()->progress.empty() && !migrated.project()->statusAt &&
              migrated.project()->tasks == doc.project()->tasks && migrated.project()->wbsNodes == doc.project()->wbsNodes, "schema-3 defaults / inputs mismatch");
        rejects([&] { migrated.save(); }); migrated.saveAs(dir.filePath("migrated-v4.jobs").toStdString());
        check(read(legacyPath) == legacyBytes && store.load(migrated.path()).project == *migrated.project(), "v3 original modified or v4 migration failed");
        std::cout << "PASS: tracking roundtrip, retained history/provenance/clone, calendar immutability, baseline recomputation integrity, malformed ledger rejection, schema-3 copy migration\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
