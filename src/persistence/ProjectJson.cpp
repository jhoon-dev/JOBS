#include "formatting/QtDiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "persistence/ProjectJson.h"
#include "calculation/Tracking.h"
#include <QJsonArray>
#include <cmath>
#include <stdexcept>
namespace jobs::json {
namespace {
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
std::string str(const QString &s) {
    return s.toUtf8().toStdString();
}

void keys(const QJsonObject &o, const QStringList &allowed) {
    for (const auto &key : o.keys())
        if (!allowed.contains(key))
            throw AppError(ErrorCode::FILE_UNKNOWN_FIELD, {diagnosticArgument(key)});
}
std::string string(const QJsonObject &o, const QString &key) {
    if (!o.value(key).isString())
        throw AppError(ErrorCode::FILE_PROJECTJSON_002, {diagnosticArgument(key)});
    return str(o.value(key).toString());
}
std::optional<std::string> optionalString(const QJsonObject &o, const QString &key) {
    if (!o.contains(key))
        throw AppError(ErrorCode::FILE_PROJECTJSON_003, {diagnosticArgument(key)});
    if (o.value(key).isNull())
        return std::nullopt;
    return string(o, key);
}
std::int64_t integer(const QJsonObject &o, const QString &key, double maximum = 9007199254740991.0) {
    auto value = o.value(key);
    double n = value.toDouble(-1);
    if (!value.isDouble() || !std::isfinite(n) || n < 0 || n > maximum || std::floor(n) != n)
        throw AppError(ErrorCode::FILE_PROJECTJSON_004, {diagnosticArgument(key)});
    return static_cast<std::int64_t>(n);
}
bool boolean(const QJsonObject &o, const QString &key) {
    if (!o.value(key).isBool())
        throw AppError(ErrorCode::FILE_PROJECTJSON_005, {diagnosticArgument(key)});
    return o.value(key).toBool();
}
QJsonArray array(const QJsonObject &o, const QString &key) {
    if (!o.value(key).isArray())
        throw AppError(ErrorCode::FILE_PROJECTJSON_006, {diagnosticArgument(key)});
    return o.value(key).toArray();
}
QJsonObject object(const QJsonValue &value) {
    if (!value.isObject())
        throw AppError(ErrorCode::FILE_PROJECTJSON_007, {});
    return value.toObject();
}
QJsonArray strings(const std::vector<std::string> &values) {
    QJsonArray a;
    for (const auto &s : values)
        a.append(qs(s));
    return a;
}
std::vector<std::string> readStrings(const QJsonObject &o, const QString &key) {
    std::vector<std::string> result;
    for (const auto &v : array(o, key)) {
        if (!v.isString())
            throw AppError(ErrorCode::FILE_PROJECTJSON_008, {diagnosticArgument(key)});
        result.push_back(str(v.toString()));
    }
    return result;
}
QJsonValue nullable(const std::optional<std::string> &value) {
    return value ? QJsonValue(qs(*value)) : QJsonValue(QJsonValue::Null);
}
QJsonValue money(const std::optional<std::int64_t> &v) {
    return v ? QJsonValue(double(*v)) : QJsonValue(QJsonValue::Null);
}
std::optional<std::int64_t> readMoney(const QJsonObject &o, const QString &key, double limit = double(maxMoney)) {
    if (!o.contains(key))
        throw AppError(ErrorCode::FILE_PROJECTJSON_009, {diagnosticArgument(key)});
    return o[key].isNull() ? std::optional<std::int64_t>{} : integer(o, key, limit);
}
QStringList planKeys(int schema) {
    QStringList result{"name", "startDate", "calendar", "tasks"};
    if (schema >= 2)
        result.append({"holidays", "calculationProfile"});
    if (schema >= 3)
        result.append("wbsNodes");
    if (schema >= 5)
        result.append({"resources", "assignments"});
    if (schema >= 6)
        result.append({"currency", "costProfile", "taskCosts"});
    if (schema >= 7)
        result.append("calendars");
    return result;
}
QJsonObject encodePlan(const PlanInput &p) {
    QJsonArray tasks, nodes;
    for (const auto &t : p.tasks)
        tasks.append(QJsonObject{{"id", qs(t.id)},
                                 {"name", qs(t.name)},
                                 {"wbsId", qs(t.wbsId)},
                                 {"durationMinutes", t.durationMinutes ? QJsonValue(double(*t.durationMinutes))
                                                                       : QJsonValue(QJsonValue::Null)},
                                 {"predecessors", strings(t.predecessors)}});
    if (isPdm(p))
        for (int i = 0; i < tasks.size(); ++i) {
            auto t = tasks[i].toObject();
            const auto &task = p.tasks[std::size_t(i)];
            QJsonArray edges;
            for (const auto &d : task.dependencyRules)
                edges.append(QJsonObject{{"predecessorId", qs(d.predecessorId)},
                                         {"type", qs(enumToken(d.type))},
                                         {"lagMinutes", double(d.lagMinutes)}});
            t["dependencyRules"] = edges;
            t["calendarId"] = qs(task.calendarId);
            tasks[i] = t;
            if (hasConstraints(p)) {
                t["constraintType"] = qs(enumToken(task.constraintType));
                t["constraintAt"] = nullable(task.constraintAt);
                t["deadlineAt"] = nullable(task.deadlineAt);
            }
            if (isEffort(p)) {
                QJsonArray gaps;
                for (const auto &b : task.breaks)
                    gaps.append(QJsonObject{{"start", qs(b.start)}, {"finish", qs(b.finish)}});
                t["taskType"] = qs(enumToken(task.taskType));
                t["workDriven"] = task.workDriven;
                t["workUnitMinutes"] = money(task.workUnitMinutes);
                t["breaks"] = gaps;
            }
            tasks[i] = t;
        }
    for (const auto &n : p.wbsNodes)
        nodes.append(QJsonObject{{"id", qs(n.id)}, {"name", qs(n.name)}, {"parentId", qs(n.parentId)}});
    QJsonArray resources, assignments;
    for (const auto &r : p.resources) {
        QJsonArray exceptions;
        for (const auto &d : r.exceptions)
            exceptions.append(QJsonObject{{"date", qs(d.date)},
                                          {"units", d.units ? QJsonValue(*d.units) : QJsonValue(QJsonValue::Null)}});
        QJsonArray rates;
        for (const auto &rate : r.rates)
            rates.append(
                QJsonObject{{"effectiveDate", qs(rate.effectiveDate)}, {"hourlyMinor", money(rate.hourlyMinor)}});
        resources.append(QJsonObject{{"id", qs(r.id)},
                                     {"name", qs(r.name)},
                                     {"type", qs(r.type)},
                                     {"maxUnits", r.maxUnits ? QJsonValue(*r.maxUnits) : QJsonValue(QJsonValue::Null)},
                                     {"exceptions", exceptions},
                                     {"rates", rates}});
        if (isEffort(p)) {
            auto resource = resources.last().toObject();
            resource["calendarId"] = qs(r.calendarId);
            resources[resources.size() - 1] = resource;
        }
    }
    for (const auto &a : p.assignments)
        assignments.append(QJsonObject{{"taskId", qs(a.taskId)}, {"resourceId", qs(a.resourceId)}, {"units", a.units}});
    QJsonArray costs;
    for (const auto &c : p.taskCosts)
        costs.append(QJsonObject{{"taskId", qs(c.taskId)},
                                 {"fixedMinor", money(c.fixedMinor)},
                                 {"accrual", qs(c.accrual)},
                                 {"credit", qs(c.credit)}});
    QJsonObject result{{"name", qs(p.name)},
                       {"startDate", qs(p.startDate)},
                       {"calendar", qs(p.calendar)},
                       {"tasks", tasks},
                       {"holidays", strings(p.holidays)},
                       {"calculationProfile", qs(p.calculationProfile)},
                       {"wbsNodes", nodes},
                       {"resources", resources},
                       {"assignments", assignments},
                       {"currency", qs(p.currency)},
                       {"costProfile", qs(p.costProfile)},
                       {"taskCosts", costs}};
    if (isPdm(p)) {
        QJsonArray calendars;
        for (const auto &c : p.calendars) {
            QJsonArray weekdays, periods;
            for (int day : c.weekdays)
                weekdays.append(day);
            for (const auto &s : c.periods)
                periods.append(QJsonObject{{"begin", s.begin}, {"end", s.end}});
            calendars.append(QJsonObject{{"id", qs(c.id)},
                                         {"name", qs(c.name)},
                                         {"weekdays", weekdays},
                                         {"periods", periods},
                                         {"holidays", strings(c.holidays)}});
        }
        result["calendars"] = calendars;
    }
    return result;
}
PlanInput decodePlan(const QJsonObject &o, int schema) {
    keys(o, planKeys(schema));
    PlanInput p;
    p.name = string(o, "name");
    p.startDate = string(o, "startDate");
    p.calendar = string(o, "calendar");
    if (schema >= 2) {
        p.holidays = readStrings(o, "holidays");
        p.calculationProfile = string(o, "calculationProfile");
    }
    const bool pdm = schema >= 7 && isPdm(p);
    const bool constrained = schema >= 8 && hasConstraints(p);
    const bool effort = schema >= 9 && isEffort(p);
    if (isEffort(p) && !effort)
        throw AppError(ErrorCode::FILE_PROJECTJSON_010, {});
    if (isPdm(p) && !pdm)
        throw AppError(ErrorCode::FILE_PROJECTJSON_011, {});
    if (hasConstraints(p) && !constrained)
        throw AppError(ErrorCode::FILE_PROJECTJSON_012, {});
    if (!pdm && o.contains("calendars"))
        throw AppError(ErrorCode::FILE_PROJECTJSON_013, {});
    if (pdm)
        for (const auto &v : array(o, "calendars")) {
            auto c = object(v);
            keys(c, {"id", "name", "weekdays", "periods", "holidays"});
            TaskCalendar cal;
            cal.id = string(c, "id");
            cal.name = string(c, "name");
            cal.weekdays.clear();
            cal.periods.clear();
            for (const auto &day : array(c, "weekdays")) {
                QJsonObject d{{"day", day}};
                cal.weekdays.push_back(int(integer(d, "day", 7)));
            }
            for (const auto &period : array(c, "periods")) {
                auto s = object(period);
                keys(s, {"begin", "end"});
                cal.periods.push_back({int(integer(s, "begin", 1440)), int(integer(s, "end", 1440))});
            }
            cal.holidays = readStrings(c, "holidays");
            p.calendars.push_back(std::move(cal));
        }
    if (schema >= 3)
        for (const auto &v : array(o, "wbsNodes")) {
            auto n = object(v);
            keys(n, {"id", "name", "parentId"});
            p.wbsNodes.push_back({string(n, "id"), string(n, "name"), string(n, "parentId")});
        }
    if (schema >= 5) {
        for (const auto &v : array(o, "resources")) {
            auto r = object(v);
            QStringList fields{"id", "name", "type", "maxUnits", "exceptions"};
            if (schema >= 6)
                fields.append("rates");
            if (effort)
                fields.append("calendarId");
            keys(r, fields);
            Resource resource;
            resource.id = string(r, "id");
            resource.name = string(r, "name");
            resource.type = string(r, "type");
            if (effort)
                resource.calendarId = string(r, "calendarId");
            if (!r.contains("maxUnits"))
                throw AppError(ErrorCode::FILE_PROJECTJSON_014, {});
            resource.maxUnits = r["maxUnits"].isNull() ? std::optional<int>{} : int(integer(r, "maxUnits", 100000));
            for (const auto &x : array(r, "exceptions")) {
                auto d = object(x);
                keys(d, {"date", "units"});
                if (!d.contains("units"))
                    throw AppError(ErrorCode::FILE_PROJECTJSON_015, {});
                resource.exceptions.push_back(
                    {string(d, "date"), d["units"].isNull() ? std::optional<int>{} : int(integer(d, "units", 100000))});
            }
            if (schema >= 6)
                for (const auto &x : array(r, "rates")) {
                    auto rate = object(x);
                    keys(rate, {"effectiveDate", "hourlyMinor"});
                    resource.rates.push_back(
                        {string(rate, "effectiveDate"), readMoney(rate, "hourlyMinor", 1000000000)});
                }
            p.resources.push_back(std::move(resource));
        }
        for (const auto &v : array(o, "assignments")) {
            auto a = object(v);
            keys(a, {"taskId", "resourceId", "units"});
            p.assignments.push_back({string(a, "taskId"), string(a, "resourceId"), int(integer(a, "units", 100000))});
        }
    }
    if (schema >= 6) {
        p.currency = string(o, "currency");
        p.costProfile = string(o, "costProfile");
        for (const auto &v : array(o, "taskCosts")) {
            auto c = object(v);
            keys(c, {"taskId", "fixedMinor", "accrual", "credit"});
            p.taskCosts.push_back(
                {string(c, "taskId"), readMoney(c, "fixedMinor"), string(c, "accrual"), string(c, "credit")});
        }
    }
    for (const auto &v : array(o, "tasks")) {
        auto t = object(v);
        QStringList fields{"id", "name"};
        if (schema >= 2)
            fields.append({"durationMinutes", "predecessors"});
        if (schema >= 3)
            fields.append("wbsId");
        if (pdm)
            fields.append({"dependencyRules", "calendarId"});
        if (constrained)
            fields.append({"constraintType", "constraintAt", "deadlineAt"});
        if (effort)
            fields.append({"taskType", "workDriven", "workUnitMinutes", "breaks"});
        keys(t, fields);
        Task task{string(t, "id"), string(t, "name"), std::nullopt, {}};
        if (schema >= 2) {
            if (!t.contains("durationMinutes"))
                throw AppError(ErrorCode::FILE_PROJECTJSON_016, {});
            if (!t.value("durationMinutes").isNull())
                task.durationMinutes = integer(t, "durationMinutes", 5000000);
            task.predecessors = readStrings(t, "predecessors");
        }
        if (schema >= 3)
            task.wbsId = string(t, "wbsId");
        if (pdm) {
            task.calendarId = string(t, "calendarId");
            for (const auto &edgeValue : array(t, "dependencyRules")) {
                auto d = object(edgeValue);
                keys(d, {"predecessorId", "type", "lagMinutes"});
                const auto lag = d["lagMinutes"].toDouble(9000000);
                if (!d["lagMinutes"].isDouble() || !std::isfinite(lag) || std::floor(lag) != lag ||
                    std::abs(lag) > 5000000)
                    throw AppError(ErrorCode::FILE_PROJECTJSON_017, {});
                task.dependencyRules.push_back(
                    {string(d, "predecessorId"), dependencyTypeFromToken(string(d, "type")), std::int64_t(lag)});
            }
        }
        if (constrained) {
            task.constraintType = constraintTypeFromToken(string(t, "constraintType"));
            task.constraintAt = optionalString(t, "constraintAt");
            task.deadlineAt = optionalString(t, "deadlineAt");
        }
        if (effort) {
            task.taskType = taskTypeFromToken(string(t, "taskType"));
            task.workDriven = boolean(t, "workDriven");
            task.workUnitMinutes = readMoney(t, "workUnitMinutes", double(maxWork));
            for (const auto &gap : array(t, "breaks")) {
                auto b = object(gap);
                keys(b, {"start", "finish"});
                task.breaks.push_back({string(b, "start"), string(b, "finish")});
            }
        }
        p.tasks.push_back(std::move(task));
    }
    return p;
}
QJsonObject encodeRow(const TaskSchedule &r) {
    return {{"taskId", qs(r.taskId)},
            {"start", qs(r.start)},
            {"finish", qs(r.finish)},
            {"earlyStartMinutes", double(r.earlyStartMinutes)},
            {"earlyFinishMinutes", double(r.earlyFinishMinutes)},
            {"lateStartMinutes", double(r.lateStartMinutes)},
            {"lateFinishMinutes", double(r.lateFinishMinutes)},
            {"totalFloatMinutes", double(r.totalFloatMinutes)},
            {"freeFloatMinutes", double(r.freeFloatMinutes)},
            {"critical", r.critical},
            {"drivingPredecessors", strings(r.drivingPredecessors)},
            {"remainingStart", nullable(r.remainingStart)},
            {"completedActual", r.completedActual}};
}
TaskSchedule decodeRow(const QJsonObject &o) {
    keys(o, {"taskId", "start", "finish", "earlyStartMinutes", "earlyFinishMinutes", "lateStartMinutes",
             "lateFinishMinutes", "totalFloatMinutes", "freeFloatMinutes", "critical", "drivingPredecessors",
             "remainingStart", "completedActual"});
    TaskSchedule r;
    r.taskId = string(o, "taskId");
    r.start = string(o, "start");
    r.finish = string(o, "finish");
    r.earlyStartMinutes = integer(o, "earlyStartMinutes");
    r.earlyFinishMinutes = integer(o, "earlyFinishMinutes");
    r.lateStartMinutes = integer(o, "lateStartMinutes");
    r.lateFinishMinutes = integer(o, "lateFinishMinutes");
    r.totalFloatMinutes = integer(o, "totalFloatMinutes");
    r.freeFloatMinutes = integer(o, "freeFloatMinutes");
    r.critical = boolean(o, "critical");
    r.drivingPredecessors = readStrings(o, "drivingPredecessors");
    r.remainingStart = optionalString(o, "remainingStart");
    r.completedActual = boolean(o, "completedActual");
    return r;
}
QJsonObject encodeBaseline(const Baseline &b) {
    QJsonArray schedule;
    for (const auto &row : b.schedule)
        schedule.append(encodeRow(row));
    return {{"id", qs(b.id)},
            {"name", qs(b.name)},
            {"approvedBy", qs(b.approvedBy)},
            {"reason", qs(b.reason)},
            {"createdAt", qs(b.createdAt)},
            {"sourceDocumentId", qs(b.sourceDocumentId)},
            {"inputRevision", double(b.inputRevision)},
            {"version", double(b.version)},
            {"input", encodePlan(b.input)},
            {"engineVersion", qs(b.engineVersion)},
            {"projectFinish", qs(b.projectFinish)},
            {"schedule", schedule}};
}
Baseline decodeBaseline(const QJsonObject &o, int schema) {
    keys(o, {"id", "name", "approvedBy", "reason", "createdAt", "sourceDocumentId", "inputRevision", "version", "input",
             "engineVersion", "projectFinish", "schedule"});
    Baseline b;
    b.id = string(o, "id");
    b.name = string(o, "name");
    b.approvedBy = string(o, "approvedBy");
    b.reason = string(o, "reason");
    b.createdAt = string(o, "createdAt");
    b.sourceDocumentId = string(o, "sourceDocumentId");
    b.inputRevision = std::uint64_t(integer(o, "inputRevision"));
    b.version = std::uint64_t(integer(o, "version"));
    b.input = decodePlan(object(o.value("input")), schema);
    b.engineVersion = string(o, "engineVersion");
    b.projectFinish = string(o, "projectFinish");
    for (const auto &v : array(o, "schedule"))
        b.schedule.push_back(decodeRow(object(v)));
    return b;
}
QJsonObject encodeEntry(const ProgressEntry &e) {
    QJsonObject out{{"id", qs(e.id)},
                    {"taskId", qs(e.taskId)},
                    {"statusAt", qs(e.statusAt)},
                    {"recordedAt", qs(e.recordedAt)},
                    {"reporter", qs(e.reporter)},
                    {"reason", qs(e.reason)},
                    {"previousEntryId", qs(e.previousEntryId)},
                    {"recordedRevision", double(e.recordedRevision)},
                    {"sourceDocumentId", qs(e.sourceDocumentId)},
                    {"planDurationMinutes",
                     e.planDurationMinutes ? QJsonValue(double(*e.planDurationMinutes)) : QJsonValue(QJsonValue::Null)},
                    {"actualStart", nullable(e.actualStart)},
                    {"actualFinish", nullable(e.actualFinish)},
                    {"actualDurationMinutes", double(e.actualDurationMinutes)},
                    {"remainingMinutes", double(e.remainingMinutes)}};
    if (e.remainingWorkUnitMinutes)
        out["remainingWorkUnitMinutes"] = double(*e.remainingWorkUnitMinutes);
    return out;
}
ProgressEntry decodeEntry(const QJsonObject &o, int schema) {
    QStringList fields{"id",
                       "taskId",
                       "statusAt",
                       "recordedAt",
                       "reporter",
                       "reason",
                       "previousEntryId",
                       "recordedRevision",
                       "sourceDocumentId",
                       "planDurationMinutes",
                       "actualStart",
                       "actualFinish",
                       "actualDurationMinutes",
                       "remainingMinutes"};
    if (schema >= 9)
        fields.append("remainingWorkUnitMinutes");
    keys(o, fields);
    ProgressEntry e;
    e.id = string(o, "id");
    e.taskId = string(o, "taskId");
    e.statusAt = string(o, "statusAt");
    e.recordedAt = string(o, "recordedAt");
    e.reporter = string(o, "reporter");
    e.reason = string(o, "reason");
    e.previousEntryId = string(o, "previousEntryId");
    e.recordedRevision = std::uint64_t(integer(o, "recordedRevision"));
    e.sourceDocumentId = string(o, "sourceDocumentId");
    if (!o.contains("planDurationMinutes"))
        throw AppError(ErrorCode::FILE_PROJECTJSON_018, {});
    if (!o.value("planDurationMinutes").isNull())
        e.planDurationMinutes = integer(o, "planDurationMinutes", 5000000);
    e.actualStart = optionalString(o, "actualStart");
    e.actualFinish = optionalString(o, "actualFinish");
    if (o.contains("remainingWorkUnitMinutes"))
        e.remainingWorkUnitMinutes = integer(o, "remainingWorkUnitMinutes", double(maxWork));
    e.actualDurationMinutes = integer(o, "actualDurationMinutes", 5000000);
    e.remainingMinutes = integer(o, "remainingMinutes", 5000000);
    return e;
}
QJsonObject encodeScenario(const LevelingScenario &s) {
    QJsonArray entries;
    for (const auto &e : s.progress)
        entries.append(encodeEntry(e));
    return {{"id", qs(s.id)},
            {"name", qs(s.name)},
            {"createdAt", qs(s.createdAt)},
            {"sourceDocumentId", qs(s.sourceDocumentId)},
            {"inputRevision", double(s.inputRevision)},
            {"policy", qs(s.policy)},
            {"priority", qs(s.priority)},
            {"horizonWorkDays", s.horizonWorkDays},
            {"input", encodePlan(s.input)},
            {"statusAt", nullable(s.statusAt)},
            {"progress", entries}};
}
LevelingScenario decodeScenario(const QJsonObject &o, int schema) {
    keys(o, {"id", "name", "createdAt", "sourceDocumentId", "inputRevision", "policy", "priority", "horizonWorkDays",
             "input", "statusAt", "progress"});
    LevelingScenario s;
    s.id = string(o, "id");
    s.name = string(o, "name");
    s.createdAt = string(o, "createdAt");
    s.sourceDocumentId = string(o, "sourceDocumentId");
    s.inputRevision = std::uint64_t(integer(o, "inputRevision"));
    s.policy = string(o, "policy");
    s.priority = string(o, "priority");
    s.horizonWorkDays = int(integer(o, "horizonWorkDays", 5200));
    s.input = decodePlan(object(o["input"]), schema);
    s.statusAt = optionalString(o, "statusAt");
    for (const auto &v : array(o, "progress"))
        s.progress.push_back(decodeEntry(object(v), schema));
    return s;
}
QJsonObject encodeCostEntry(const CostEntry &e) {
    return {{"id", qs(e.id)},
            {"taskId", qs(e.taskId)},
            {"statusAt", qs(e.statusAt)},
            {"recordedAt", qs(e.recordedAt)},
            {"reporter", qs(e.reporter)},
            {"reason", qs(e.reason)},
            {"previousEntryId", qs(e.previousEntryId)},
            {"sourceDocumentId", qs(e.sourceDocumentId)},
            {"recordedRevision", double(e.recordedRevision)},
            {"currency", qs(e.currency)},
            {"actualMinor", money(e.actualMinor)},
            {"physicalBasisPoints",
             e.physicalBasisPoints ? QJsonValue(*e.physicalBasisPoints) : QJsonValue(QJsonValue::Null)}};
}
CostEntry decodeCostEntry(const QJsonObject &o) {
    keys(o, {"id", "taskId", "statusAt", "recordedAt", "reporter", "reason", "previousEntryId", "sourceDocumentId",
             "recordedRevision", "currency", "actualMinor", "physicalBasisPoints"});
    CostEntry e;
    e.id = string(o, "id");
    e.taskId = string(o, "taskId");
    e.statusAt = string(o, "statusAt");
    e.recordedAt = string(o, "recordedAt");
    e.reporter = string(o, "reporter");
    e.reason = string(o, "reason");
    e.previousEntryId = string(o, "previousEntryId");
    e.sourceDocumentId = string(o, "sourceDocumentId");
    e.recordedRevision = std::uint64_t(integer(o, "recordedRevision"));
    e.currency = string(o, "currency");
    e.actualMinor = readMoney(o, "actualMinor");
    if (!o.contains("physicalBasisPoints"))
        throw AppError(ErrorCode::FILE_PROJECTJSON_019, {});
    if (!o["physicalBasisPoints"].isNull())
        e.physicalBasisPoints = int(integer(o, "physicalBasisPoints", 10000));
    return e;
}
} // namespace
QJsonObject encodeProject(const Project &p) {
    auto o = encodePlan(p);
    o["documentId"] = qs(p.documentId);
    o["derivedFrom"] = qs(p.derivedFrom);
    o["revision"] = double(p.revision);
    o["statusAt"] = nullable(p.statusAt);
    QJsonArray baselines, progress;
    for (const auto &b : p.baselines)
        baselines.append(encodeBaseline(b));
    for (const auto &e : p.progress)
        progress.append(encodeEntry(e));
    QJsonArray scenarios;
    for (const auto &s : p.scenarios)
        scenarios.append(encodeScenario(s));
    QJsonArray costs;
    for (const auto &e : p.costEntries)
        costs.append(encodeCostEntry(e));
    o["baselines"] = baselines;
    o["progress"] = progress;
    o["scenarios"] = scenarios;
    o["costEntries"] = costs;
    return o;
}
Project decodeProject(const QJsonObject &o, int schema) {
    auto fields = planKeys(schema);
    fields.append({"documentId", "derivedFrom", "revision"});
    if (schema >= 4)
        fields.append({"statusAt", "baselines", "progress"});
    if (schema >= 5)
        fields.append("scenarios");
    if (schema >= 6)
        fields.append("costEntries");
    keys(o, fields);
    auto plan = o;
    for (const auto &key :
         {"documentId", "derivedFrom", "revision", "statusAt", "baselines", "progress", "scenarios", "costEntries"})
        plan.remove(key);
    Project p;
    static_cast<PlanInput &>(p) = decodePlan(plan, schema);
    p.documentId = string(o, "documentId");
    p.derivedFrom = string(o, "derivedFrom");
    p.revision = std::uint64_t(integer(o, "revision"));
    if (schema >= 4) {
        p.statusAt = optionalString(o, "statusAt");
        for (const auto &b : array(o, "baselines"))
            p.baselines.push_back(decodeBaseline(object(b), schema));
        for (const auto &e : array(o, "progress"))
            p.progress.push_back(decodeEntry(object(e), schema));
    }
    if (schema >= 5)
        for (const auto &v : array(o, "scenarios"))
            p.scenarios.push_back(decodeScenario(object(v), schema));
    if (schema >= 6)
        for (const auto &v : array(o, "costEntries"))
            p.costEntries.push_back(decodeCostEntry(object(v)));
    validate(p);
    for (const auto &b : p.baselines)
        verifyBaseline(b);
    return p;
}
} // namespace jobs::json
