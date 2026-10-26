#include "persistence/ProjectJson.h"
#include "calculation/Tracking.h"
#include <QJsonArray>
#include <cmath>
#include <stdexcept>
namespace jobs::json {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
[[noreturn]] void fail(const QString& message) { throw std::runtime_error(str(message)); }
void keys(const QJsonObject& o, const QStringList& allowed) { for (const auto& key : o.keys()) if (!allowed.contains(key)) fail("지원하지 않는 필드: " + key); }
std::string string(const QJsonObject& o, const QString& key) {
    if (!o.value(key).isString()) fail("문자열 필드 누락 또는 형식 오류: " + key);
    return str(o.value(key).toString());
}
std::optional<std::string> optionalString(const QJsonObject& o, const QString& key) {
    if (!o.contains(key)) fail("필드 누락: " + key);
    if (o.value(key).isNull()) return std::nullopt;
    return string(o, key);
}
std::int64_t integer(const QJsonObject& o, const QString& key, double maximum = 9007199254740991.0) {
    auto value = o.value(key); double n = value.toDouble(-1);
    if (!value.isDouble() || !std::isfinite(n) || n < 0 || n > maximum || std::floor(n) != n) fail("정수 필드 오류: " + key);
    return static_cast<std::int64_t>(n);
}
bool boolean(const QJsonObject& o, const QString& key) {
    if (!o.value(key).isBool()) fail("참/거짓 필드 오류: " + key);
    return o.value(key).toBool();
}
QJsonArray array(const QJsonObject& o, const QString& key) {
    if (!o.value(key).isArray()) fail("목록 필드 누락 또는 형식 오류: " + key);
    return o.value(key).toArray();
}
QJsonObject object(const QJsonValue& value) { if (!value.isObject()) fail("객체 형식 오류"); return value.toObject(); }
QJsonArray strings(const std::vector<std::string>& values) { QJsonArray a; for (const auto& s : values) a.append(qs(s)); return a; }
std::vector<std::string> readStrings(const QJsonObject& o, const QString& key) {
    std::vector<std::string> result;
    for (const auto& v : array(o, key)) { if (!v.isString()) fail("문자열 목록 오류: " + key); result.push_back(str(v.toString())); } return result;
}
QJsonValue nullable(const std::optional<std::string>& value) { return value ? QJsonValue(qs(*value)) : QJsonValue(QJsonValue::Null); }
QStringList planKeys(int schema) {
    QStringList result{"name", "startDate", "calendar", "tasks"};
    if (schema >= 2) result.append({"holidays", "calculationProfile"});
    if (schema >= 3) result.append("wbsNodes");
    return result;
}
QJsonObject encodePlan(const PlanInput& p) {
    QJsonArray tasks, nodes;
    for (const auto& t : p.tasks) tasks.append(QJsonObject{{"id", qs(t.id)}, {"name", qs(t.name)}, {"wbsId", qs(t.wbsId)},
        {"durationMinutes", t.durationMinutes ? QJsonValue(double(*t.durationMinutes)) : QJsonValue(QJsonValue::Null)}, {"predecessors", strings(t.predecessors)}});
    for (const auto& n : p.wbsNodes) nodes.append(QJsonObject{{"id", qs(n.id)}, {"name", qs(n.name)}, {"parentId", qs(n.parentId)}});
    return {{"name", qs(p.name)}, {"startDate", qs(p.startDate)}, {"calendar", qs(p.calendar)}, {"tasks", tasks},
        {"holidays", strings(p.holidays)}, {"calculationProfile", qs(p.calculationProfile)}, {"wbsNodes", nodes}};
}
PlanInput decodePlan(const QJsonObject& o, int schema) {
    keys(o, planKeys(schema)); PlanInput p;
    p.name = string(o, "name"); p.startDate = string(o, "startDate"); p.calendar = string(o, "calendar");
    if (schema >= 2) { p.holidays = readStrings(o, "holidays"); p.calculationProfile = string(o, "calculationProfile"); }
    if (schema >= 3) for (const auto& v : array(o, "wbsNodes")) {
        auto n = object(v); keys(n, {"id", "name", "parentId"}); p.wbsNodes.push_back({string(n, "id"), string(n, "name"), string(n, "parentId")});
    }
    for (const auto& v : array(o, "tasks")) {
        auto t = object(v); QStringList fields{"id", "name"};
        if (schema >= 2) fields.append({"durationMinutes", "predecessors"});
        if (schema >= 3) fields.append("wbsId");
        keys(t, fields);
        Task task{string(t, "id"), string(t, "name"), std::nullopt, {}};
        if (schema >= 2) {
            if (!t.contains("durationMinutes")) fail("기간 필드 누락");
            if (!t.value("durationMinutes").isNull()) task.durationMinutes = integer(t, "durationMinutes", 5000000);
            task.predecessors = readStrings(t, "predecessors");
        }
        if (schema >= 3) task.wbsId = string(t, "wbsId");
        p.tasks.push_back(std::move(task));
    }
    return p;
}
QJsonObject encodeRow(const TaskSchedule& r) {
    return {{"taskId", qs(r.taskId)}, {"start", qs(r.start)}, {"finish", qs(r.finish)},
        {"earlyStartMinutes", double(r.earlyStartMinutes)}, {"earlyFinishMinutes", double(r.earlyFinishMinutes)},
        {"lateStartMinutes", double(r.lateStartMinutes)}, {"lateFinishMinutes", double(r.lateFinishMinutes)},
        {"totalFloatMinutes", double(r.totalFloatMinutes)}, {"freeFloatMinutes", double(r.freeFloatMinutes)},
        {"critical", r.critical}, {"drivingPredecessors", strings(r.drivingPredecessors)},
        {"remainingStart", nullable(r.remainingStart)}, {"completedActual", r.completedActual}};
}
TaskSchedule decodeRow(const QJsonObject& o) {
    keys(o, {"taskId", "start", "finish", "earlyStartMinutes", "earlyFinishMinutes", "lateStartMinutes", "lateFinishMinutes",
        "totalFloatMinutes", "freeFloatMinutes", "critical", "drivingPredecessors", "remainingStart", "completedActual"});
    TaskSchedule r; r.taskId = string(o, "taskId"); r.start = string(o, "start"); r.finish = string(o, "finish");
    r.earlyStartMinutes = integer(o, "earlyStartMinutes"); r.earlyFinishMinutes = integer(o, "earlyFinishMinutes");
    r.lateStartMinutes = integer(o, "lateStartMinutes"); r.lateFinishMinutes = integer(o, "lateFinishMinutes");
    r.totalFloatMinutes = integer(o, "totalFloatMinutes"); r.freeFloatMinutes = integer(o, "freeFloatMinutes");
    r.critical = boolean(o, "critical"); r.drivingPredecessors = readStrings(o, "drivingPredecessors");
    r.remainingStart = optionalString(o, "remainingStart"); r.completedActual = boolean(o, "completedActual"); return r;
}
QJsonObject encodeBaseline(const Baseline& b) {
    QJsonArray schedule; for (const auto& row : b.schedule) schedule.append(encodeRow(row));
    return {{"id", qs(b.id)}, {"name", qs(b.name)}, {"approvedBy", qs(b.approvedBy)}, {"reason", qs(b.reason)}, {"createdAt", qs(b.createdAt)},
        {"sourceDocumentId", qs(b.sourceDocumentId)}, {"inputRevision", double(b.inputRevision)}, {"version", double(b.version)},
        {"input", encodePlan(b.input)}, {"engineVersion", qs(b.engineVersion)}, {"projectFinish", qs(b.projectFinish)}, {"schedule", schedule}};
}
Baseline decodeBaseline(const QJsonObject& o) {
    keys(o, {"id", "name", "approvedBy", "reason", "createdAt", "sourceDocumentId", "inputRevision", "version", "input", "engineVersion", "projectFinish", "schedule"});
    Baseline b; b.id = string(o, "id"); b.name = string(o, "name"); b.approvedBy = string(o, "approvedBy"); b.reason = string(o, "reason");
    b.createdAt = string(o, "createdAt"); b.sourceDocumentId = string(o, "sourceDocumentId"); b.inputRevision = std::uint64_t(integer(o, "inputRevision"));
    b.version = std::uint64_t(integer(o, "version")); b.input = decodePlan(object(o.value("input")), 4);
    b.engineVersion = string(o, "engineVersion"); b.projectFinish = string(o, "projectFinish");
    for (const auto& v : array(o, "schedule")) b.schedule.push_back(decodeRow(object(v)));
    return b;
}
QJsonObject encodeEntry(const ProgressEntry& e) {
    return {{"id", qs(e.id)}, {"taskId", qs(e.taskId)}, {"statusAt", qs(e.statusAt)}, {"recordedAt", qs(e.recordedAt)},
        {"reporter", qs(e.reporter)}, {"reason", qs(e.reason)}, {"previousEntryId", qs(e.previousEntryId)}, {"recordedRevision", double(e.recordedRevision)},
        {"sourceDocumentId", qs(e.sourceDocumentId)}, {"planDurationMinutes", e.planDurationMinutes ? QJsonValue(double(*e.planDurationMinutes)) : QJsonValue(QJsonValue::Null)},
        {"actualStart", nullable(e.actualStart)}, {"actualFinish", nullable(e.actualFinish)}, {"actualDurationMinutes", double(e.actualDurationMinutes)}, {"remainingMinutes", double(e.remainingMinutes)}};
}
ProgressEntry decodeEntry(const QJsonObject& o) {
    keys(o, {"id", "taskId", "statusAt", "recordedAt", "reporter", "reason", "previousEntryId", "recordedRevision", "sourceDocumentId", "planDurationMinutes", "actualStart", "actualFinish", "actualDurationMinutes", "remainingMinutes"});
    ProgressEntry e; e.id = string(o, "id"); e.taskId = string(o, "taskId"); e.statusAt = string(o, "statusAt"); e.recordedAt = string(o, "recordedAt");
    e.reporter = string(o, "reporter"); e.reason = string(o, "reason"); e.previousEntryId = string(o, "previousEntryId"); e.recordedRevision = std::uint64_t(integer(o, "recordedRevision"));
    e.sourceDocumentId = string(o, "sourceDocumentId");
    if (!o.contains("planDurationMinutes")) fail("보고 당시 계획 기간 누락");
    if (!o.value("planDurationMinutes").isNull()) e.planDurationMinutes = integer(o, "planDurationMinutes", 5000000);
    e.actualStart = optionalString(o, "actualStart"); e.actualFinish = optionalString(o, "actualFinish");
    e.actualDurationMinutes = integer(o, "actualDurationMinutes", 5000000); e.remainingMinutes = integer(o, "remainingMinutes", 5000000); return e;
}
}
QJsonObject encodeProject(const Project& p) {
    auto o = encodePlan(p); o["documentId"] = qs(p.documentId); o["derivedFrom"] = qs(p.derivedFrom); o["revision"] = double(p.revision);
    o["statusAt"] = nullable(p.statusAt); QJsonArray baselines, progress;
    for (const auto& b : p.baselines) baselines.append(encodeBaseline(b));
    for (const auto& e : p.progress) progress.append(encodeEntry(e));
    o["baselines"] = baselines; o["progress"] = progress; return o;
}
Project decodeProject(const QJsonObject& o, int schema) {
    auto fields = planKeys(schema); fields.append({"documentId", "derivedFrom", "revision"});
    if (schema >= 4) fields.append({"statusAt", "baselines", "progress"});
    keys(o, fields);
    auto plan = o; for (const auto& key : {"documentId", "derivedFrom", "revision", "statusAt", "baselines", "progress"}) plan.remove(key);
    Project p; static_cast<PlanInput&>(p) = decodePlan(plan, schema);
    p.documentId = string(o, "documentId"); p.derivedFrom = string(o, "derivedFrom"); p.revision = std::uint64_t(integer(o, "revision"));
    if (schema >= 4) {
        p.statusAt = optionalString(o, "statusAt");
        for (const auto& b : array(o, "baselines")) p.baselines.push_back(decodeBaseline(object(b)));
        for (const auto& e : array(o, "progress")) p.progress.push_back(decodeEntry(object(e)));
    }
    validate(p); for (const auto& b : p.baselines) verifyBaseline(b); return p;
}
}
