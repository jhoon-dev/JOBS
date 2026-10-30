#include <algorithm>
#include "application/Document.h"
#include "calculation/Constraints.h"
#include "calculation/Resources.h"
#include "calculation/Costs.h"
#include "calculation/Reports.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include <QCoreApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>
namespace {
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "expected rejection");
}
jobs::Project sample() {
    jobs::Project p;
    p.documentId = "constraints";
    p.name = "일정 제약";
    p.startDate = "2026-10-26";
    p.calculationProfile = jobs::constraintProfile;
    p.tasks = {{"A", "선행", 480, {}}, {"B", "후행", 240, {"A"}}};
    return p;
}
bool diagnostic(const jobs::ScheduleResult &r, const std::string &code) {
    for (const auto &d : r.diagnostics)
        if (d.code == code)
            return true;
    return false;
}
void date(const jobs::ScheduleResult &r, std::size_t i, const std::string &a, const std::string &b) {
    if (r.status != jobs::CalculationStatus::Valid || r.tasks.at(i).start != a || r.tasks.at(i).finish != b) {
        for (const auto &d : r.diagnostics)
            std::cerr << d.code << ": " << d.code << '\n';
        if (i < r.tasks.size())
            std::cerr << r.tasks[i].start << " -> " << r.tasks[i].finish << '\n';
        throw std::runtime_error("constraint dates wrong");
    }
}
QByteArray read(const QString &path) {
    QFile f(path);
    check(f.open(QIODevice::ReadOnly), "read");
    return f.readAll();
}
void write(const QString &path, const QByteArray &bytes) {
    QFile f(path);
    check(f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size(), "write");
}
void timing(jobs::Task &t, jobs::ConstraintType type, const std::string &at) {
    t.constraintType = type;
    t.constraintAt = at;
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        auto p = sample();
        date(jobs::calculatePlan(p), 1, "2026-10-27 09:00", "2026-10-27 14:00");
        timing(p.tasks[1], jobs::ConstraintType::SNET, "2026-10-28 13:00");
        date(jobs::calculatePlan(p), 1, "2026-10-28 13:00", "2026-10-28 17:00");
        timing(p.tasks[1], jobs::ConstraintType::FNET, "2026-10-28 15:00");
        date(jobs::calculatePlan(p), 1, "2026-10-28 10:00", "2026-10-28 15:00");
        timing(p.tasks[1], jobs::ConstraintType::SNLT, "2026-10-27 10:00");
        date(jobs::calculatePlan(p), 1, "2026-10-27 09:00", "2026-10-27 14:00");
        timing(p.tasks[1], jobs::ConstraintType::SNLT, "2026-10-26 17:00");
        auto conflict = jobs::calculatePlan(p);
        check(conflict.status == jobs::CalculationStatus::Conflict && !conflict.projectFinish &&
                  conflict.tasks.empty() && diagnostic(conflict, "CONSTRAINT_CONFLICT"),
              "upper conflict reported as valid");
        check(conflict.diagnostics[0].taskIds.size() == 2 &&
                  std::find(conflict.diagnostics[0].arguments.begin(), conflict.diagnostics[0].arguments.end(),
                            "2026-10-26 17:00") != conflict.diagnostics[0].arguments.end(),
              "conflict lacks dates/causal IDs");
        timing(p.tasks[1], jobs::ConstraintType::FNLT, "2026-10-27 13:00");
        check(diagnostic(jobs::calculatePlan(p), "CONSTRAINT_CONFLICT"), "finish upper ignored");
        timing(p.tasks[1], jobs::ConstraintType::MSO, "2026-10-27 10:00");
        date(jobs::calculatePlan(p), 1, "2026-10-27 10:00", "2026-10-27 15:00");
        timing(p.tasks[1], jobs::ConstraintType::MSO, "2026-10-27 12:30");
        check(diagnostic(jobs::calculatePlan(p), "CONSTRAINT_CALENDAR"), "fixed nonworking start silently snapped");
        timing(p.tasks[1], jobs::ConstraintType::MFO, "2026-10-27 17:00");
        date(jobs::calculatePlan(p), 1, "2026-10-27 13:00", "2026-10-27 17:00");
        timing(p.tasks[1], jobs::ConstraintType::MFO, "2026-10-27 10:00");
        check(diagnostic(jobs::calculatePlan(p), "CONSTRAINT_CONFLICT"), "fixed finish broke FS");
        timing(p.tasks[1], jobs::ConstraintType::MSO, "2026-10-25 09:00");
        check(diagnostic(jobs::calculatePlan(p), "CONSTRAINT_CONFLICT"), "constraint before project silently moved");
        auto shift = sample();
        shift.startDate = "2026-10-30";
        jobs::TaskCalendar calendar;
        calendar.id = "shift";
        calendar.name = "토요일 포함";
        calendar.weekdays = {1, 2, 3, 4, 5, 6};
        calendar.periods = {{780, 1020}};
        shift.calendars = {calendar};
        shift.tasks[1].calendarId = "shift";
        timing(shift.tasks[1], jobs::ConstraintType::MFO, "2026-10-31 17:00");
        date(jobs::calculatePlan(shift), 1, "2026-10-31 13:00", "2026-10-31 17:00");
        auto late = sample();
        late.tasks = {{"A", "ALAP", 120, {}}, {"B", "ALAP successor", 120, {"A"}}, {"C", "long", 960, {}}};
        late.tasks[0].constraintType = jobs::ConstraintType::ALAP;
        late.tasks[1].constraintType = jobs::ConstraintType::ALAP;
        auto alap = jobs::calculatePlan(late);
        date(alap, 0, "2026-10-27 14:00", "2026-10-27 16:00");
        date(alap, 1, "2026-10-27 16:00", "2026-10-27 18:00");
        check(alap.tasks[0].critical && alap.tasks[1].critical, "ALAP float incorrect");
        auto due = sample();
        due.tasks[1].deadlineAt = "2026-10-26 18:00";
        auto warning = jobs::calculatePlan(due);
        date(warning, 1, "2026-10-27 09:00", "2026-10-27 14:00");
        check(diagnostic(warning, "DEADLINE_MISSED") &&
                  jobs::deadlineSlack(due, due.tasks[1], warning.tasks[1].finish) == -240,
              "deadline changed schedule or lost slack");
        auto invalid = sample();
        invalid.tasks[1].constraintType = static_cast<jobs::ConstraintType>(-1);
        rejects([&] { jobs::validate(invalid); });
        invalid = sample();
        invalid.tasks[1].constraintAt = "2026-10-27 09:00";
        rejects([&] { jobs::validate(invalid); });
        invalid = sample();
        invalid.tasks[1].deadlineAt = "2026-02-30 12:00";
        rejects([&] { jobs::validate(invalid); });
        // Actual facts stay fixed when they violate a date constraint.
        auto forecast = sample();
        forecast.tasks.resize(1);
        timing(forecast.tasks[0], jobs::ConstraintType::MSO, "2026-10-26 09:00");
        forecast.statusAt = "2026-10-26 15:00";
        jobs::ProgressEntry e;
        e.id = "actual";
        e.taskId = "A";
        e.statusAt = *forecast.statusAt;
        e.recordedAt = "2026-10-26T09:00:00Z";
        e.reporter = "PM";
        e.reason = "보고";
        e.sourceDocumentId = forecast.documentId;
        e.recordedRevision = 1;
        e.planDurationMinutes = 480;
        e.actualStart = "2026-10-26 10:00";
        e.actualDurationMinutes = 240;
        e.remainingMinutes = 240;
        forecast.progress = {e};
        auto f = jobs::calculate(forecast);
        check(f.status == jobs::CalculationStatus::Conflict && diagnostic(f, "CONSTRAINT_CONFLICT") &&
                  f.tasks[0].start == *e.actualStart && !f.projectFinish,
              "forecast rewrote actual constraint violation");
        // A resource delay cannot override a hard start, but may miss a soft deadline.
        auto resource = sample();
        resource.tasks[1].predecessors.clear();
        resource.tasks[1].durationMinutes = 480;
        resource.resources = {{"R", "공유", "labor", 100, {}, {}}};
        resource.assignments = {{"A", "R", 100}, {"B", "R", 100}};
        timing(resource.tasks[1], jobs::ConstraintType::MSO, "2026-10-26 09:00");
        auto blocked = jobs::levelResources(resource, "task-order", 20);
        check(blocked.leveled.status == jobs::CalculationStatus::Conflict && !blocked.leveled.projectFinish,
              "resource leveling overrode fixed start");
        resource.tasks[1].constraintType = jobs::ConstraintType::ASAP;
        resource.tasks[1].constraintAt.reset();
        resource.tasks[1].deadlineAt = "2026-10-26 18:00";
        auto leveled = jobs::levelResources(resource, "task-order", 20);
        check(leveled.leveled.status == jobs::CalculationStatus::Valid &&
                  leveled.after.status == jobs::CalculationStatus::Valid,
              "deadline blocked leveling");
        bool missed = false;
        for (const auto &d : leveled.diagnostics)
            missed = missed || d.code == "DEADLINE_MISSED";
        check(missed, "leveling lost deadline warning");
        QTemporaryDir dir;
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("드래그 입력", "2026-10-26", {"작업"});
        doc.setDuration(0, 480);
        auto id = doc.project()->tasks[0].id;
        doc.captureBaseline("기존 계획", "PM", "승인", "2026-10-26T09:00:00Z");
        const auto frozen = doc.project()->baselines[0];
        doc.applyCalculation(jobs::calculate(*doc.project()));
        const auto revision = doc.project()->revision;
        doc.editGantt(doc.project()->documentId, revision, id, false, "2026-10-27 09:00");
        check(doc.project()->tasks[0].constraintType == jobs::ConstraintType::SNET &&
                  doc.project()->tasks[0].durationMinutes == 480 && doc.project()->revision == revision + 1 &&
                  doc.calculation()->status == jobs::CalculationStatus::Stale,
              "move not atomic timing edit");
        doc.undo();
        check(!doc.project()->tasks[0].constraintAt && doc.project()->baselines[0] == frozen, "undo lost baseline");
        doc.redo();
        doc.applyCalculation(jobs::calculate(*doc.project()));
        auto before = *doc.project();
        doc.editGantt(before.documentId, before.revision, id, false, "2026-10-27 09:00");
        check(*doc.project() == before, "same move added history");
        rejects([&] { doc.editGantt(before.documentId, before.revision - 1, id, false, "2026-10-28 09:00"); });
        doc.editGantt(before.documentId, before.revision, id, true, "2026-10-27 14:00");
        check(doc.project()->tasks[0].durationMinutes == 240 &&
                  doc.project()->tasks[0].constraintAt == "2026-10-27 09:00",
              "resize rewrote constraint");
        doc.applyCalculation(jobs::calculate(*doc.project()));
        rejects(
            [&] { doc.editGantt(doc.project()->documentId, doc.project()->revision, id, true, "2026-10-27 09:00"); });
        doc.setTaskTiming(0, jobs::ConstraintType::MFO, "2026-10-28 18:00", "2026-10-27 18:00");
        doc.applyCalculation(jobs::calculate(*doc.project()));
        doc.editGantt(doc.project()->documentId, doc.project()->revision, id, false, "2026-10-29 09:00");
        check(doc.project()->tasks[0].constraintType == jobs::ConstraintType::MFO &&
                  doc.project()->tasks[0].constraintAt == "2026-10-29 14:00",
              "move discarded fixed finish type");
        doc.captureBaseline("제약 계획", "PM", "승인", "2026-10-26T09:01:00Z");
        const auto path = dir.filePath("제약.jobs");
        doc.saveAs(path.toStdString());
        check(store.load(path.toStdString()).project == *doc.project(), "schema8 roundtrip failed");
        auto bytes = read(path);
        check(QJsonDocument::fromJson(bytes).object()["schemaVersion"] == QJsonValue(9), "schema9 missing");
        doc.setTaskTiming(0, jobs::ConstraintType::ASAP, {}, "2026-10-26 18:00");
        jobs::JsonRecoveryStore recovery(dir.filePath("recovery"));
        auto session = recovery.newSession();
        session->checkpoint(doc.recoverySnapshot("2026-10-26T09:02:00Z"));
        session.reset();
        auto scan = recovery.scan();
        auto claim = recovery.claim(scan.candidates[0]);
        check(claim->snapshot.project == *doc.project(), "recovery lost timing");
        claim.reset();
        jobs::Document readonly(store);
        readonly.open(path.toStdString(), true);
        rejects([&] { readonly.setTaskTiming(0, jobs::ConstraintType::ASAP, {}, {}); });
        // A native schema7 snapshot remains original-preserving and reproducible.
        jobs::Document legacy(store);
        legacy.create("v7", "2026-10-26", {"A"});
        legacy.setDuration(0, 60);
        legacy.putCalendar({{}, "old", {1, 2, 3, 4, 5}, {{540, 720}, {780, 1080}}, {}});
        auto original = dir.filePath("v7.jobs");
        legacy.saveAs(original.toStdString());
        auto root = QJsonDocument::fromJson(read(original)).object();
        root["schemaVersion"] = 7;
        write(original, QJsonDocument(root).toJson());
        auto oldBytes = read(original);
        jobs::Document migrated(store);
        migrated.open(original.toStdString(), true);
        check(migrated.requiresSaveAs() && jobs::calculatePlan(*migrated.project()).engineVersion == "jobs-pdm-1.2",
              "migrated plan did not select the current PDM engine");
        migrated.saveAs(dir.filePath("v8.jobs").toStdString());
        check(read(original) == oldBytes, "migration overwrote original");
        jobs::ReportOptions options;
        options.reportId = "constraints";
        options.generatedAt = "2026-10-26T09:00:00Z";
        auto report = jobs::buildReport(due, options);
        bool timingTable = false;
        for (const auto &table : report.tables)
            if (table.id == "constraints")
                timingTable = table.rows.size() == 2;
        check(timingTable, "report lacks constraints/deadline state");
        std::cout << "PASS: 8 constraints/ALAP chains, fixed-calendar and causal conflicts, soft deadlines, actual "
                     "preservation, leveling, atomic guarded drag/undo, schema8 and old engine recovery/report\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
