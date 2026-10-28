#include "persistence/JsonProjectStore.h"
#include "persistence/JsonWorkspaceStore.h"
#include "persistence/WorkspaceFile.h"
#include "formatting/DiagnosticText.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <iostream>

namespace {
void check(bool value, const char *reason) {
    if (!value)
        throw std::logic_error(reason);
}
template <class F> void rejects(F call, const char *code) {
    try {
        call();
    } catch (const jobs::AppError &error) {
        check(error.code() == code, "unexpected workspace rejection code");
        return;
    }
    throw std::logic_error("workspace operation was accepted");
}
QByteArray read(const std::string &path) {
    QFile file(QString::fromStdString(path));
    check(file.open(QIODevice::ReadOnly), "read");
    return file.readAll();
}
void write(const std::string &path, const QByteArray &bytes) {
    QFile file(QString::fromStdString(path));
    check(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "write");
}
QByteArray sign(QJsonObject root, QJsonObject payload) {
    root["payload"] = payload;
    root["sha256"] = QString::fromLatin1(
        QCryptographicHash::hash(QJsonDocument(payload).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256)
            .toHex());
    return QJsonDocument(root).toJson();
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir;
        check(dir.isValid(), "temporary directory");
        jobs::JsonProjectStore files;
        jobs::JsonWorkspaceStore store(files);
        jobs::Workspace w;
        w.id = "workspace";
        const auto path = dir.filePath("MixedCase.jobsworkspace").toStdString();
        w.projects = {{"P", dir.filePath("Project.jobs").toStdString(), "Project"}};
        w.resources = {{"S", "Shared"}};
        w.resources[0].maxUnits.reset();
        w.resources[0].exceptions = {{"2026-10-26", 0}, {"2026-10-27", 100}};
        w.resources[0].calendar.holidays = {"2026-10-28"};
        w.bindings = {{"P", "R", "S"}};
        auto lease = files.lock(path);
        const auto fingerprint = store.save(path, w, {});
        auto loaded = store.load(path);
        check(loaded.workspace == w && loaded.fingerprint == fingerprint && loaded.path == files.normalizePath(path),
              "roundtrip changed workspace");
        const auto bytes = read(path);
        const auto root = QJsonDocument::fromJson(bytes).object();
        check(root["payload"].toObject()["projects"].toArray()[0].toObject()["path"] == "Project.jobs",
              "path not relative");
        check(jobs::decodeWorkspaceFile(bytes, dir.path()) == w, "memory/store decoder mismatch");
        check(QDir(dir.path()).entryList({"*.jobsworkspace"}, QDir::Files) == QStringList{"MixedCase.jobsworkspace"},
              "workspace filename lowercased");
        rejects([&] { (void)store.save(path, w, {}); }, "WORKSPACE_FILE_EXISTS");
        rejects([&] { (void)files.lock(path); }, "FILE_LOCKED");
        auto changed = w;
        changed.resources[0].maxUnits = 200;
        const auto next = store.save(path, changed, fingerprint);
        check(next != fingerprint && store.load(path).workspace == changed, "conditional replacement failed");
        rejects([&] { (void)store.save(path, w, fingerprint); }, "WORKSPACE_EXTERNAL_CHANGE");
        auto modified = root;
        modified["schemaVersion"] = 2;
        rejects([&] { (void)jobs::decodeWorkspaceFile(QJsonDocument(modified).toJson(), dir.path()); },
                "FILE_VERSION_UNSUPPORTED");
        modified = root;
        modified["sha256"] = "bad";
        rejects([&] { (void)jobs::decodeWorkspaceFile(QJsonDocument(modified).toJson(), dir.path()); },
                "FILE_INTEGRITY_FAILED");
        auto payload = root["payload"].toObject();
        payload["unknown"] = 1;
        rejects([&] { (void)jobs::decodeWorkspaceFile(sign(root, payload), dir.path()); }, "WORKSPACE_INVALID");
        payload = root["payload"].toObject();
        auto resources = payload["resources"].toArray();
        auto resource = resources[0].toObject();
        resource["maxUnits"] = 1.5;
        resources[0] = resource;
        payload["resources"] = resources;
        rejects([&] { (void)jobs::decodeWorkspaceFile(sign(root, payload), dir.path()); }, "WORKSPACE_INVALID");
        auto invalid = w;
        invalid.bindings.push_back(w.bindings[0]);
        rejects([&] { jobs::validateWorkspace(invalid); }, "WORKSPACE_BINDING_INVALID");
        invalid = w;
        invalid.projects.push_back(w.projects[0]);
        rejects([&] { jobs::validateWorkspace(invalid); }, "WORKSPACE_DUPLICATE_PROJECT");
        invalid = w;
        invalid.projects[0].path.clear();
        rejects([&] { (void)store.save(dir.filePath("Empty.jobsworkspace").toStdString(), invalid, {}); },
                "WORKSPACE_INVALID");
        rejects([&] { (void)jobs::decodeWorkspaceFile(QByteArray(8 * 1024 * 1024 + 1, ' '), dir.path()); },
                "WORKSPACE_LIMIT");
        write(path, "external bytes");
        const auto external = read(path);
        rejects([&] { (void)store.save(path, w, next); }, "WORKSPACE_EXTERNAL_CHANGE");
        check(read(path) == external, "external change overwritten");
        std::cout << "PASS: workspace roundtrip, relative paths, checksum, version/field/size/reference validation, "
                     "lock and external-change protection\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << jobs::errorText(error) << '\n';
        return 1;
    }
}
