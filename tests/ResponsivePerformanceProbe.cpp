#include "BuildIdentity.h"
#include "PerformanceFixture.h"
#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
#include <QTemporaryDir>
#include <algorithm>
#include <future>
#include <iostream>

namespace {
QJsonObject samples(std::vector<double> values) {
    QJsonArray raw; for (auto value : values) raw.append(value);
    std::sort(values.begin(), values.end());
    return {{"samples_ms", raw}, {"median_ms", values[values.size() / 2]},
            {"observed_p95_ms", values[(95 * values.size() + 99) / 100 - 1]}};
}
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
}
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    QCommandLineParser parser; parser.addHelpOption();
    parser.addOptions({{"tasks", "Task count", "count", "10000"}, {"baselines", "Baseline count", "count", "3"},
                       {"repeats", "Odd repeat count", "count", "3"}, {"output", "Output JSON", "path"}});
    parser.process(app);
    try {
        const auto n = parser.value("tasks").toInt(), baselineCount = parser.value("baselines").toInt(), repeats = parser.value("repeats").toInt();
        check((n == 1000 || n == 10000) && (baselineCount == 0 || baselineCount == 3) && repeats > 0 && repeats <= 11 && repeats % 2,
              "unsupported fixture/repeats");
        QTemporaryDir dir; jobs::JsonProjectStore store;
        const auto path = dir.filePath("Snapshot.jobs").toUtf8().toStdString();
        store.save(path, performanceFixture(n, true), {});
        jobs::Document document(store); document.open(path);
        for (int i = 0; i < baselineCount; ++i)
            document.captureBaseline("Baseline " + std::to_string(i), "PM", "response measurement", "2026-10-28T00:00:00Z");
        document.save(); const auto original = *document.project();
        std::vector<double> requestTimes, workerTimes, applyTimes;
        for (int i = 0; i < repeats; ++i) {
            QElapsedTimer timer; timer.start();
            auto request = document.makeEditRequest(jobs::DurationEdit{"T0", 120});
            requestTimes.push_back(double(timer.nsecsElapsed()) / 1e6);
            auto worker = std::async(std::launch::async, [request = std::move(request)]() mutable {
                QElapsedTimer clock; clock.start();
                auto result = jobs::Document::prepareEdit(std::move(request), std::make_shared<jobs::CalculationControl>());
                return std::pair{std::move(result), double(clock.nsecsElapsed()) / 1e6};
            });
            auto [result, elapsed] = worker.get(); workerTimes.push_back(elapsed);
            check(result->state() == jobs::EditState::Ready, "preparation failed");
            timer.restart(); const auto state = document.applyPreparedEdit(*result);
            applyTimes.push_back(double(timer.nsecsElapsed()) / 1e6);
            check(state == jobs::EditState::Applied && document.dirty() && document.project()->tasks[0].durationMinutes == 120 &&
                  document.project()->tasks[0].workUnitMinutes == 6000 && document.project()->assignments[0].units == 50,
                  "prepared edit changed work/failed dirty");
            document.undo();
            check(!document.dirty() && static_cast<const jobs::PlanInput&>(*document.project()) == static_cast<const jobs::PlanInput&>(original) &&
                  document.project()->baselines == original.baselines, "undo did not restore input/save point");
        }
        QJsonObject output{{"status", "success"}, {"fixture", "jobs-responsive-phases-v1"}, {"task_count", n},
            {"baseline_count", baselineCount}, {"repeats", repeats}, {"request_gui", samples(requestTimes)},
            {"prepare_worker", samples(workerTimes)}, {"apply_gui", samples(applyTimes)},
            {"source_revision", JOBS_PERF_REVISION}, {"built_product_source_sha256", JOBS_SOURCE_SHA256},
            {"os", QSysInfo::prettyProductName()}, {"platform", QApplication::platformName()}, {"qt", qVersion()},
            {"scope", "Sealed request creation, isolated worker preparation, Document GUI commit only; excludes UI refresh, explicit save/open, controlled result destruction and OS presentation"}};
        QFile file(parser.value("output")); check(file.open(QIODevice::WriteOnly), "output unavailable");
        file.write(QJsonDocument(output).toJson());
        std::cout << "PASS: response phases " << n << "/" << baselineCount << '\n';
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
