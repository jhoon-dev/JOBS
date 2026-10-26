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
        auto future = QJsonDocument::fromJson(originalBytes).object(); future["schemaVersion"] = 2;
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
        std::cout << "PASS: validation, Unicode roundtrip, stable IDs/revisions, save-as lineage, locks, read-only, conflict, corruption, schema, failed-open/save, single-file copy\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
