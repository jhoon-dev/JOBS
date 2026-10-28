#include "application/Document.h"
#include "persistence/JsonProjectStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <array>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <tuple>

namespace {
void check(bool condition, const std::string &message) {
    if (!condition)
        throw std::runtime_error(message);
}
class CountingStore final : public jobs::ProjectStore {
public:
    mutable std::array<unsigned, 6> calls{};
    std::string newId() const override {
        ++calls[0];
        return store_.newId();
    }
    std::string normalizePath(const std::string &path) const override {
        ++calls[4];
        return store_.normalizePath(path);
    }
    bool sameFile(const std::string &left, const std::string &right) const override {
        ++calls[5];
        return store_.sameFile(left, right);
    }
    std::unique_ptr<jobs::WriteLease> lock(const std::string &path) override {
        ++calls[1];
        return store_.lock(path);
    }
    jobs::StoredProject load(const std::string &path) const override {
        ++calls[2];
        return store_.load(path);
    }
    std::string save(const std::string &path, const jobs::Project &project, const std::string &fingerprint) override {
        ++calls[3];
        return store_.save(path, project, fingerprint);
    }

private:
    jobs::JsonProjectStore store_;
};
struct Case {
    const char *name;
    std::function<void(jobs::Document &)> call;
    jobs::ErrorCode code = jobs::ErrorCode::DOCUMENT_NOT_EDITABLE;
};
template <class F> void unchanged(jobs::Document &doc, CountingStore &store, F call) {
    const auto root = doc.snapshot();
    const auto value = root ? std::optional{*root} : std::nullopt;
    const auto state =
        std::tuple{doc.dirty(),        doc.readOnly(), doc.sessionGeneration(), doc.calculationGeneration(),
                   doc.historyBytes(), doc.canUndo(),  doc.canRedo(),           doc.undoLabel(),
                   doc.redoLabel(),    doc.path(),     doc.historyNotice()};
    const auto tasks = doc.calculation() ? doc.calculation()->tasks : std::vector<jobs::TaskSchedule>{};
    const bool hasCalculation = doc.calculation().has_value();
    const auto status = hasCalculation ? doc.calculation()->status : jobs::CalculationStatus::Stale;
    const auto calls = store.calls;
    unsigned events = 0;
    const auto token = doc.subscribe([&](jobs::Document::Event) { ++events; });
    call();
    doc.unsubscribe(token);
    check(events == 0, "rejected edit emitted an event");
    check(store.calls == calls, "rejected edit accessed files or allocated an ID");
    check(doc.snapshot().get() == root.get() && doc.project() == value,
          "rejected edit changed/detached input snapshot");
    check(state == std::tuple{doc.dirty(), doc.readOnly(), doc.sessionGeneration(), doc.calculationGeneration(),
                              doc.historyBytes(), doc.canUndo(), doc.canRedo(), doc.undoLabel(), doc.redoLabel(),
                              doc.path(), doc.historyNotice()},
          "rejected edit changed document state/history");
    check((doc.calculation() ? doc.calculation()->tasks : std::vector<jobs::TaskSchedule>{}) == tasks &&
              doc.calculation().has_value() == hasCalculation &&
              (!hasCalculation || doc.calculation()->status == status),
          "rejected edit changed calculation");
}
void rejected(jobs::Document &doc, CountingStore &store, const Case &test) {
    unchanged(doc, store, [&] {
        bool failed = false;
        try {
            test.call(doc);
        } catch (const jobs::AppError &error) {
            failed = true;
            check(error.code() == jobs::errorCodeToken(test.code),
                  std::string(test.name) + ": wrong rejection priority/code");
        }
        check(failed, std::string(test.name) + ": edit was accepted");
    });
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        QTemporaryDir directory;
        check(directory.isValid(), "temporary directory unavailable");
        CountingStore store;
        const auto path = directory.filePath("Guard.jobs").toStdString();
        {
            jobs::Document source(store);
            source.create("Guard fixture", "2026-10-26", {"Task"});
            source.setDuration(0, 60);
            source.saveAs(path);
        }
        jobs::Document closed(store), readOnly(store);
        readOnly.open(path, true);
        readOnly.applyCalculation(jobs::calculate(*readOnly.project()));
        const std::vector<Case> cases{
            {"renameTask", [](auto &d) { d.renameTask(9999, ""); }},
            {"addTask", [](auto &d) { d.addTask(""); }},
            {"addWbs", [](auto &d) { (void)d.addWbs(""); }},
            {"renameWbs", [](auto &d) { d.renameWbs("missing", ""); }},
            {"moveWbs", [](auto &d) { d.moveWbs("missing", "missing"); }},
            {"removeWbs", [](auto &d) { d.removeWbs("missing"); }},
            {"assignWbs", [](auto &d) { d.assignWbs(9999, "missing"); }},
            {"setDuration", [](auto &d) { d.setDuration(9999, -1); }},
            {"setTaskEffort",
             [](auto &d) { d.setTaskEffort(9999, static_cast<jobs::TaskType>(-1), true, {}, {}, {}); }},
            {"setPredecessors", [](auto &d) { d.setPredecessors(9999, {"missing"}); }},
            {"setDependencies",
             [](auto &d) { d.setDependencies(9999, {{"missing", static_cast<jobs::DependencyType>(-1), 0}}); }},
            {"putCalendar", [](auto &d) { (void)d.putCalendar({}); }},
            {"removeCalendar", [](auto &d) { d.removeCalendar("missing"); }},
            {"setTaskCalendar", [](auto &d) { d.setTaskCalendar(9999, "missing"); }},
            {"setTaskTiming", [](auto &d) { d.setTaskTiming(9999, static_cast<jobs::ConstraintType>(-1), {}, {}); }},
            {"setHolidays", [](auto &d) { d.setHolidays({"invalid"}); }},
            {"setStatusAt", [](auto &d) { d.setStatusAt("invalid"); }},
            {"appendBaseline", [](auto &d) { (void)d.appendBaseline({}); }},
            {"captureBaseline", [](auto &d) { (void)d.captureBaseline("", "", "", "invalid"); }},
            {"setCurrency", [](auto &d) { d.setCurrency("invalid"); }},
            {"setTaskCost", [](auto &d) { d.setTaskCost({}); }},
            {"putResource", [](auto &d) { (void)d.putResource({}); }},
            {"removeResource", [](auto &d) { d.removeResource("missing"); }},
            {"setAssignments", [](auto &d) { d.setAssignments("missing", {}); }},
            {"saveScenario", [](auto &d) { (void)d.saveScenario("", "", 0, "", "", 0); }},
            {"insertTasks", [](auto &d) { (void)d.insertTasks({}); }},
            {"removeTasks", [](auto &d) { d.removeTasks({}); }},
            {"duplicateTasks", [](auto &d) { (void)d.duplicateTasks({}); }},
            {"editTasks", [](auto &d) { d.editTasks({}); }},
            {"makeEditRequest", [](auto &d) { (void)d.makeEditRequest(jobs::DurationEdit{"missing", -1}); }},
            {"recordProgress", [](auto &d) { d.recordProgress({}); }, jobs::ErrorCode::STATUS_DATE_REQUIRED},
            {"recordCost", [](auto &d) { d.recordCost({}); }, jobs::ErrorCode::COST_STATUS_DATE_REQUIRED},
            {"editGantt", [](auto &d) { d.editGantt("", 0, "", false, "invalid"); },
             jobs::ErrorCode::GANTT_REQUIRES_CURRENT_PLAN},
            {"save", [](auto &d) { d.save(); }, jobs::ErrorCode::LEGACY_REQUIRES_SAVE_AS},
            {"recoverySnapshotRequest", [](auto &d) { (void)d.recoverySnapshotRequest("invalid"); },
             jobs::ErrorCode::RECOVERY_SNAPSHOT_INVALID}};
        for (const auto &test : cases) {
            rejected(closed, store, test);
            rejected(readOnly, store, test);
        }
        jobs::Document writable(store);
        writable.open(path);
        auto prepared = jobs::Document::prepareEdit(
            writable.makeEditRequest(jobs::DurationEdit{writable.project()->tasks[0].id, 120}),
            std::make_shared<jobs::CalculationControl>());
        for (auto *target : {&closed, &readOnly})
            unchanged(*target, store, [&] {
                check(target->applyPreparedEdit(*prepared) == jobs::EditState::Stale,
                      "prepared edit bypassed authorization");
            });
        writable.renameTask(0, "Writable");
        check(writable.canUndo(), "writable edit stopped working");
        writable.undo();
        writable.redo();
        for (const auto &test : cases)
            if (std::string_view(test.name) == "recordProgress" || std::string_view(test.name) == "recordCost")
                rejected(writable, store, test);
        const auto taskId = readOnly.project()->tasks[0].id;
        check(readOnly.previewRemoveTasks({taskId}).tasks == 1, "read-only preview was blocked");
        const auto oldId = readOnly.project()->documentId;
        readOnly.saveAs(directory.filePath("Independent.jobs").toStdString());
        check(!readOnly.readOnly() && readOnly.project()->derivedFrom == oldId &&
                  readOnly.project()->documentId != oldId,
              "read-only independent copy was blocked");
        std::cout << "PASS: " << cases.size() * 2
                  << " closed/read-only guard cases, special messages, no input/history/snapshot/IO/ID effects, "
                     "stale preparation, writable undo/redo and independent copy\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
