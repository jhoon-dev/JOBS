#include "formatting/QtDiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "persistence/ProjectXml.h"
#include "persistence/ExchangePackage.h"
#include "persistence/JsonProjectStore.h"
#include <QCryptographicHash>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QUuid>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>

namespace jobs {
namespace {
constexpr auto ns = "http://schemas.microsoft.com/project";
constexpr qsizetype byteLimit = 16 * 1024 * 1024;
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
std::string str(const QString &s) {
    return s.toUtf8().toStdString();
}

QString hash(const QByteArray &b) {
    return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex());
}
struct Node {
    QString name, text, path;
    std::vector<Node> children;
    mutable bool used = false;
};
Node readNode(QXmlStreamReader &r, const QString &path, int depth, int &count, qsizetype &pathCharacters) {
    if (depth > 32 || ++count > 300000)
        throw AppError(ErrorCode::FILE_PROJECTXML_001, {});
    pathCharacters += path.size();
    if (r.name().size() > 128 || path.size() > 4096 || pathCharacters > 16 * 1024 * 1024)
        throw AppError(ErrorCode::FILE_PROJECTXML_002, {});
    if (r.namespaceUri() != QLatin1StringView(ns))
        throw AppError(ErrorCode::FILE_PROJECTXML_003, {diagnosticArgument(r.namespaceUri().toString())});
    if (!r.attributes().isEmpty())
        throw AppError(ErrorCode::FILE_PROJECTXML_004, {diagnosticArgument(path)});
    Node n{r.name().toString(), {}, path, {}, false};
    std::map<QString, int> indices;
    while (!r.atEnd()) {
        auto t = r.readNext();
        if (t == QXmlStreamReader::DTD || t == QXmlStreamReader::EntityReference)
            throw AppError(ErrorCode::FILE_PROJECTXML_005, {});
        if (t == QXmlStreamReader::StartElement) {
            auto name = r.name().toString();
            int i = ++indices[name];
            n.children.push_back(
                readNode(r, path + "/" + name + "[" + QString::number(i) + "]", depth + 1, count, pathCharacters));
        } else if (t == QXmlStreamReader::Characters)
            n.text += r.text();
        else if (t == QXmlStreamReader::EndElement)
            break;
    }
    if (!n.children.empty() && !n.text.trimmed().isEmpty())
        throw AppError(ErrorCode::FILE_PROJECTXML_006, {diagnosticArgument(path)});
    return n;
}
const Node *child(const Node &n, const QString &name) {
    const Node *found = nullptr;
    for (const auto &c : n.children)
        if (c.name == name) {
            if (found)
                throw AppError(ErrorCode::FILE_PROJECTXML_007, {diagnosticArgument(c.path)});
            found = &c;
        }
    if (found)
        found->used = true;
    return found;
}
QString field(const Node &n, const QString &name, const QString &fallback = {}) {
    auto *c = child(n, name);
    if (!c)
        return fallback;
    if (!c->children.empty())
        throw AppError(ErrorCode::FILE_PROJECTXML_008, {diagnosticArgument(c->path)});
    return c->text;
}
qint64 number(const QString &s, const QString &path, qint64 lo, qint64 hi) {
    static const QRegularExpression re("^[+-]?[0-9]+$");
    bool ok = false;
    auto n = s.trimmed().toLongLong(&ok);
    if (!ok || !re.match(s.trimmed()).hasMatch() || n < lo || n > hi)
        throw AppError(ErrorCode::FILE_PROJECTXML_009, {diagnosticArgument(path)});
    return n;
}
qint64 num(const Node &n, const QString &name, qint64 lo, qint64 hi, const QString &fallback = {}) {
    return number(field(n, name, fallback), n.path + "/" + name, lo, hi);
}
QString uid(const Node &n, const QString &name = "UID") {
    return QString::number(num(n, name, 0, 2147483647));
}
bool flag(const Node &n, const QString &name, bool fallback = false) {
    auto s = field(n, name, fallback ? "1" : "0").trimmed();
    if (s == "1" || s == "true")
        return true;
    if (s == "0" || s == "false")
        return false;
    throw AppError(ErrorCode::FILE_PROJECTXML_010, {diagnosticArgument(n.path), diagnosticArgument(name)});
}
std::vector<const Node *> list(const Node &n, const QString &wrapper, const QString &item) {
    std::vector<const Node *> out;
    auto *w = child(n, wrapper);
    if (!w)
        return out;
    for (const auto &c : w->children)
        if (c.name == item) {
            c.used = true;
            out.push_back(&c);
        }
    if (out.size() > 10000)
        throw AppError(ErrorCode::FILE_PROJECTXML_011, {diagnosticArgument(w->path)});
    return out;
}
void diagnostic(QJsonArray &d, const QString &path, const QString &text) {
    d.append(QJsonObject{{"path", path}, {"message", text}});
}
void unconverted(const Node &n, QJsonArray &d) {
    for (const auto &c : n.children) {
        if (!c.used)
            diagnostic(d, c.path, "매핑하지 않음 · 원본 XML에 보존");
        else
            unconverted(c, d);
    }
}
QString stamp(const QString &s, const QString &path) {
    static const QRegularExpression re("^([0-9]{4}-[0-9]{2}-[0-9]{2})T([0-9]{2}:[0-9]{2}):00$");
    auto m = re.match(s.trimmed());
    auto out = m.captured(1) + " " + m.captured(2);
    if (!m.hasMatch() || !validStamp(str(out)))
        throw AppError(ErrorCode::FILE_PROJECTXML_012, {diagnosticArgument(path)});
    return out;
}
qint64 duration(const QString &s, const QString &path) {
    static const QRegularExpression re("^PT(?:(\\d+)H)?(?:(\\d+)M)?(?:(\\d+)S)?$");
    auto m = re.match(s.trimmed());
    if (!m.hasMatch() || (m.captured(1).isEmpty() && m.captured(2).isEmpty() && m.captured(3).isEmpty()))
        throw AppError(ErrorCode::FILE_PROJECTXML_013, {diagnosticArgument(path)});
    auto part = [&](int i) { return m.captured(i).isEmpty() ? qint64(0) : number(m.captured(i), path, 0, 300000000); };
    auto seconds = part(1) * 3600 + part(2) * 60 + part(3);
    if (seconds % 60 || seconds / 60 > 5000000)
        throw AppError(ErrorCode::FILE_PROJECTXML_014, {diagnosticArgument(path)});
    return seconds / 60;
}
bool workingFormat(qint64 n) {
    return n == 3 || n == 5 || n == 7 || n == 9 || n == 11 || n == 35 || n == 37 || n == 39 || n == 41 || n == 43;
}
int units(const QString &s, const QString &path, bool zero) {
    static const QRegularExpression re("^(\\d+)(?:\\.(\\d{1,2})0*)?$");
    auto m = re.match(s.trimmed());
    if (!m.hasMatch())
        throw AppError(ErrorCode::FILE_PROJECTXML_015, {diagnosticArgument(path)});
    auto whole = number(m.captured(1), path, 0, 1000);
    auto fraction = m.captured(2).leftJustified(2, '0').toInt();
    auto value = whole * 100 + fraction;
    if (value > 100000 || (!zero && value == 0))
        throw AppError(ErrorCode::FILE_PROJECTXML_016, {diagnosticArgument(path)});
    return int(value);
}
void graph(const Project &p) {
    std::map<std::string, int> incoming;
    std::map<std::string, std::vector<std::string>> outgoing;
    for (const auto &t : p.tasks)
        incoming[t.id] = 0;
    for (const auto &t : p.tasks)
        for (const auto &pred : t.predecessors) {
            if (pred == t.id || !incoming.contains(pred))
                throw AppError(ErrorCode::FILE_PROJECTXML_017, {diagnosticArgument(qs(t.id))});
            ++incoming[t.id];
            outgoing[pred].push_back(t.id);
        }
    std::vector<std::string> ready;
    for (const auto &[id, n] : incoming)
        if (!n)
            ready.push_back(id);
    std::size_t visited = 0;
    while (!ready.empty()) {
        auto id = ready.back();
        ready.pop_back();
        ++visited;
        for (const auto &next : outgoing[id])
            if (!--incoming[next])
                ready.push_back(next);
    }
    if (visited != p.tasks.size())
        throw AppError(ErrorCode::FILE_PROJECTXML_018, {});
}
int minute(const QString &s, const QString &path) {
    static const QRegularExpression re("^([0-9]{2}):([0-9]{2}):00$");
    auto m = re.match(s.trimmed());
    int h = m.captured(1).toInt(), v = m.captured(2).toInt();
    if (!m.hasMatch() || h > 24 || v > 59 || (h == 24 && v))
        throw AppError(ErrorCode::FILE_PROJECTXML_019, {diagnosticArgument(path)});
    return h * 60 + v;
}
using Week = std::array<std::vector<WorkPeriod>, 7>;
struct CalendarInput {
    const Node *node;
    QString base;
};
struct CalendarValue {
    Week week;
    std::set<std::string> holidays;
};
void holiday(const Node &n, CalendarValue &c) {
    if (!child(n, "DayWorking"))
        throw AppError(ErrorCode::FILE_PROJECTXML_020, {diagnosticArgument(n.path)});
    if (flag(n, "DayWorking"))
        throw AppError(ErrorCode::FILE_PROJECTXML_021, {diagnosticArgument(n.path)});
    if (auto *times = child(n, "WorkingTimes"); times && !times->children.empty())
        throw AppError(ErrorCode::FILE_PROJECTXML_022, {diagnosticArgument(n.path)});
    auto *range = child(n, "TimePeriod");
    if (!range)
        throw AppError(ErrorCode::FILE_PROJECTXML_023, {diagnosticArgument(n.path)});
    auto from = stamp(field(*range, "FromDate"), range->path);
    auto to = stamp(field(*range, "ToDate"), range->path);
    if (from.mid(11) != "00:00" || (to.mid(11) != "00:00" && to.mid(11) != "23:59"))
        throw AppError(ErrorCode::FILE_PROJECTXML_024, {diagnosticArgument(n.path)});
    auto a = QDate::fromString(from.left(10), Qt::ISODate), b = QDate::fromString(to.left(10), Qt::ISODate);
    if (a > b || a.daysTo(b) > 10000)
        throw AppError(ErrorCode::FILE_PROJECTXML_025, {diagnosticArgument(n.path)});
    for (auto day = a; day <= b; day = day.addDays(1)) {
        c.holidays.insert(str(day.toString(Qt::ISODate)));
        if (c.holidays.size() > 10000)
            throw AppError(ErrorCode::FILE_PROJECTXML_026, {});
    }
}
QJsonObject baseReport(const Project &p, bool importing) {
    return {{"contract", "jobs-mspdi-plan-v1"},
            {"direction", importing ? "import" : "export"},
            {"format", "Microsoft Project XML"},
            {"namespace", ns},
            {"documentId", qs(p.documentId)},
            {"revision", QString::number(p.revision)},
            {"nativeSnapshotProfile", qs(p.calculationProfile)},
            {"targetPolicy", QJsonObject{{"calculationProfile", effortProfile},
                                         {"lagCalendar", "successor-working-calendar"},
                                         {"projectStartAnchor", "09:00"}}},
            {"compatibility",
             QJsonObject{{"import", importing ? "mapped-subset" : "not-applicable"},
                         {"originalPreservation", importing ? "exact-xml-bytes" : "exact-native-input-snapshot"},
                         {"jobsRecomputation", "unverified"},
                         {"externalRoundTrip", "unverified"}}},
            {"companionPolicy",
             "원본과 진단은 이 폴더의 동반 파일입니다. .jobs 사본 저장/이동 시 자동 복제되지 않습니다."}};
}
} // namespace

QByteArray readProjectXml(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw AppError(ErrorCode::FILE_PROJECTXML_027, {diagnosticArgument(f.errorString())});
    if (f.size() > byteLimit)
        throw AppError(ErrorCode::FILE_PROJECTXML_028, {});
    auto bytes = f.read(byteLimit + 1);
    if (f.error() != QFileDevice::NoError || bytes.size() > byteLimit || !f.atEnd())
        throw AppError(ErrorCode::FILE_PROJECTXML_029, {});
    return bytes;
}
ProjectXmlExchange importProjectXml(const QByteArray &bytes, const QString &sourceName) {
    if (bytes.size() > byteLimit)
        throw AppError(ErrorCode::FILE_PROJECTXML_030, {});
    QXmlStreamReader reader(bytes);
    reader.setEntityExpansionLimit(0);
    int count = 0;
    qsizetype pathCharacters = 0;
    std::optional<Node> root;
    while (!reader.atEnd()) {
        auto token = reader.readNext();
        if (token == QXmlStreamReader::DTD || token == QXmlStreamReader::EntityReference)
            throw AppError(ErrorCode::FILE_PROJECTXML_031, {});
        if (token == QXmlStreamReader::StartElement) {
            if (root || reader.name() != QLatin1StringView("Project"))
                throw AppError(ErrorCode::FILE_PROJECTXML_032, {});
            root = readNode(reader, "/Project", 1, count, pathCharacters);
        }
    }
    if (reader.hasError() || !root)
        throw AppError(ErrorCode::FILE_PROJECTXML_033, {diagnosticArgument(reader.errorString())});
    auto &r = *root;
    Project p;
    p.documentId = str(QUuid::createUuid().toString(QUuid::WithoutBraces));
    p.calculationProfile = effortProfile;
    p.name = str(field(r, "Name"));
    auto start = stamp(field(r, "StartDate"), "/Project/StartDate");
    p.startDate = str(start.left(10));
    p.currency = str(field(r, "CurrencyCode", "KRW"));
    currencyDecimals(p.currency);
    if (!flag(r, "ScheduleFromStart", true))
        throw AppError(ErrorCode::FILE_PROJECTXML_034, {});
    QJsonArray diagnostics, mapping;
    diagnostic(diagnostics, "/Project",
               "기간 기반 계획 변환입니다. 원본 날짜·공수·실적·비용의 계산 동등성은 미검증입니다.");
    diagnostic(diagnostics, "/Project/Calendars",
               "프로젝트 달력을 수입 작업/자원에 명시 배정합니다. 이후 새 작업은 JOBS 기본 달력을 사용합니다.");
    if (start.mid(11) != "09:00")
        diagnostic(diagnostics, "/Project/StartDate", "시작 날짜는 보존하며 JOBS의 날짜 시작 기준은 09:00입니다.");
    auto saveVersion = field(r, "SaveVersion"), build = field(r, "BuildNumber");
    if (saveVersion.isEmpty())
        throw AppError(ErrorCode::FILE_PROJECTXML_035, {});
    number(saveVersion, "/Project/SaveVersion", 1, 999);
    auto map = [&](const QString &kind, const QString &id, const std::string &target) {
        mapping.append(QJsonObject{{"kind", kind}, {"xmlUid", id}, {"jobsId", qs(target)}});
    };

    std::map<QString, CalendarInput> inputs;
    std::vector<QString> calendarOrder;
    for (auto *c : list(r, "Calendars", "Calendar")) {
        auto id = uid(*c);
        if (inputs.contains(id))
            throw AppError(ErrorCode::FILE_PROJECTXML_036, {diagnosticArgument(id)});
        bool base = flag(*c, "IsBaseCalendar", true);
        auto parent = field(*c, "BaseCalendarUID", "-1").trimmed();
        if (base && parent != "-1")
            throw AppError(ErrorCode::FILE_PROJECTXML_037, {diagnosticArgument(c->path)});
        inputs[id] = {c, base ? QString{} : QString::number(number(parent, c->path, 0, 2147483647))};
        calendarOrder.push_back(id);
    }
    std::map<QString, CalendarValue> resolved;
    std::set<QString> resolving;
    std::size_t expandedHolidays = 0;
    std::function<CalendarValue(const QString &, int)> resolve = [&](const QString &id, int depth) -> CalendarValue {
        if (resolved.contains(id))
            return resolved.at(id);
        if (depth > 32 || resolving.contains(id))
            throw AppError(ErrorCode::FILE_PROJECTXML_038, {diagnosticArgument(id)});
        if (!inputs.contains(id))
            throw AppError(ErrorCode::FILE_PROJECTXML_039, {diagnosticArgument(id)});
        resolving.insert(id);
        const auto &input = inputs.at(id);
        const auto &c = *input.node;
        CalendarValue value;
        if (!input.base.isEmpty())
            value = resolve(input.base, depth + 1);
        std::set<int> seen;
        for (auto *day : list(c, "WeekDays", "WeekDay")) {
            int type = int(num(*day, "DayType", 0, 7));
            if (type == 0) {
                holiday(*day, value);
                continue;
            }
            if (!child(*day, "DayWorking"))
                throw AppError(ErrorCode::FILE_PROJECTXML_040, {diagnosticArgument(day->path)});
            int index = type == 1 ? 6 : type - 2;
            if (!seen.insert(index).second)
                throw AppError(ErrorCode::FILE_PROJECTXML_041, {diagnosticArgument(day->path)});
            auto &periods = value.week[std::size_t(index)];
            periods.clear();
            auto times = list(*day, "WorkingTimes", "WorkingTime");
            if (flag(*day, "DayWorking")) {
                if (times.empty() || times.size() > 5)
                    throw AppError(ErrorCode::FILE_PROJECTXML_042, {diagnosticArgument(day->path)});
                for (auto *time : times)
                    periods.push_back(
                        {minute(field(*time, "FromTime"), time->path), minute(field(*time, "ToTime"), time->path)});
            } else if (!times.empty())
                throw AppError(ErrorCode::FILE_PROJECTXML_043, {diagnosticArgument(day->path)});
        }
        if (input.base.isEmpty() && seen.size() != 7)
            throw AppError(ErrorCode::FILE_PROJECTXML_044, {diagnosticArgument(c.path)});
        for (auto *exception : list(c, "Exceptions", "Exception")) {
            auto type = num(*exception, "Type", 1, 9, "1");
            if ((type != 1 && type != 9) || num(*exception, "Period", 1, 10000, "1") != 1 ||
                flag(*exception, "EnteredByOccurrences"))
                throw AppError(ErrorCode::FILE_PROJECTXML_045, {diagnosticArgument(exception->path)});
            for (const auto &name : {"DaysOfWeek", "MonthItem", "MonthPosition", "Month", "MonthDay"})
                if (child(*exception, name))
                    throw AppError(ErrorCode::FILE_PROJECTXML_046, {diagnosticArgument(exception->path)});
            holiday(*exception, value);
        }
        if (child(c, "WorkWeeks"))
            throw AppError(ErrorCode::FILE_PROJECTXML_047, {diagnosticArgument(c.path)});
        TaskCalendar out;
        out.id = "xml-cal-" + str(id);
        out.name = str(field(c, "Name", "Calendar " + id));
        out.weekdays.clear();
        out.periods.clear();
        for (std::size_t i = 0; i < 7; ++i)
            if (!value.week[i].empty()) {
                out.weekdays.push_back(int(i) + 1);
                if (out.periods.empty())
                    out.periods = value.week[i];
                else if (out.periods != value.week[i])
                    throw AppError(ErrorCode::FILE_PROJECTXML_048, {diagnosticArgument(c.path)});
            }
        out.holidays.assign(value.holidays.begin(), value.holidays.end());
        expandedHolidays += out.holidays.size();
        if (expandedHolidays > 100000)
            throw AppError(ErrorCode::FILE_PROJECTXML_049, {});
        p.calendars.push_back(out);
        map("calendar", id, out.id);
        resolving.erase(id);
        resolved[id] = value;
        return value;
    };
    for (const auto &id : calendarOrder)
        resolve(id, 0);
    auto projectCalendar = QString::number(num(r, "CalendarUID", 0, 2147483647));
    if (!inputs.contains(projectCalendar))
        throw AppError(ErrorCode::FILE_PROJECTXML_050, {diagnosticArgument(projectCalendar)});
    auto calendar = [&](const Node &n) {
        auto id = field(n, "CalendarUID", "-1").trimmed();
        if (id == "-1")
            id = projectCalendar;
        else
            id = QString::number(number(id, n.path, 0, 2147483647));
        if (!inputs.contains(id))
            throw AppError(ErrorCode::FILE_PROJECTXML_051, {diagnosticArgument(n.path)});
        return "xml-cal-" + str(id);
    };

    std::set<QString> taskUids;
    std::map<QString, std::string> taskIds;
    std::vector<std::pair<const Node *, std::size_t>> taskNodes;
    std::vector<std::pair<int, std::string>> stack;
    bool previousLeaf = false;
    int previousLevel = 0;
    for (auto *n : list(r, "Tasks", "Task")) {
        auto id = uid(*n);
        if (!taskUids.insert(id).second)
            throw AppError(ErrorCode::FILE_PROJECTXML_052, {diagnosticArgument(id)});
        if (flag(*n, "IsSubproject") || child(*n, "Project"))
            throw AppError(ErrorCode::FILE_PROJECTXML_053, {diagnosticArgument(n->path)});
        if (id == "0") {
            diagnostic(diagnostics, n->path, "프로젝트 요약 · 실행 작업으로 변환하지 않음");
            continue;
        }
        if (flag(*n, "IsNull"))
            throw AppError(ErrorCode::FILE_PROJECTXML_054, {diagnosticArgument(n->path)});
        if (flag(*n, "Manual") || flag(*n, "ExternalTask") || !field(*n, "SubprojectName").isEmpty())
            throw AppError(ErrorCode::FILE_PROJECTXML_055, {diagnosticArgument(n->path)});
        auto level = int(num(*n, "OutlineLevel", 1, 32, "1"));
        if (level > previousLevel + 1 || (previousLeaf && level > previousLevel))
            throw AppError(ErrorCode::FILE_PROJECTXML_056, {diagnosticArgument(n->path)});
        while (!stack.empty() && stack.back().first >= level)
            stack.pop_back();
        auto parent = stack.empty() ? std::string{} : stack.back().second;
        if (level > 1 && (stack.empty() || stack.back().first != level - 1))
            throw AppError(ErrorCode::FILE_PROJECTXML_057, {diagnosticArgument(n->path)});
        auto name = str(field(*n, "Name"));
        bool summary = flag(*n, "Summary");
        if (summary) {
            if (child(*n, "PredecessorLink"))
                throw AppError(ErrorCode::FILE_PROJECTXML_058, {diagnosticArgument(n->path)});
            WbsNode group{"xml-wbs-" + str(id), name, parent};
            p.wbsNodes.push_back(group);
            stack.emplace_back(level, group.id);
            map("wbs", id, group.id);
        } else {
            Task t;
            t.id = "xml-task-" + str(id);
            t.name = name;
            t.wbsId = parent;
            t.calendarId = calendar(*n);
            auto d = field(*n, "Duration");
            if (!d.isEmpty())
                t.durationMinutes = duration(d, n->path + "/Duration");
            const auto format = num(*n, "DurationFormat", 0, 100, "7");
            if (!workingFormat(format))
                throw AppError(ErrorCode::FILE_PROJECTXML_059, {diagnosticArgument(n->path)});
            if (format >= 35)
                diagnostic(diagnostics, n->path + "/DurationFormat",
                           "근무분 값만 매핑 · 추정 기간 표시를 변환하지 않음");
            if (flag(*n, "Milestone") && t.durationMinutes != 0)
                throw AppError(ErrorCode::FILE_PROJECTXML_060, {diagnosticArgument(n->path)});
            if (num(*n, "Type", 0, 2, "0") != 1)
                diagnostic(diagnostics, n->path + "/Type",
                           "기간 기반 고정 기간으로 투영 · 공수/작업 유형 계산 규칙은 보존 원본 참조");
            static const std::array<ConstraintType, 8> constraints{
                ConstraintType::ASAP, ConstraintType::ALAP, ConstraintType::MSO,  ConstraintType::MFO,
                ConstraintType::SNET, ConstraintType::SNLT, ConstraintType::FNET, ConstraintType::FNLT};
            auto type = num(*n, "ConstraintType", 0, 7, "0");
            t.constraintType = constraints[std::size_t(type)];
            if (type > 1)
                t.constraintAt = str(stamp(field(*n, "ConstraintDate"), n->path + "/ConstraintDate"));
            auto deadline = field(*n, "Deadline");
            if (!deadline.isEmpty())
                t.deadlineAt = str(stamp(deadline, n->path + "/Deadline"));
            if (flag(*n, "IgnoreResourceCalendar"))
                throw AppError(ErrorCode::FILE_PROJECTXML_061, {diagnosticArgument(n->path)});
            taskIds[id] = t.id;
            map("task", id, t.id);
            taskNodes.emplace_back(n, p.tasks.size());
            p.tasks.push_back(std::move(t));
        }
        previousLeaf = !summary;
        previousLevel = level;
    }
    for (const auto &[n, index] : taskNodes) {
        auto &task = p.tasks[index];
        for (const auto &link : n->children)
            if (link.name == "PredecessorLink") {
                link.used = true;
                auto pred = uid(link, "PredecessorUID");
                if (!taskIds.contains(pred) || flag(link, "CrossProject") || !field(link, "CrossProjectName").isEmpty())
                    throw AppError(ErrorCode::FILE_PROJECTXML_062, {diagnosticArgument(link.path)});
                static const std::array<DependencyType, 4> types{DependencyType::FF, DependencyType::FS,
                                                                 DependencyType::SF, DependencyType::SS};
                auto type = types[std::size_t(num(link, "Type", 0, 3, "1"))];
                if (child(link, "LinkLag") && !child(link, "LagFormat"))
                    throw AppError(ErrorCode::FILE_PROJECTXML_063, {diagnosticArgument(link.path)});
                auto lag = num(link, "LinkLag", -50000000, 50000000, "0");
                if (lag % 10)
                    throw AppError(ErrorCode::FILE_PROJECTXML_064, {diagnosticArgument(link.path)});
                const auto format = num(link, "LagFormat", 0, 100, "7");
                if (!workingFormat(format))
                    throw AppError(ErrorCode::FILE_PROJECTXML_065, {diagnosticArgument(link.path)});
                if (format >= 35)
                    diagnostic(diagnostics, link.path + "/LagFormat",
                               "근무분 값만 매핑 · 추정 지연 표시를 변환하지 않음");
                task.predecessors.push_back(taskIds.at(pred));
                if (type != DependencyType::FS || lag)
                    task.dependencyRules.push_back({taskIds.at(pred), type, lag / 10});
            }
    }
    std::map<QString, std::string> resources;
    std::set<QString> resourceUids;
    for (auto *n : list(r, "Resources", "Resource")) {
        auto id = uid(*n);
        if (!resourceUids.insert(id).second)
            throw AppError(ErrorCode::FILE_PROJECTXML_066, {diagnosticArgument(id)});
        auto name = field(*n, "Name");
        if (id == "0" && name.trimmed().isEmpty()) {
            diagnostic(diagnostics, n->path, "미배정 placeholder · 자원으로 변환하지 않음");
            continue;
        }
        if (num(*n, "Type", 0, 2, "1") != 1)
            throw AppError(ErrorCode::FILE_PROJECTXML_067, {diagnosticArgument(n->path)});
        Resource resource;
        resource.id = "xml-resource-" + str(id);
        resource.name = str(name);
        resource.calendarId = calendar(*n);
        auto capacity = field(*n, "MaxUnits");
        resource.maxUnits = capacity.isEmpty() ? std::nullopt : std::optional<int>{units(capacity, n->path, true)};
        resources[id] = resource.id;
        map("resource", id, resource.id);
        p.resources.push_back(std::move(resource));
    }
    std::set<QString> assignments;
    for (auto *n : list(r, "Assignments", "Assignment")) {
        auto id = uid(*n);
        if (!assignments.insert(id).second)
            throw AppError(ErrorCode::FILE_PROJECTXML_068, {diagnosticArgument(id)});
        auto t = uid(*n, "TaskUID"), resource = uid(*n, "ResourceUID");
        if (!taskIds.contains(t) || !resources.contains(resource))
            throw AppError(ErrorCode::FILE_PROJECTXML_069, {diagnosticArgument(n->path)});
        p.assignments.push_back({taskIds.at(t), resources.at(resource), units(field(*n, "Units"), n->path, false)});
        map("assignment", id, taskIds.at(t) + "/" + resources.at(resource));
    }
    unconverted(r, diagnostics);
    validate(p);
    graph(p);
    auto report = baseReport(p, true);
    report["source"] = QJsonObject{{"name", QFileInfo(sourceName).fileName()},
                                   {"sha256", hash(bytes)},
                                   {"producer", "unverified"},
                                   {"saveVersion", saveVersion},
                                   {"buildNumber", build}};
    report["mapping"] = mapping;
    report["diagnostics"] = diagnostics;
    return {std::move(p), bytes, report, true};
}

namespace {
QString xmlStamp(const std::string &s) {
    auto v = qs(s);
    v.replace(' ', 'T');
    return v + ":00";
}
QString timeText(int m) {
    return QString("%1:%2:00").arg(m / 60, 2, 10, QChar('0')).arg(m % 60, 2, 10, QChar('0'));
}
QString unitText(int u) {
    return QString::number(u / 100) + "." + QString::number(u % 100).rightJustified(2, '0');
}
QString durationText(std::int64_t n) {
    return "PT" + QString::number(n / 60) + "H" + QString::number(n % 60) + "M0S";
}
} // namespace
ProjectXmlExchange exportProjectXml(const Project &p) {
    validate(p);
    graph(p);
    if (qs(p.name).size() > 255)
        throw AppError(ErrorCode::FILE_PROJECTXML_070, {});
    for (const auto &t : p.tasks)
        if (qs(t.name).size() > 512)
            throw AppError(ErrorCode::FILE_PROJECTXML_071, {diagnosticArgument(qs(t.id))});
    for (const auto &g : p.wbsNodes)
        if (qs(g.name).size() > 512)
            throw AppError(ErrorCode::FILE_PROJECTXML_072, {diagnosticArgument(qs(g.id))});
    for (const auto &r : p.resources)
        if (qs(r.name).size() > 512)
            throw AppError(ErrorCode::FILE_PROJECTXML_073, {diagnosticArgument(qs(r.id))});
    for (const auto &c : p.calendars)
        if (qs(c.name).size() > 512)
            throw AppError(ErrorCode::FILE_PROJECTXML_074, {diagnosticArgument(qs(c.id))});
    if (p.tasks.size() + p.wbsNodes.size() > 10000 || p.resources.size() > 10000 || p.assignments.size() > 10000 ||
        p.calendars.size() >= 10000)
        throw AppError(ErrorCode::FILE_PROJECTXML_075, {});
    QByteArray bytes;
    QXmlStreamWriter w(&bytes);
    w.setAutoFormatting(true);
    w.writeStartDocument();
    w.writeStartElement("Project");
    w.writeDefaultNamespace(ns);
    auto value = [&](const QString &name, const QString &text) {
        w.writeTextElement(name, text);
        if (bytes.size() > byteLimit)
            throw AppError(ErrorCode::FILE_PROJECTXML_076, {});
    };
    value("SaveVersion", "12");
    value("Name", qs(p.name));
    value("ScheduleFromStart", "1");
    value("StartDate", qs(p.startDate) + "T09:00:00");
    value("CurrencyDigits", QString::number(currencyDecimals(p.currency)));
    value("CurrencyCode", qs(p.currency));
    value("CalendarUID", "1");
    value("DefaultStartTime", "09:00:00");
    value("DefaultFinishTime", "18:00:00");
    value("MinutesPerDay", "480");
    value("MinutesPerWeek", "2400");
    QJsonArray diagnostics, mapping;
    diagnostic(diagnostics, "/Project",
               "기간 기반 계획 투영입니다. 외부 제품의 계산/재저장 호환은 미검증입니다. 원본 전체 입력/이력은 "
               "source.jobs 참조.");
    auto map = [&](const QString &kind, int id, const std::string &target) {
        mapping.append(QJsonObject{{"kind", kind}, {"xmlUid", QString::number(id)}, {"jobsId", qs(target)}});
    };
    std::map<std::string, int> calendars;
    calendars[""] = 1;
    w.writeStartElement("Calendars");
    auto calendar = [&](const TaskCalendar &c, int id) {
        if (c.periods.size() > 5 || c.holidays.size() > 10000)
            throw AppError(ErrorCode::FILE_PROJECTXML_077, {});
        w.writeStartElement("Calendar");
        value("UID", QString::number(id));
        value("Name", qs(c.name));
        value("IsBaseCalendar", "1");
        value("BaseCalendarUID", "-1");
        w.writeStartElement("WeekDays");
        for (int day = 1; day <= 7; ++day) {
            int nativeDay = day == 1 ? 7 : day - 1;
            bool working = std::find(c.weekdays.begin(), c.weekdays.end(), nativeDay) != c.weekdays.end();
            w.writeStartElement("WeekDay");
            value("DayType", QString::number(day));
            value("DayWorking", working ? "1" : "0");
            if (working) {
                w.writeStartElement("WorkingTimes");
                for (const auto &period : c.periods) {
                    w.writeStartElement("WorkingTime");
                    value("FromTime", timeText(period.begin));
                    value("ToTime", timeText(period.end));
                    w.writeEndElement();
                }
                w.writeEndElement();
            }
            w.writeEndElement();
        }
        for (const auto &date : c.holidays) {
            w.writeStartElement("WeekDay");
            value("DayType", "0");
            value("DayWorking", "0");
            w.writeStartElement("TimePeriod");
            value("FromDate", qs(date) + "T00:00:00");
            value("ToDate", qs(date) + "T23:59:00");
            w.writeEndElement();
            w.writeEndElement();
        }
        w.writeEndElement();
        w.writeEndElement();
    };
    TaskCalendar standard;
    standard.name = "JOBS standard";
    standard.holidays = p.holidays;
    calendar(standard, 1);
    map("calendar", 1, "");
    int next = 2;
    for (const auto &c : p.calendars) {
        calendars[c.id] = next;
        calendar(c, next);
        map("calendar", next++, c.id);
    }
    w.writeEndElement();

    struct Row {
        const WbsNode *group = nullptr;
        const Task *task = nullptr;
        int level = 1;
    };
    std::map<std::string, std::vector<const WbsNode *>> groups;
    std::map<std::string, std::vector<const Task *>> tasks;
    for (const auto &g : p.wbsNodes)
        groups[g.parentId].push_back(&g);
    for (const auto &t : p.tasks)
        tasks[t.wbsId].push_back(&t);
    std::vector<Row> rows;
    std::function<void(const std::string &, int)> visit = [&](const std::string &parent, int level) {
        if (level > 32)
            throw AppError(ErrorCode::FILE_PROJECTXML_078, {});
        for (auto *g : groups[parent]) {
            rows.push_back({g, nullptr, level});
            visit(g->id, level + 1);
        }
        for (auto *t : tasks[parent])
            rows.push_back({nullptr, t, level});
    };
    visit("", 1);
    std::map<std::string, int> taskIds;
    next = 1;
    for (const auto &row : rows) {
        if (row.task)
            taskIds[row.task->id] = next;
        map(row.task ? "task" : "wbs", next, row.task ? row.task->id : row.group->id);
        ++next;
    }
    w.writeStartElement("Tasks");
    next = 1;
    for (const auto &row : rows) {
        w.writeStartElement("Task");
        value("UID", QString::number(next));
        value("ID", QString::number(next++));
        value("Name", qs(row.task ? row.task->name : row.group->name));
        value("Type", "1");
        value("OutlineLevel", QString::number(row.level));
        if (row.task && row.task->durationMinutes)
            value("Duration", durationText(*row.task->durationMinutes));
        value("DurationFormat", "3");
        value("Milestone", row.task && row.task->durationMinutes == 0 ? "1" : "0");
        value("Summary", row.group ? "1" : "0");
        if (row.task) {
            const auto &t = *row.task;
            static const std::array<ConstraintType, 8> constraints{
                ConstraintType::ASAP, ConstraintType::ALAP, ConstraintType::MSO,  ConstraintType::MFO,
                ConstraintType::SNET, ConstraintType::SNLT, ConstraintType::FNET, ConstraintType::FNLT};
            auto it = std::find(constraints.begin(), constraints.end(), t.constraintType);
            value("ConstraintType", QString::number(it - constraints.begin()));
            value("CalendarUID", QString::number(calendars.at(t.calendarId)));
            if (t.constraintAt)
                value("ConstraintDate", xmlStamp(*t.constraintAt));
            if (t.deadlineAt)
                value("Deadline", xmlStamp(*t.deadlineAt));
            for (const auto &d : dependencies(t)) {
                w.writeStartElement("PredecessorLink");
                value("PredecessorUID", QString::number(taskIds.at(d.predecessorId)));
                int type = d.type == DependencyType::FF   ? 0
                           : d.type == DependencyType::FS ? 1
                           : d.type == DependencyType::SF ? 2
                                                          : 3;
                value("Type", QString::number(type));
                value("CrossProject", "0");
                value("LinkLag", QString::number(d.lagMinutes * 10));
                value("LagFormat", "3");
                w.writeEndElement();
            }
            if (t.taskType != TaskType::FixedDuration || t.workUnitMinutes || !t.breaks.empty())
                diagnostic(diagnostics, "/Task/" + qs(t.id),
                           "공수·작업 유형·분할을 내보내지 않음 · 명목 기간 기반 투영");
            if (!t.durationMinutes)
                diagnostic(diagnostics, "/Task/" + qs(t.id) + "/Duration",
                           "미정 기간 생략 · 외부 앱에서 기본 기간으로 해석할 수 있음");
        }
        w.writeEndElement();
    }
    w.writeEndElement();
    std::map<std::string, int> resources;
    next = 1;
    w.writeStartElement("Resources");
    for (const auto &r : p.resources) {
        resources[r.id] = next;
        map("resource", next, r.id);
        w.writeStartElement("Resource");
        value("UID", QString::number(next++));
        value("Name", qs(r.name));
        value("Type", "1");
        if (r.maxUnits)
            value("MaxUnits", unitText(*r.maxUnits));
        else
            diagnostic(diagnostics, "/Resource/" + qs(r.id) + "/MaxUnits",
                       "미정 가용량 생략 · 외부 앱에서 기본 가용량으로 해석할 수 있음");
        value("CalendarUID", QString::number(calendars.at(r.calendarId)));
        w.writeEndElement();
        if (r.type != "labor")
            diagnostic(diagnostics, "/Resource/" + qs(r.id), "장비를 work 자원으로 투영");
        if (!r.rates.empty() || !r.exceptions.empty())
            diagnostic(diagnostics, "/Resource/" + qs(r.id), "단가/가용량 예외를 내보내지 않음");
    }
    w.writeEndElement();
    next = 1;
    w.writeStartElement("Assignments");
    for (const auto &a : p.assignments) {
        w.writeStartElement("Assignment");
        value("UID", QString::number(next));
        map("assignment", next++, a.taskId + "/" + a.resourceId);
        value("TaskUID", QString::number(taskIds.at(a.taskId)));
        value("ResourceUID", QString::number(resources.at(a.resourceId)));
        value("Units", unitText(a.units));
        w.writeEndElement();
    }
    w.writeEndElement();
    w.writeEndElement();
    w.writeEndDocument();
    if (w.hasError() || bytes.size() > byteLimit)
        throw AppError(ErrorCode::FILE_PROJECTXML_079, {});
    // Parsing our own output is a support-boundary check, never a Microsoft Project compatibility claim.
    importProjectXml(bytes, "project.xml");
    if (!p.baselines.empty() || !p.progress.empty() || !p.costEntries.empty() || !p.scenarios.empty() || p.statusAt ||
        !p.taskCosts.empty())
        diagnostic(diagnostics, "/Project/history-costs",
                   "기준선·실적·원가·상태일·시나리오는 XML에 포함하지 않음 · source.jobs에 보존");
    auto report = baseReport(p, false);
    report["source"] = QJsonObject{{"name", "source.jobs"}, {"product", "JOBS"}};
    report["xmlSha256"] = hash(bytes);
    report["mapping"] = mapping;
    report["diagnostics"] = diagnostics;
    return {p, bytes, report, false};
}

QString publishProjectXml(const ProjectXmlExchange &e, const QString &destination) {
    return publishPlanExchange(e.project, e.xml, e.report, e.importing, "xml", destination);
}
QString projectXmlPreview(const ProjectXmlExchange &e) {
    auto text = QString("%1\n작업 %2 · WBS %3 · 달력 %4 · 자원 %5 · 배정 %6\n\n가져오기: %7\n원본 보존: %8\nJOBS "
                        "재계산 동등성: 미검증\nMicrosoft Project 왕복: 미검증\n\n원본/진단은 교환 폴더에 보관됩니다. "
                        ".jobs만 사본 저장/이동하면 동반 파일은 자동 복제되지 않습니다.\n\n")
                    .arg(qs(e.project.name))
                    .arg(e.project.tasks.size())
                    .arg(e.project.wbsNodes.size())
                    .arg(e.project.calendars.size())
                    .arg(e.project.resources.size())
                    .arg(e.project.assignments.size())
                    .arg(e.importing ? "지원 부분 변환" : "해당 없음");
    text = text.arg(e.importing ? "XML 원본 바이트의 동반 파일" : "현재 프로젝트 전체 입력/이력의 동반 사본");
    const auto diagnostics = e.report["diagnostics"].toArray();
    int shown = 0;
    for (const auto &d : diagnostics) {
        if (shown++ == 2000) {
            text += "이후 진단은 conversion.json 참조\n";
            break;
        }
        auto o = d.toObject();
        text += o["path"].toString() + ": " + o["message"].toString() + "\n";
    }
    return text;
}
} // namespace jobs
