#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include "PerformanceFixture.h"
#include <QCoreApplication>
#include <QFile>
#include <QTemporaryDir>
#include <chrono>
#include <future>
#include <iostream>
#include <thread>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
QByteArray bytes(const std::string &path) {
    QFile file(QString::fromUtf8(path));
    check(file.open(QIODevice::ReadOnly), "read failed");
    return file.readAll();
}
std::shared_ptr<jobs::Document::PreparedEdit>
prepare(jobs::Document &doc, jobs::EditCommand command,
        std::shared_ptr<jobs::CalculationControl> control = std::make_shared<jobs::CalculationControl>()) {
    auto request = doc.makeEditRequest(std::move(command));
    return std::async(std::launch::async,
                      [request = std::move(request), control]() mutable {
                          return jobs::Document::prepareEdit(std::move(request), control);
                      })
        .get();
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        using jobs::EditState;
        QTemporaryDir directory;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("prepared edit", "2026-10-26", {"Work", "Successor"});
        doc.setDuration(0, 480);
        doc.setDuration(1, 60);
        const auto id = doc.project()->tasks[0].id;
        const auto resource = doc.putResource({{}, "Team", "labor", 100, {}});
        doc.setAssignments(id, {{id, resource, 100}});
        doc.setTaskEffort(0, jobs::TaskType::FixedWork, true, 480, 48000, {});
        doc.setPredecessors(1, {id});
        doc.saveAs(directory.filePath("Prepared.jobs").toUtf8().toStdString());
        const auto disk = bytes(doc.path());
        auto before = *doc.project();
        const auto initialHistory = doc.historyBytes();
        auto prepared = prepare(doc, jobs::DurationEdit{id, 240});
        check(prepared->state() == EditState::Ready && *doc.project() == before &&
                  doc.historyBytes() == initialHistory && !doc.dirty(),
              "preparation modified live state/history");
        check(doc.applyPreparedEdit(*prepared) == EditState::Applied &&
                  doc.project()->revision == before.revision + 1 && doc.project()->tasks[0].workUnitMinutes == 48000 &&
                  doc.project()->assignments[0].units == 200,
              "inverse duration did not apply atomically or preserve work");
        check(doc.applyPreparedEdit(*prepared) == EditState::Stale, "prepared edit applied twice");
        const auto edited = static_cast<const jobs::PlanInput &>(*doc.project());
        doc.undo();
        check(static_cast<const jobs::PlanInput &>(*doc.project()) == static_cast<const jobs::PlanInput &>(before),
              "prepared edit was not one undo");
        const auto redo = doc.redoLabel();
        before = *doc.project();
        auto control = std::make_shared<jobs::CalculationControl>();
        prepared = prepare(doc, jobs::DurationEdit{id, 240}, control);
        control->cancel();
        check(doc.applyPreparedEdit(*prepared) == EditState::Cancelled && *doc.project() == before &&
                  doc.redoLabel() == redo,
              "late cancellation changed input or consumed redo");
        control = std::make_shared<jobs::CalculationControl>();
        control->cancel();
        prepared = prepare(doc, jobs::DurationEdit{id, 120}, control);
        check(prepared->state() == EditState::Cancelled && *doc.project() == before,
              "pre-cancel became failure/conflict");
        prepared = prepare(doc, jobs::DurationEdit{id, -1});
        check(prepared->state() == EditState::Failed && !prepared->error().empty() &&
                  doc.applyPreparedEdit(*prepared) == EditState::Failed && *doc.project() == before &&
                  doc.redoLabel() == redo,
              "failed inverse edit changed live state");
        prepared = prepare(doc, jobs::AssignmentEdit{id, doc.project()->assignments});
        check(prepared->state() == EditState::NoChange && doc.applyPreparedEdit(*prepared) == EditState::NoChange &&
                  *doc.project() == before && doc.redoLabel() == redo,
              "no-op lost revision or redo");
        doc.redo();
        check(static_cast<const jobs::PlanInput &>(*doc.project()) == edited, "prepared redo lost assignments");
        prepared = prepare(doc, jobs::DurationEdit{id, 120});
        doc.renameTask(1, "Concurrent edit");
        before = *doc.project();
        check(doc.applyPreparedEdit(*prepared) == EditState::Stale && *doc.project() == before,
              "stale revision replaced a newer edit");
        doc.save();
        prepared = prepare(doc, jobs::DurationEdit{id, 120});
        doc.open(doc.path());
        before = *doc.project();
        check(doc.applyPreparedEdit(*prepared) == EditState::Stale && *doc.project() == before,
              "same ID/revision reload accepted an old session");
        doc.setTaskEffort(0, jobs::TaskType::FixedUnits, false, 480, {}, {});
        prepared = prepare(doc, jobs::AssignmentEdit{id, {{id, resource, 50}}});
        check(doc.applyPreparedEdit(*prepared) == EditState::Applied && doc.project()->tasks[0].workDriven &&
                  doc.project()->tasks[0].workUnitMinutes == 96000 && doc.project()->assignments[0].units == 50,
              "fixed-units assignment did not capture source planned work");
        doc.setStatusAt("2026-10-26 18:00");
        jobs::ProgressEntry entry;
        entry.taskId = id;
        entry.actualStart = "2026-10-26 09:00";
        entry.actualDurationMinutes = 60;
        entry.remainingWorkUnitMinutes = 90000;
        entry.reporter = "PM";
        entry.reason = "Measured";
        entry.recordedAt = "2026-10-27T00:00:00Z";
        prepared = prepare(doc, jobs::ProgressEdit{entry});
        check(doc.project()->progress.empty() && doc.applyPreparedEdit(*prepared) == EditState::Applied &&
                  doc.project()->progress.size() == 1 && !doc.canUndo(),
              "progress applied partially or entered removable history");
        const auto first = doc.project()->progress.front();
        check(first.planDurationMinutes && *first.planDurationMinutes == 1920,
              "progress planned duration was not computed on the source plan");
        prepared = prepare(doc, jobs::ProgressEdit{entry});
        check(doc.applyPreparedEdit(*prepared) == EditState::Applied && doc.project()->progress.front() == first &&
                  doc.project()->progress.back().previousEntryId == first.id &&
                  doc.project()->progress.back().id != first.id,
              "progress correction did not preserve the immutable chain/unique reserved ID");
        check(bytes(doc.path()) != disk, "test save failed");
        const auto saved = bytes(doc.path());
        before = *doc.project();
        prepared = prepare(doc, jobs::EffortEdit{id, jobs::TaskType::FixedWork, true, 480, -1, {}});
        check(prepared->state() == EditState::Failed && *doc.project() == before && bytes(doc.path()) == saved,
              "invalid prepared effort changed the source file/facts");
        auto large = performanceFixture(10000, true);
        const auto largePath = directory.filePath("Large.jobs").toUtf8().toStdString();
        store.save(largePath, large, {});
        jobs::Document big(store);
        big.open(largePath);
        before = *big.project();
        control = std::make_shared<jobs::CalculationControl>();
        auto request = big.makeEditRequest(jobs::DurationEdit{"T0", 120});
        auto future = std::async(std::launch::async, [request = std::move(request), control]() mutable {
            return jobs::Document::prepareEdit(std::move(request), control);
        });
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
        bool running = false;
        while (std::chrono::steady_clock::now() < deadline &&
               future.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
            if (control->stage.load() == jobs::CalculationStage::Forward && control->completed.load() > 10) {
                running = true;
                break;
            }
            std::this_thread::yield();
        }
        control->cancel();
        prepared = future.get();
        check(running && prepared->state() == EditState::Cancelled &&
                  big.applyPreparedEdit(*prepared) == EditState::Cancelled && *big.project() == before &&
                  !big.canUndo() && !big.dirty(),
              "running cancellation produced a partial edit/conflict");
        std::cout << "PASS: sealed worker preparation, one commit/undo, cancellation/late cancel/failure/no-op, source "
                     "session, work preservation, immutable progress and running cancellation\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
