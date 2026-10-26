#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QCryptographicHash>
#include <iostream>
#include <functional>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void rejects(const std::function<void()>& fn) {
    bool rejected = false;
    try { fn(); } catch (const std::exception&) { rejected = true; }
    check(rejected, "expected rejection");
}
QByteArray read(const QString& path) {
    QFile f(path); check(f.open(QIODevice::ReadOnly), "test read failed"); return f.readAll();
}
void write(const QString& path, const QByteArray& bytes) {
    QFile f(path); check(f.open(QIODevice::WriteOnly), "test write failed");
    check(f.write(bytes) == bytes.size(), "test write incomplete");
}
}
int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir;
        check(dir.isValid(), "temporary directory unavailable");
        auto a = dir.filePath(QString::fromUtf8("프로젝트 α.jobs"));
        auto b = dir.filePath("copy.jobs");
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        rejects([&] { doc.create("", "2026-10-26", {}); });
        rejects([&] { doc.create("test", "2026-02-30", {}); });
        check(!doc.project(), "invalid create changed document");
        doc.create("한글 프로젝트", "2026-10-26", {"설계", "개발"});
        const auto initial = *doc.project();
        check(doc.dirty(), "new document must be dirty");
        doc.saveAs(a.toUtf8().toStdString());
        check(!doc.dirty() && *doc.project() == initial, "first save changed identity or data");
        check(store.load(a.toUtf8().toStdString()).project == initial, "roundtrip mismatch");
        jobs::Document second(store);
        rejects([&] { second.open(a.toUtf8().toStdString()); });
        second.open(a.toUtf8().toStdString(), true);
        rejects([&] { second.addTask("blocked"); });
        const auto originalBytes = read(a);
        doc.saveAs(b.toStdString());
        check(doc.project()->documentId != initial.documentId && doc.project()->derivedFrom == initial.documentId,
              "save as identity/lineage mismatch");
        doc.renameTask(0, "변경된 설계"); doc.addTask("검증"); doc.save();
        check(read(a) == originalBytes, "save as changed source bytes");
        check(store.load(b.toStdString()).project == *doc.project(), "edited roundtrip mismatch");
        rejects([&] { doc.saveAs(a.toUtf8().toStdString()); });
        rejects([&] { doc.renameTask(0, "  "); });
        check(!doc.dirty(), "invalid edit changed dirty state");
        write(b, read(b) + "\n");
        auto external = read(b);
        doc.addTask("미저장");
        rejects([&] { doc.save(); });
        check(doc.dirty() && read(b) == external, "conflict overwrote external file or cleared dirty");
        auto beforeFailedOpen = *doc.project();
        auto broken = dir.filePath("broken.jobs"); write(broken, "not JSON");
        rejects([&] { doc.open(broken.toStdString()); });
        check(*doc.project() == beforeFailedOpen, "failed open replaced active document");
        auto future = QJsonDocument::fromJson(originalBytes).object(); future["schemaVersion"] = 4;
        write(broken, QJsonDocument(future).toJson());
        rejects([&] { store.load(broken.toStdString()); });
        auto corrupt = QJsonDocument::fromJson(originalBytes).object();
        auto payload = corrupt["payload"].toObject(); payload["name"] = "tampered"; corrupt["payload"] = payload;
        write(broken, QJsonDocument(corrupt).toJson());
        rejects([&] { store.load(broken.toStdString()); });
        // A valid checksum must not allow duplicate task IDs or unknown fields to be silently lost.
        auto sign = [&](QJsonObject p) {
            auto root = QJsonDocument::fromJson(originalBytes).object(); root["payload"] = p;
            root["sha256"] = QString::fromLatin1(QCryptographicHash::hash(
                QJsonDocument(p).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
            write(broken, QJsonDocument(root).toJson());
        };
        payload = QJsonDocument::fromJson(originalBytes).object()["payload"].toObject();
        payload["futureField"] = true; sign(payload);
        rejects([&] { store.load(broken.toStdString()); });
        payload.remove("futureField");
        auto tasks = payload["tasks"].toArray(); tasks.append(tasks.first()); payload["tasks"] = tasks;
        sign(payload);
        rejects([&] { store.load(broken.toStdString()); });
        rejects([&] { doc.saveAs(dir.filePath("missing/test.jobs").toStdString()); });
        check(doc.dirty() && *doc.project() == beforeFailedOpen, "failed save as changed session");
        // Copying a single file preserves IDs without the original folder/lock file.
        auto moved = dir.filePath("moved.jobs"); write(moved, originalBytes);
        check(store.load(moved.toStdString()).project == initial, "single file portability mismatch");
        second.saveAs(dir.filePath("read-only-copy.jobs").toStdString());
        second.addTask("사본 편집"); second.save();
        check(read(a) == originalBytes, "read-only copy modified source");
        // Scheduling inputs survive persistence; results remain a session cache.
        second.setDuration(0, 480); second.setDuration(1, 240);
        second.setPredecessors(1, {second.project()->tasks[0].id}); second.setHolidays({"2026-11-02"});
        auto result = jobs::calculate(*second.project()); second.applyCalculation(result);
        check(second.calculation()->status == jobs::CalculationStatus::Incomplete, "missing task duration ignored");
        second.setDuration(2, 60); second.applyCalculation(jobs::calculate(*second.project()));
        check(second.calculation()->status == jobs::CalculationStatus::Valid, "application calculation rejected");
        auto oldResult = *second.calculation(); second.setDuration(0, 960);
        check(second.calculation()->status == jobs::CalculationStatus::Stale, "edit failed to invalidate calculation");
        second.applyCalculation(oldResult);
        check(second.calculation()->status == jobs::CalculationStatus::Stale, "old revision overwrote current result");
        second.save();
        check(store.load(second.path()).project == *second.project(), "schedule input roundtrip mismatch");
        auto beforeBadHoliday = *second.project();
        rejects([&] { second.setHolidays({"2026-02-30"}); });
        check(*second.project() == beforeBadHoliday, "invalid holiday changed project");
        second.create("different document", "2026-10-26", {"task"}); second.applyCalculation(oldResult);
        check(second.calculation()->status == jobs::CalculationStatus::Stale, "wrong document calculation accepted");
        // Construct an actual schema-1 payload, then check non-destructive migration by Save As.
        auto legacy = QJsonDocument::fromJson(originalBytes).object();
        auto legacyPayload = legacy["payload"].toObject(); legacyPayload.remove("holidays"); legacyPayload.remove("calculationProfile"); legacyPayload.remove("wbsNodes");
        QJsonArray legacyTasks;
        for (const auto& value : legacyPayload["tasks"].toArray()) {
            auto task = value.toObject(); task.remove("durationMinutes"); task.remove("predecessors"); task.remove("wbsId"); legacyTasks.append(task);
        }
        legacyPayload["tasks"] = legacyTasks; legacy["payload"] = legacyPayload; legacy["schemaVersion"] = 1;
        legacy["sha256"] = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(legacyPayload).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
        auto legacyPath = dir.filePath("legacy.jobs"); const auto legacyBytes = QJsonDocument(legacy).toJson(); write(legacyPath, legacyBytes);
        jobs::Document migrated(store); migrated.open(legacyPath.toStdString());
        check(migrated.requiresSaveAs() && migrated.project()->documentId == initial.documentId && !migrated.project()->tasks[0].durationMinutes,
              "legacy defaults/identity mismatch");
        migrated.setDuration(0, 480); rejects([&] { migrated.save(); });
        check(read(legacyPath) == legacyBytes && migrated.dirty(), "legacy save modified original");
        migrated.saveAs(dir.filePath("migrated.jobs").toStdString());
        check(!migrated.requiresSaveAs() && read(legacyPath) == legacyBytes && store.load(migrated.path()).project == *migrated.project(), "legacy save as failed");
        auto parent = migrated.addWbs("단계 α"), child = migrated.addWbs("상세", parent);
        migrated.assignWbs(0, child); migrated.save();
        check(store.load(migrated.path()).project == *migrated.project(), "WBS hierarchy/task assignment roundtrip mismatch");
        // Schema 2 keeps scheduling input and IDs, adds an empty hierarchy, and preserves original bytes.
        auto version2 = QJsonDocument::fromJson(read(QString::fromStdString(migrated.path()))).object();
        auto p2 = version2["payload"].toObject(); p2.remove("wbsNodes");
        QJsonArray tasks2;
        for (auto value : p2["tasks"].toArray()) { auto t = value.toObject(); t.remove("wbsId"); tasks2.append(t); }
        p2["tasks"] = tasks2; version2["payload"] = p2; version2["schemaVersion"] = 2;
        version2["sha256"] = QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(p2).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex());
        const auto bytes2 = QJsonDocument(version2).toJson(); auto path2 = dir.filePath("version2.jobs"); write(path2, bytes2);
        jobs::Document from2(store); from2.open(path2.toStdString());
        check(from2.requiresSaveAs() && from2.project()->wbsNodes.empty() && from2.project()->tasks[0].wbsId.empty() &&
              from2.project()->tasks[0].id == migrated.project()->tasks[0].id && from2.project()->tasks[0].durationMinutes == 480,
              "schema 2 input/identity migration mismatch");
        rejects([&] { from2.save(); });
        from2.saveAs(dir.filePath("version3.jobs").toStdString());
        check(read(path2) == bytes2 && store.load(from2.path()).project == *from2.project(), "schema 2 original changed / new copy failed");
        auto badWbs = QJsonDocument::fromJson(originalBytes).object()["payload"].toObject();
        badWbs["wbsNodes"] = QJsonArray{QJsonObject{{"id", "w1"}, {"name", "one"}, {"parentId", "w2"}},
            QJsonObject{{"id", "w2"}, {"name", "two"}, {"parentId", "w1"}}};
        sign(badWbs); rejects([&] { store.load(broken.toStdString()); });
        std::cout << "PASS: file lifecycle, Unicode, locks/conflicts, corruption/schema, schedule input roundtrip, stale revision/identity, schema-1 copy migration\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
