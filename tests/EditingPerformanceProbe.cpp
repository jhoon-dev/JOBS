#include "TaskTableTest.h"
#include "BuildIdentity.h"
#include "PerformanceFixture.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include <QAction>
#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSysInfo>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QThread>
#include <algorithm>
#include <iostream>
#include <map>

namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
QJsonObject samples(std::vector<double> values) {
    QJsonArray raw;
    for (double value : values) raw.append(value);
    std::sort(values.begin(), values.end());
    const auto n = values.size();
    return {{"samples_ms", raw}, {"median_ms", n % 2 ? values[n / 2] : (values[n / 2 - 1] + values[n / 2]) / 2},
            {"observed_p95_ms", values[(95 * n + 99) / 100 - 1]}};
}
QString git(const QStringList& args) {
    QProcess process;
    process.setWorkingDirectory(JOBS_SOURCE_DIR);
    process.start("git", args);
    if (!process.waitForFinished(5000) || process.exitCode()) return "unavailable";
    return QString::fromUtf8(process.readAllStandardOutput()).trimmed();
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    try {
        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addOptions({{"tasks", "Task count", "count", "1000"},
                           {"baselines", "Baseline count (0 or 3)", "count", "0"},
                           {"repeats", "Samples per operation", "count", "7"},
                           {"output", "JSON output path", "path", "editing-performance.json"},
                           {"require-100", "Require all 100 edits to remain undoable"}});
        parser.process(app);
        bool okay = false;
        const int count = parser.value("tasks").toInt(&okay);
        check(okay && count >= 1 && count <= 10000, "tasks must be 1..10000");
        const int baselines = parser.value("baselines").toInt(&okay);
        check(okay && (baselines == 0 || baselines == 3), "baselines must be 0 or 3");
        const int repeats = parser.value("repeats").toInt(&okay);
        check(okay && repeats >= 1 && repeats <= 100, "repeats must be 1..100");
        QTemporaryDir directory;
        check(directory.isValid(), "temporary directory unavailable");
        jobs::JsonProjectStore store;
        auto fixture = performanceFixture(count, false);
        const auto path = directory.filePath("EditPlan.jobs").toUtf8().toStdString();
        store.save(path, fixture, {});
        jobs::Document document(store);
        document.open(path);
        for (int i = 0; i < baselines; ++i)
            document.captureBaseline("Baseline " + std::to_string(i), "PM", "benchmark", "2026-10-27T00:00:00Z");
        document.save();
        const auto initial = *document.project();
        std::map<QString, std::vector<double>> timings;
        auto measure = [&](const QString& name, auto operation, bool events = false) {
            QElapsedTimer timer;
            timer.start();
            operation();
            if (events) app.processEvents();
            timings[name].push_back(double(timer.nsecsElapsed()) / 1000000);
        };
        for (int i = 0; i < repeats; ++i) {
            const auto name = "Document edit " + std::to_string(i);
            measure("document_name", [&] { document.renameTask(0, name); });
            check(document.project()->tasks[0].name == name, "name edit lost");
            measure("document_undo", [&] { document.undo(); });
            measure("document_redo", [&] { document.redo(); });
            document.undo();
            measure("document_duration", [&] { document.setDuration(0, 120 + i); });
            check(document.project()->tasks[0].durationMinutes == 120 + i, "duration edit lost");
            measure("document_duration_undo", [&] { document.undo(); });
            measure("document_duration_redo", [&] { document.redo(); });
            document.undo();
        }
        check(static_cast<const jobs::PlanInput&>(*document.project()) == static_cast<const jobs::PlanInput&>(initial),
              "document undo failed to restore plan");
        document.applyCalculation(jobs::calculate(*document.project()));
        {
            jobs::MainWindow window(document);
            window.show();
            app.processEvents();
            auto* table = taskTable(window);
            auto* undo = window.findChild<QAction*>("undoEdit");
            auto* redo = window.findChild<QAction*>("redoEdit");
            check(table && undo && redo && table->rowCount() == count, "UI controls missing");
            // Scroll the edited row into view. No autosave controller is enabled in this probe.
            table->setCurrentCell(0, 0);
            table->scrollTo(cell(table, 0, 0));
            app.processEvents();
            for (int i = 0; i < repeats; ++i) {
                const auto name = QString("Visible edit %1").arg(i);
                measure("ui_name_commit_events", [&] { setCellText(table, 0, 0, name); }, true);
                check(QString::fromStdString(document.project()->tasks[0].name) == name, "UI name edit lost");
                measure("ui_undo_events", [&] { undo->trigger(); }, true);
                measure("ui_redo_events", [&] { redo->trigger(); }, true);
                undo->trigger(); app.processEvents();
                measure("ui_duration_commit_events", [&] { setCellText(table, 0, 1, QString::number(2 + i)); }, true);
                check(document.project()->tasks[0].durationMinutes == (2 + i) * 60, "UI duration edit lost");
                measure("ui_duration_undo_events", [&] { undo->trigger(); }, true);
                measure("ui_duration_redo_events", [&] { redo->trigger(); }, true);
                undo->trigger(); app.processEvents();
            }
            window.hide();
        }
        document.open(path);
        for (int i = 0; i < 100; ++i) document.renameTask(std::size_t(i % count), "Retained edit " + std::to_string(i));
        const auto historyBytes = document.historyBytes();
        int retained = 0;
        while (document.canUndo()) { document.undo(); ++retained; }
        if (parser.isSet("require-100")) check(retained == 100, "100 edits did not fit the history budget");
        if (retained == 100)
            check(static_cast<const jobs::PlanInput&>(*document.project()) == static_cast<const jobs::PlanInput&>(initial),
                  "100 undos did not restore the complete plan");
        check(document.project()->baselines == initial.baselines && document.project()->progress == initial.progress &&
              document.project()->costEntries == initial.costEntries, "editing changed immutable facts");
        QJsonObject measurements;
        for (const auto& [name, values] : timings) measurements[name] = samples(values);
        auto runtimeRevision = git({"rev-parse", "HEAD"});
        const auto runtimeDirty = git({"status", "--porcelain"});
        if (!runtimeDirty.isEmpty()) runtimeRevision += "+dirty";
        QJsonObject out{{"status", "success"}, {"fixture", "jobs-perf-editing-v1"}, {"task_count", count},
                        {"baseline_count", baselines}, {"repeats", repeats}, {"measurements", measurements},
                        {"retained_undo_after_100_edits", retained}, {"estimated_history_bytes", double(historyBytes)},
                        {"build_source_revision", JOBS_PERF_REVISION}, {"runtime_source_revision", runtimeRevision},
                        {"built_product_source_sha256", JOBS_SOURCE_SHA256}, {"build_configuration", JOBS_PERF_CONFIG},
                        {"os", QSysInfo::prettyProductName()}, {"qt", qVersion()},
                        {"platform", QApplication::platformName()}, {"logical_cpu_count", QThread::idealThreadCount()},
                        {"scope", "Cell commit + synchronous refresh + processEvents; excludes OS presentation latency, autosave, effort inversions and resource/progress edits"}};
#ifdef _MSC_VER
        out["compiler"] = QString("MSVC %1").arg(_MSC_VER);
#else
        out["compiler"] = __VERSION__;
#endif
        QFile output(parser.value("output"));
        const auto bytes = QJsonDocument(out).toJson();
        check(output.open(QIODevice::WriteOnly) && output.write(bytes) == bytes.size(), "benchmark output failed");
        std::cout << "PASS: tasks=" << count << " baselines=" << baselines << " retained=" << retained << '\n';
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
