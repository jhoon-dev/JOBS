#include "ParserHarness.h"
#include "persistence/ProjectFile.h"
#include "persistence/ProjectJson.h"
#include "persistence/ProjectXml.h"
#include "persistence/JsonProjectStore.h"
#include "domain/CalculationControl.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
void check(bool condition, const char* message) {
    if (!condition) throw std::logic_error(message);
}
QByteArray read(const QString& path) {
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "cannot read seed fixture");
    return file.readAll();
}
QByteArray envelope(const QJsonObject& body, int schema) {
    const auto digest = QCryptographicHash::hash(
        QJsonDocument(body).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex();
    return QJsonDocument(QJsonObject{{"format", "JOBS"}, {"schemaVersion", schema},
        {"payload", body}, {"sha256", QString::fromLatin1(digest)}}).toJson(QJsonDocument::Compact);
}
struct Seed {
    QString parser, name;
    QByteArray bytes;
    bool valid;
};
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        const auto fixtureRoot = QString::fromUtf8(JOBS_FIXTURES);
        std::vector<Seed> seeds;
        jobs::Project minimal;
        minimal.documentId = "parser-seed"; minimal.name = "한글 seed"; minimal.startDate = "2026-10-26";
        minimal.tasks = {{"T1", "Task", 60, {}}};
        for (int schema = 1; schema <= 9; ++schema) {
            auto body = jobs::json::encodeProject(minimal);
            const auto remove = [&](int introduced, std::initializer_list<const char*> fields) {
                if (schema < introduced) for (auto field : fields) body.remove(QString::fromLatin1(field));
            };
            remove(2, {"holidays", "calculationProfile"}); remove(3, {"wbsNodes"});
            remove(4, {"statusAt", "baselines", "progress"}); remove(5, {"resources", "assignments", "scenarios"});
            remove(6, {"currency", "costProfile", "taskCosts", "costEntries"});
            auto task = body["tasks"].toArray()[0].toObject();
            if (schema < 2) { task.remove("durationMinutes"); task.remove("predecessors"); }
            if (schema < 3) task.remove("wbsId");
            body["tasks"] = QJsonArray{task};
            const auto bytes = envelope(body, schema);
            (void)jobs::decodeProjectFile(bytes);
            seeds.push_back({"jobs", QString("schema-%1.jobs").arg(schema), bytes, true});
            seeds.push_back({"jobs", QString("payload-%1.json").arg(schema), QJsonDocument(body).toJson(), true});
        }
        const auto xml = read(fixtureRoot + "/project-xml-plan.xml");
        const auto xer = read(fixtureRoot + "/primavera-plan.xer");
        seeds.push_back({"xml", "plan.xml", xml, true}); seeds.push_back({"xer", "plan.xer", xer, true});
        auto westernXer = QString::fromUtf8(xer).toLatin1();
        westernXer.replace("SITE-01", QByteArray("Caf\xe9", 4));
        check(jobs::fuzz::probe("xer", westernXer).accepted == 1, "XER encoding branches were not distinguished");
        seeds.push_back({"xer", "windows-1252.xer", westernXer, true});
        // Rich PDM, constraints and effort payloads reach profile-specific decoders.
        for (const auto* profile : {jobs::pdmProfile, jobs::constraintProfile, jobs::effortProfile}) {
            auto p = jobs::importProjectXml(xml, "seed.xml").project;
            p.calculationProfile = profile;
            const auto bytes = envelope(jobs::json::encodeProject(p), 9);
            (void)jobs::decodeProjectFile(bytes);
            seeds.push_back({"jobs", QString::fromLatin1(profile) + ".jobs", bytes, true});
        }
        const auto seedCount = seeds.size();
        for (std::size_t i = 0; i < seedCount; ++i) {
            auto bytes = seeds[i].bytes.left(seeds[i].bytes.size() / 2);
            // Truncating a raw payload is deliberately invalid JSON too.
            seeds.push_back({seeds[i].parser, seeds[i].name + ".truncated", bytes, false});
        }
        for (const auto* parser : {"jobs", "xml", "xer"}) {
            seeds.push_back({parser, "empty", {}, false});
            seeds.push_back({parser, "binary", QByteArray("\0\xff\xfe\x01", 4), false});
        }
        auto corrupt = envelope(jobs::json::encodeProject(minimal), 9);
        corrupt.replace("Task", "Tampered");
        bool rejected = false;
        try { (void)jobs::decodeProjectFile(corrupt); } catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "envelope checksum bypassed");
        seeds.push_back({"jobs", "checksum.jobs", corrupt, true}); // body path still accepts it
        auto unknown = jobs::json::encodeProject(minimal); unknown["unknown"] = 1;
        seeds.push_back({"jobs", "unknown.jobs", envelope(unknown, 9), false});
        rejected = false;
        try { (void)jobs::decodeProjectFile(QByteArray(jobs::projectFileLimitBytes + 1, ' ')); }
        catch (const std::runtime_error&) { rejected = true; }
        check(rejected, "native byte limit bypassed");
        bool logicEscaped = false;
        try { (void)jobs::fuzz::probe("unknown", {}); } catch (const std::logic_error&) { logicEscaped = true; }
        check(logicEscaped, "harness swallowed a logic error");
        auto cancelled = std::make_shared<jobs::CalculationControl>(); cancelled->cancel();
        bool cancellationEscaped = false;
        {
            jobs::EvaluationScope evaluation(cancelled);
            try { (void)jobs::fuzz::probe("jobs", seeds.front().bytes); }
            catch (const jobs::CalculationCancelled&) { cancellationEscaped = true; }
        }
        check(cancellationEscaped, "harness swallowed cancellation");
        for (const auto& seed : seeds) {
            const auto result = jobs::fuzz::probe(seed.parser.toStdString(), seed.bytes);
            if (seed.valid) check(result.accepted > 0, "valid seed never reached successful parser path");
            else check(result.accepted == 0 && result.rejected > 0, "invalid seed was accepted");
            if (app.arguments().size() == 3 && app.arguments()[1] == "--write-corpus") {
                const auto directory = app.arguments()[2] + '/' + seed.parser;
                check(QDir().mkpath(directory), "cannot create corpus directory");
                QFile file(directory + '/' + seed.name);
                check(file.open(QIODevice::WriteOnly) && file.write(seed.bytes) == seed.bytes.size(), "cannot write corpus seed");
            }
        }
        // The memory parser and real store must agree on successful input.
        QTemporaryDir directory;
        jobs::JsonProjectStore store;
        const auto path = directory.filePath("Seed.jobs").toStdString();
        store.save(path, minimal, {});
        check(store.load(path).project == jobs::decodeProjectFile(read(QString::fromStdString(path))), "store decoder diverged");
        std::cout << "PASS: " << seeds.size() << " parser seeds, schema 1..9, checksum/body separation, byte limit and store parity\n";
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
