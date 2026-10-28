#include "application/Document.h"
#include "formatting/PlanTokens.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include "persistence/ProjectFile.h"
#include "persistence/ProjectJson.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <set>
#include <type_traits>

using namespace jobs;
static_assert(std::is_same_v<decltype(Dependency{}.type), DependencyType>);
static_assert(std::is_same_v<decltype(Task{}.constraintType), ConstraintType>);
static_assert(std::is_same_v<decltype(Task{}.taskType), TaskType>);
static_assert(!std::is_convertible_v<std::string, DependencyType>);
static_assert(!std::is_convertible_v<int, ConstraintType>);
static_assert(!std::is_convertible_v<TaskType, int>);

namespace {
int checks = 0;
void check(bool condition, const std::string &message) {
    ++checks;
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, const std::string &message) {
    bool rejected = false;
    try {
        action();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, message);
}
QByteArray read(const QString &path) {
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "read " + path.toStdString());
    return file.readAll();
}
void tokens() {
    const std::pair<DependencyType, const char *> relations[] = {
        {DependencyType::FS, "FS"}, {DependencyType::SS, "SS"}, {DependencyType::FF, "FF"}, {DependencyType::SF, "SF"}};
    const bool fromFinish[]{true, false, true, false}, toFinish[]{false, false, true, true};
    int i = 0;
    for (const auto &[value, token] : relations) {
        check(enumToken(value) == token && dependencyTypeFromToken(token) == value, "relationship canonical token");
        check(dependencyFromFinish(value) == fromFinish[i] && dependencyToFinish(value) == toFinish[i],
              "endpoint semantics");
        ++i;
    }
    const std::pair<ConstraintType, const char *> constraints[] = {
        {ConstraintType::ASAP, "ASAP"}, {ConstraintType::ALAP, "ALAP"}, {ConstraintType::SNET, "SNET"},
        {ConstraintType::SNLT, "SNLT"}, {ConstraintType::FNET, "FNET"}, {ConstraintType::FNLT, "FNLT"},
        {ConstraintType::MSO, "MSO"},   {ConstraintType::MFO, "MFO"}};
    for (const auto &[value, token] : constraints)
        check(enumToken(value) == token && constraintTypeFromToken(token) == value, "constraint canonical token");
    const std::pair<TaskType, const char *> tasks[] = {{TaskType::FixedDuration, "fixed-duration"},
                                                       {TaskType::FixedWork, "fixed-work"},
                                                       {TaskType::FixedUnits, "fixed-units"}};
    for (const auto &[value, token] : tasks)
        check(enumToken(value) == token && taskTypeFromToken(token) == value, "task type canonical token");
    for (const auto token : {"", "fs", " FS", "FS ", "XX", "0"})
        rejects([&] { dependencyTypeFromToken(token); }, "invalid relationship token accepted");
    for (const auto token : {"", "asap", " ASAP", "ASAP ", "XX", "0"})
        rejects([&] { constraintTypeFromToken(token); }, "invalid constraint token accepted");
    for (const auto token : {"", "Fixed-Work", " fixed-work", "fixed-work ", "work", "1"})
        rejects([&] { taskTypeFromToken(token); }, "invalid task type token accepted");
    rejects([] { enumToken(static_cast<DependencyType>(-1)); }, "invalid relationship encoded");
    rejects([] { enumToken(static_cast<ConstraintType>(99)); }, "invalid constraint encoded");
    rejects([] { enumToken(static_cast<TaskType>(99)); }, "invalid task type encoded");
    rejects([] { dependencyFromFinish(static_cast<DependencyType>(99)); }, "invalid predecessor endpoint");
    rejects([] { dependencyToFinish(static_cast<DependencyType>(99)); }, "invalid successor endpoint");
}
void nestedWire(const QJsonValue &value) {
    if (value.isArray()) {
        for (const auto &child : value.toArray())
            nestedWire(child);
    } else if (value.isObject()) {
        const auto object = value.toObject();
        for (auto it = object.begin(); it != object.end(); ++it) {
            if (it.key() == "constraintType")
                check(it.value().isString() && isValid(constraintTypeFromToken(it.value().toString().toStdString())),
                      "constraint ordinal on wire");
            if (it.key() == "taskType")
                check(it.value().isString() && isValid(taskTypeFromToken(it.value().toString().toStdString())),
                      "task ordinal on wire");
            if (it.key() == "type" && object.contains("predecessorId"))
                check(it.value().isString() && isValid(dependencyTypeFromToken(it.value().toString().toStdString())),
                      "relationship ordinal on wire");
            nestedWire(it.value());
        }
    }
}
void invalidValues(const Project &project) {
    auto p = project;
    p.tasks[0].taskType = static_cast<TaskType>(-1);
    rejects([&] { validate(p); }, "domain accepted invalid task enum");
    p = project;
    p.tasks[0].constraintType = static_cast<ConstraintType>(-1);
    rejects([&] { validate(p); }, "domain accepted invalid constraint enum");
    p = project;
    p.tasks[1].dependencyRules[0].type = static_cast<DependencyType>(-1);
    rejects([&] { validate(p); }, "domain accepted invalid relationship enum");
    const auto original = json::encodeProject(project);
    for (const auto &key : {"constraintType", "taskType"}) {
        for (const QJsonValue value : {QJsonValue("unknown"), QJsonValue(0), QJsonValue(QJsonValue::Null)}) {
            auto body = original;
            auto tasks = body["tasks"].toArray();
            auto task = tasks[0].toObject();
            task[key] = value;
            tasks[0] = task;
            body["tasks"] = tasks;
            rejects([&] { json::decodeProject(body, 9); }, "native decoder accepted bad enum field");
        }
    }
    for (const QJsonValue value : {QJsonValue("ss"), QJsonValue(1), QJsonValue(QJsonValue::Null)}) {
        auto body = original;
        auto tasks = body["tasks"].toArray();
        auto task = tasks[1].toObject();
        auto rules = task["dependencyRules"].toArray();
        auto rule = rules[0].toObject();
        rule["type"] = value;
        rules[0] = rule;
        task["dependencyRules"] = rules;
        tasks[1] = task;
        body["tasks"] = tasks;
        rejects([&] { json::decodeProject(body, 9); }, "native decoder accepted bad relationship");
    }
}
void history(const Project &p, JsonProjectStore &store, QTemporaryDir &dir) {
    std::set<DependencyType> relations;
    std::set<ConstraintType> constraints;
    std::set<TaskType> types;
    for (const auto &task : p.tasks) {
        constraints.insert(task.constraintType);
        types.insert(task.taskType);
        for (const auto &d : dependencies(task))
            relations.insert(d.type);
    }
    check(relations.size() == 4 && constraints.size() == 8 && types.size() == 3, "pre-enum fixture coverage");
    check(p.baselines.size() == 1 && p.scenarios.size() == 1 && p.progress.size() == 1 && p.costEntries.size() == 1,
          "pre-enum immutable history missing");
    invalidValues(p);
    Document doc(store);
    doc.open((QString(JOBS_ENUM_FIXTURES) + "/history.jobs").toStdString(), true);
    doc.saveAs(dir.filePath("History Copy.jobs").toStdString());
    const auto saved = *doc.project();
    doc.renameTask(8, "Enum undo edit");
    doc.undo();
    check(static_cast<const PlanInput &>(*doc.project()) == static_cast<const PlanInput &>(saved),
          "undo changed enum input");
    doc.redo();
    check(doc.project()->tasks[8].name == "Enum undo edit", "redo failed");
    check(doc.project()->baselines == p.baselines && doc.project()->scenarios == p.scenarios &&
              doc.project()->progress == p.progress && doc.project()->costEntries == p.costEntries,
          "undo rewrote facts");
    const auto before = *doc.project();
    rejects([&] { doc.setTaskTiming(8, static_cast<ConstraintType>(99), {}, {}); }, "invalid constraint edit accepted");
    rejects([&] { doc.setTaskEffort(8, static_cast<TaskType>(99), false, 60, {}, {}); },
            "invalid effort edit accepted");
    check(*doc.project() == before, "invalid enum edit mutated project");
    JsonRecoveryStore recovery(dir.filePath("enum-recovery"));
    auto session = recovery.newSession();
    session->checkpoint(doc.recoverySnapshot("2026-10-28T00:00:00Z"));
    session.reset();
    auto scan = recovery.scan();
    check(scan.candidates.size() == 1 && scan.candidates[0].valid, "enum checkpoint invalid");
    auto claimed = recovery.claim(scan.candidates[0]);
    check(claimed->snapshot.project == *doc.project(), "checkpoint lost enum/history");
    Document restored(store);
    restored.recover(claimed->snapshot);
    check(static_cast<const PlanInput &>(*restored.project()) == static_cast<const PlanInput &>(*doc.project()) &&
              restored.project()->baselines == p.baselines && restored.project()->scenarios == p.scenarios,
          "recovery changed enum history");
    restored.saveAs(dir.filePath("Recovered Enums.jobs").toStdString());
    check(store.load(restored.path()).project == *restored.project(), "recovered enum save failed");
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        tokens();
        QTemporaryDir dir;
        check(dir.isValid(), "temporary directory");
        JsonProjectStore store;
        const QDir fixture(JOBS_ENUM_FIXTURES);
        const auto manifest =
            QJsonDocument::fromJson(read(fixture.filePath("manifest.json"))).object()["files"].toArray();
        check(manifest.size() == 13, "fixture manifest coverage");
        for (const auto &item : manifest) {
            const auto entry = item.toObject();
            const auto name = entry["file"].toString();
            const auto path = fixture.filePath(name);
            const auto bytes = read(path);
            const auto sha = QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
            check(sha == entry["sha256"].toString().toLatin1(), "pre-enum bytes changed: " + name.toStdString());
            const auto root = QJsonDocument::fromJson(bytes).object();
            const auto version = root["schemaVersion"].toInt();
            const auto p = decodeProjectFile(bytes);
            const auto loaded = store.load(path.toStdString());
            check(p == loaded.project && loaded.requiresSaveAs == (version != 9), "legacy load/upgrade metadata");
            const auto encoded = json::encodeProject(p);
            nestedWire(encoded);
            if (version == 9)
                check(encoded == root["payload"].toObject(),
                      "current writer changed old schema9 payload: " + name.toStdString());
            if (version < 7)
                for (const auto &t : p.tasks) {
                    check(t.constraintType == ConstraintType::ASAP && t.taskType == TaskType::FixedDuration,
                          "legacy defaults changed");
                    for (const auto &d : dependencies(t))
                        check(d.type == DependencyType::FS, "legacy relationship default changed");
                }
            Document doc(store);
            doc.open(path.toStdString(), true);
            check(doc.readOnly(), "read-only open failed");
            doc.saveAs(dir.filePath(name).toStdString());
            const auto output = QJsonDocument::fromJson(read(QString::fromStdString(doc.path()))).object();
            check(output["schemaVersion"] == 9, "writer bumped schema");
            nestedWire(output["payload"]);
            check(static_cast<const PlanInput &>(*doc.project()) == static_cast<const PlanInput &>(p) &&
                      doc.project()->baselines == p.baselines && doc.project()->scenarios == p.scenarios &&
                      doc.project()->progress == p.progress && doc.project()->costEntries == p.costEntries,
                  "SaveAs changed old content");
            check(store.load(doc.path()).project == *doc.project(), "saved legacy project did not reopen");
            check(read(path) == bytes, "compatibility test overwrote original fixture");
            if (name == "history.jobs")
                history(p, store, dir);
        }
        std::cout << "PASS: " << checks
                  << " checks; 13 pre-enum SHA fixtures, schemas1-9, strict tokens, typed validation, full history, "
                     "SaveAs, undo and recovery\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
