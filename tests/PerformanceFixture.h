#pragma once
#include "calculation/Scheduler.h"
#include <QCryptographicHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>
inline jobs::Project performanceFixture(int n, bool effort) {
    jobs::Project p;
    p.documentId = "performance-fixture";
    p.name = "Deterministic performance fixture";
    p.startDate = "2026-10-26";
    p.calculationProfile = effort ? jobs::effortProfile : "native-fs0-workminutes-v1";
    if (effort) {
        p.calendars = {{"pm", "Afternoon", {1, 2, 3, 4, 5}, {{780, 1080}}, {}}};
        for (int r = 0; r < 8; ++r) {
            jobs::Resource resource;
            resource.id = "R" + std::to_string(r);
            resource.name = resource.id;
            if (r % 2)
                resource.calendarId = "pm";
            p.resources.push_back(resource);
        }
    }
    for (int i = 0; i < n; ++i) {
        jobs::Task t{"T" + std::to_string(i), "Task " + std::to_string(i), 60, {}};
        if (i >= 100)
            t.predecessors.push_back("T" + std::to_string(i - 100));
        if (effort) {
            if (i % 3 == 0) {
                t.taskType = jobs::TaskType::FixedWork;
                t.workDriven = true;
                t.workUnitMinutes = 6000;
            } else if (i % 3 == 1)
                t.taskType = jobs::TaskType::FixedUnits;
            if (i % 25 == 0)
                t.breaks = {{"2026-10-26 10:00", "2026-10-27 10:00"}};
            p.assignments.push_back({t.id, "R" + std::to_string(i % 8), 100});
        }
        p.tasks.push_back(std::move(t));
    }
    return p;
}
// All TaskSchedule fields, ordered by fixture task ID, with explicit nulls.
inline QString scheduleFingerprint(const jobs::ScheduleResult &result) {
    QJsonArray rows;
    for (const auto &r : result.tasks) {
        QJsonArray driving;
        for (const auto &id : r.drivingPredecessors)
            driving.append(QString::fromStdString(id));
        rows.append(QJsonArray{QString::fromStdString(r.taskId), qint64(r.earlyStartMinutes),
                               qint64(r.earlyFinishMinutes), qint64(r.lateStartMinutes), qint64(r.lateFinishMinutes),
                               qint64(r.totalFloatMinutes), qint64(r.freeFloatMinutes), r.critical,
                               QString::fromStdString(r.start), QString::fromStdString(r.finish), driving,
                               r.remainingStart ? QJsonValue(QString::fromStdString(*r.remainingStart))
                                                : QJsonValue(QJsonValue::Null),
                               r.completedActual});
    }
    return QString::fromLatin1(
        QCryptographicHash::hash(QJsonDocument(rows).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256)
            .toHex());
}
