#include "PerformanceFixture.h"
#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <future>
#include <iostream>

namespace {
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
bool sameInput(jobs::Project a, const jobs::Project& b) { a.revision = b.revision; return a == b; }
void verifyDirty(const jobs::Document& doc, const jobs::Project& saved) {
    check(doc.dirty() == !sameInput(*doc.project(), saved), "incremental dirty disagrees with full value comparison");
}
}
int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir dir; jobs::JsonProjectStore store;
        const auto path = dir.filePath("Responsive.jobs").toUtf8().toStdString();
        store.save(path, performanceFixture(10000, false), {});
        jobs::Document doc(store); doc.open(path);
        for (int i = 0; i < 3; ++i) doc.captureBaseline("B" + std::to_string(i), "PM", "large baseline", "2026-10-28T00:00:00Z");
        doc.save(); auto saved = *doc.project();
        const auto snapshot = doc.snapshot(); check(snapshot.get() == doc.snapshot().get(), "snapshot capture copied input");
        auto oldRequest = doc.makeEditRequest(jobs::DurationEdit{"T9999", 120});
        doc.renameTask(9999, "Late edit");
        check(snapshot->tasks.back().name == "Task 9999" && snapshot->revision == saved.revision,
              "synchronous writer modified an owning request snapshot");
        auto old = std::async(std::launch::async, [request = std::move(oldRequest)]() mutable {
            return jobs::Document::prepareEdit(std::move(request), std::make_shared<jobs::CalculationControl>());
        }).get();
        check(doc.applyPreparedEdit(*old) == jobs::EditState::Stale, "stale snapshot applied");
        check(doc.lastDirtyTaskComparisons() == 2, "name edit scanned saved project"); verifyDirty(doc, saved);
        doc.renameTask(9999, "Task 9999"); check(!doc.dirty(), "manual name restoration stayed dirty");
        doc.undo(); verifyDirty(doc, saved); doc.redo(); verifyDirty(doc, saved);
        for (auto row : {std::size_t(0), std::size_t(5000), std::size_t(9999)}) {
            for (auto minutes : {std::optional<std::int64_t>{}, std::optional<std::int64_t>{0}, std::optional<std::int64_t>{120}, std::optional<std::int64_t>{60}}) {
                doc.setDuration(row, minutes); verifyDirty(doc, saved);
                check(doc.lastDirtyTaskComparisons() == 2, "duration edit scanned saved project");
                doc.undo(); verifyDirty(doc, saved); doc.redo(); verifyDirty(doc, saved);
            }
        }
        const auto beforeInvalid = *doc.project(); const auto weight = doc.historyBytes();
        bool rejected = false; try { doc.setDuration(9999, -1); } catch (const std::exception&) { rejected = true; }
        check(rejected && *doc.project() == beforeInvalid && weight == doc.historyBytes(), "invalid duration partially committed");
        doc.open(path); doc.addTask("Structural change"); doc.renameTask(9999, "still dirty"); verifyDirty(doc, saved);
        doc.undo(); doc.undo(); check(!doc.dirty(), "structural fallback did not restore dirty tracker");
        doc.open(path); doc.setDuration(9999, 120);
        auto request = doc.makeEditRequest(jobs::DurationEdit{"T9999", 60});
        auto result = std::async(std::launch::async, [request = std::move(request)]() mutable {
            return jobs::Document::prepareEdit(std::move(request), std::make_shared<jobs::CalculationControl>());
        }).get();
        // The proposal restores the OLD save point, then a regular save moves it.
        // A cached bool would incorrectly mark the proposal clean here.
        doc.save(); saved = *doc.project();
        check(doc.applyPreparedEdit(*result) == jobs::EditState::Applied && doc.dirty() && doc.lastDirtyTaskComparisons() == 0,
              "prepared dirty did not follow a normal save during preparation");
        verifyDirty(doc, saved); doc.setDuration(9999, 120); check(!doc.dirty(), "prepared task mismatch count was wrong");
        auto retired = result->takeRetiredProjects();
        check(doc.applyPreparedEdit(*result) == jobs::EditState::Stale && !retired.empty(), "retired proposal was reusable");
        doc.setDuration(0, 120); doc.save(); saved = *doc.project();
        for (int i = 0; i < 110; ++i) doc.renameTask(std::size_t(i), "Branch " + std::to_string(i));
        for (int i = 0; i < 100; ++i) { doc.undo(); verifyDirty(doc, saved); }
        check(doc.dirty(), "evicted history falsely became the save point");
        doc.renameTask(5000, "new branch"); check(!doc.canRedo(), "new branch retained redo"); verifyDirty(doc, saved);
        const auto recoveryInput = *doc.project();
        auto recoveryRequest = doc.recoverySnapshotRequest("2026-10-28T00:00:00Z");
        doc.renameTask(9998, "After recovery capture");
        auto recovery = std::async(std::launch::async, [request = std::move(recoveryRequest)] { return request(); }).get();
        check(recovery.project == recoveryInput && recovery.sourcePath == doc.path() && recovery.lastSavedRevision == saved.revision,
              "deferred recovery copy used live input or lost captured metadata");
        doc.saveAs(dir.filePath("Independent.jobs").toUtf8().toStdString()); saved = *doc.project();
        check(!doc.dirty() && sameInput(store.load(doc.path()).project, saved), "independent save point mismatch");
        doc.setDuration(9999, 0); doc.undo(); check(!doc.dirty(), "save-as undo did not return clean");
        auto captured = doc.snapshot(); doc.open(path);
        check(captured->documentId != doc.project()->documentId && captured->baselines.size() == 3,
              "reload invalidated the lifetime of an owning snapshot");
        std::cout << "PASS: shared snapshot isolation, exact bounded dirty comparisons, duration validation/undo, structural fallback, save races, eviction and deferred recovery\n";
    } catch (const std::exception& error) { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
