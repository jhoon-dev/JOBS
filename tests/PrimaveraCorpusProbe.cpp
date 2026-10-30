#include "persistence/PrimaveraXer.h"
#include "calculation/Scheduler.h"
#include "formatting/DiagnosticText.h"
#include "formatting/PlanTokens.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <iostream>

namespace {
QString q(const std::string &value) {
    return QString::fromStdString(value);
}
QJsonArray tasks(const jobs::Project &project) {
    QJsonArray rows;
    for (const auto &task : project.tasks) {
        QJsonArray relations;
        for (const auto &edge : jobs::dependencies(task))
            relations.append(QJsonObject{{"predecessor", q(edge.predecessorId)},
                                         {"type", q(jobs::enumToken(edge.type))},
                                         {"lag_minutes", qint64(edge.lagMinutes)}});
        rows.append(QJsonObject{
            {"id", q(task.id)},
            {"name", q(task.name)},
            {"duration_minutes", task.durationMinutes ? QJsonValue(qint64(*task.durationMinutes)) : QJsonValue()},
            {"calendar_id", q(task.calendarId)},
            {"relations", relations},
            {"constraint", q(jobs::enumToken(task.constraintType))}});
    }
    return rows;
}
QJsonArray calendars(const jobs::Project &project) {
    QJsonArray rows;
    for (const auto &calendar : project.calendars) {
        QJsonArray days, periods, holidays;
        for (int day : calendar.weekdays)
            days.append(day);
        for (const auto &period : calendar.periods)
            periods.append(QJsonArray{period.begin, period.end});
        for (const auto &day : calendar.holidays)
            holidays.append(q(day));
        rows.append(QJsonObject{{"id", q(calendar.id)},
                                {"name", q(calendar.name)},
                                {"weekdays", days},
                                {"periods", periods},
                                {"holidays", holidays}});
    }
    return rows;
}
bool sameDates(const jobs::ScheduleResult &a, const jobs::ScheduleResult &b) {
    if (a.status != b.status || a.projectFinish != b.projectFinish || a.tasks.size() != b.tasks.size())
        return false;
    for (std::size_t i = 0; i < a.tasks.size(); ++i)
        if (a.tasks[i].start != b.tasks[i].start || a.tasks[i].finish != b.tasks[i].finish)
            return false;
    return true;
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (app.arguments().size() < 2 || app.arguments().size() > 3)
        return 2;
    const QDir source(app.arguments()[1]);
    QFile manifest(source.filePath("manifest.json"));
    if (!manifest.open(QIODevice::ReadOnly))
        return 2;
    const auto entries = QJsonDocument::fromJson(manifest.readAll()).object()["files"].toArray();
    QJsonArray results;
    bool matched = true;
    for (const auto &entryValue : entries) {
        const auto entry = entryValue.toObject();
        const auto file = entry["file"].toString();
        QJsonObject row{{"file", file}};
        const auto bytes = jobs::readPrimaveraXer(source.filePath(file));
        const auto digest = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        if (digest != entry["sha256"].toString())
            return 2;
        row["sha256"] = digest;
        try {
            const auto encoding =
                entry["encoding"] == "windows-1252" ? jobs::XerEncoding::Windows1252 : jobs::XerEncoding::Utf8;
            auto exchange = jobs::importPrimaveraXer(bytes, file, encoding);
            row["status"] = "imported";
            row["tasks"] = tasks(exchange.project);
            row["calendars"] = calendars(exchange.project);
            row["resources"] = qint64(exchange.project.resources.size());
            row["assignments"] = qint64(exchange.project.assignments.size());
            row["exchange"] = exchange.report;
            const auto schedule = jobs::calculate(exchange.project);
            row["calculation_status"] = int(schedule.status);
            row["finish"] = q(schedule.projectFinish.value_or(""));
            try {
                const auto exported = jobs::exportPrimaveraXer(exchange.project);
                const auto around = jobs::importPrimaveraXer(exported.xer, "round-trip.xer");
                const bool equal = exchange.project.tasks.size() == around.project.tasks.size() &&
                                   sameDates(schedule, jobs::calculate(around.project));
                row["jobs_round_trip_dates_equal"] = equal;
                matched = matched && equal;
                if (app.arguments().size() == 3) {
                    QDir output(app.arguments()[2]);
                    if (!QDir().mkpath(output.absolutePath()))
                        return 2;
                    QFile xer(output.filePath(file));
                    if (!xer.open(QIODevice::WriteOnly | QIODevice::NewOnly) ||
                        xer.write(exported.xer) != exported.xer.size())
                        return 2;
                }
            } catch (const std::exception &error) {
                row["round_trip_error"] = q(jobs::diagnosticFromException(error).code);
                matched = false;
            }
        } catch (const std::exception &error) {
            row["status"] = "rejected";
            row["error_code"] = q(jobs::diagnosticFromException(error).code);
            row["reason"] = q(jobs::errorText(error));
        }
        if (entry.contains("expected_status")) {
            matched = matched && row["status"] == entry["expected_status"];
            if (entry.contains("expected_error"))
                matched = matched && row["error_code"] == entry["expected_error"];
        }
        results.append(row);
    }
    std::cout << QJsonDocument(QJsonObject{{"status", matched ? "passed" : "failed"}, {"files", results}})
                     .toJson()
                     .toStdString();
    return matched ? 0 : 1;
}
