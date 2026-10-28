#include "formatting/QtDiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "persistence/PrimaveraXer.h"
#include "persistence/ExchangePackage.h"
#include <QCryptographicHash>
#include <QDate>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QRegularExpression>
#include <QStringConverter>
#include <QUuid>
#include <algorithm>
#include <functional>
#include <map>
#include <set>
#include <stdexcept>

namespace jobs {
namespace {
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
void diagnostic(QJsonArray &out, const QString &path, const QString &message) {
    out.append(QJsonObject{{"path", path}, {"message", message}});
}
QString encodingName(XerEncoding e) {
    return e == XerEncoding::Utf8 ? "UTF-8" : "Windows-1252";
}
bool forbiddenTextCharacter(QChar c) {
    const auto category = c.category();
    return category == QChar::Other_Control || category == QChar::Separator_Line ||
           category == QChar::Separator_Paragraph;
}
QString decode(const QByteArray &bytes, XerEncoding encoding) {
    if (bytes.size() > byteLimit)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_001, {});
    if (encoding == XerEncoding::Utf8) {
        QStringDecoder decoder(QStringDecoder::Utf8);
        QString text = decoder.decode(bytes);
        if (decoder.hasError())
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_002, {});
        if (text.startsWith(QChar(0xfeff)))
            text.remove(0, 1);
        return text;
    }
    if (bytes.startsWith("\xef\xbb\xbf"))
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_003, {});
    static constexpr char16_t cp[32] = {0x20ac, 0,      0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021,
                                        0x02c6, 0x2030, 0x0160, 0x2039, 0x0152, 0,      0x017d, 0,
                                        0,      0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
                                        0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0,      0x017e, 0x0178};
    QString out;
    out.reserve(bytes.size());
    for (unsigned char c : bytes) {
        if (c >= 0x80 && c <= 0x9f) {
            auto v = cp[c - 0x80];
            if (!v)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_004, {});
            out += QChar(v);
        } else
            out += QChar(c);
    }
    return out;
}
qint64 integer(const QString &s, const QString &path, qint64 lo = 0, qint64 hi = 2147483647) {
    static const QRegularExpression re("^-?[0-9]+$");
    bool ok = false;
    auto n = s.toLongLong(&ok);
    if (!re.match(s).hasMatch() || !ok || n < lo || n > hi)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_005, {diagnosticArgument(path)});
    return n;
}
qint64 micro(const QString &s, const QString &path, bool negative = false) {
    static const QRegularExpression re("^(-?)([0-9]+)(?:\\.([0-9]{1,6}))?$");
    auto m = re.match(s);
    if (!m.hasMatch() || (!negative && !m.captured(1).isEmpty()))
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_006, {diagnosticArgument(path)});
    auto n = integer(m.captured(2), path, 0, 1000000000) * 1000000 + m.captured(3).leftJustified(6, '0').toLongLong();
    return m.captured(1).isEmpty() ? n : -n;
}
qint64 minutes(const QString &s, const QString &path, QJsonArray &diagnostics, bool negative = false) {
    auto scaled = micro(s, path, negative) * 60;
    auto rounded = (scaled + (scaled < 0 ? -500000 : 500000)) / 1000000;
    if (std::abs(scaled - rounded * 1000000) > 30 || std::abs(rounded) > 5000000)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_007, {diagnosticArgument(path)});
    if (scaled != rounded * 1000000)
        diagnostic(diagnostics, path, "6자리 소수 시간 표현의 양자화 오차를 정수 분으로 보정");
    return rounded;
}
int units(const QString &s, const QString &path, bool allowZero) {
    auto m = micro(s, path);
    if (m % 10000 || m > 1000000000 || (!allowZero && !m))
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_008, {diagnosticArgument(path)});
    return int(m / 10000);
}
QString stamp(const QString &s, const QString &path) {
    if (!validStamp(str(s)))
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_009, {diagnosticArgument(path)});
    return s;
}
struct Row {
    QStringList cells;
    int line = 0;
};
struct Table {
    QString name;
    QStringList fields;
    std::vector<Row> rows;
    mutable std::set<QString> used;
};
struct File {
    QStringList header;
    std::map<QString, Table> tables;
};
QString path(const Table &t, const Row &r, const QString &field) {
    return t.name + "/line-" + QString::number(r.line) + "/" + field;
}
QString get(const Table &t, const Row &r, const QString &field) {
    t.used.insert(field);
    auto i = t.fields.indexOf(field);
    return i < 0 ? QString{} : r.cells.at(i);
}
QString id(const Table &t, const Row &r, const QString &field) {
    return QString::number(integer(get(t, r, field), path(t, r, field)));
}
File parse(const QString &text) {
    File file;
    Table *current = nullptr;
    bool ended = false, header = false;
    qsizetype totalCells = 0;
    if (text.count('\n') > 300001)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_010, {});
    auto lines = text.split('\n', Qt::KeepEmptyParts);
    static const QRegularExpression tableName("^[A-Z][A-Z0-9_]{0,127}$"), fieldName("^[a-z][a-z0-9_]{0,127}$");
    int lineNumber = 0;
    for (auto line : lines) {
        ++lineNumber;
        if (line.endsWith('\r'))
            line.chop(1);
        if (line.isEmpty())
            continue;
        if (ended)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_011, {});
        for (auto c : line)
            if (forbiddenTextCharacter(c) && c != '\t')
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_012, {diagnosticArgument(QString::number(lineNumber))});
        if (line.count('\t') > 256)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_013, {});
        const auto cells = line.split('\t', Qt::KeepEmptyParts);
        totalCells += cells.size();
        if (totalCells > 1000000)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_014, {});
        for (const auto &c : cells)
            if (c.size() > 1024 * 1024)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_015, {});
        if (!header) {
            if (cells.size() != 9 || cells[0] != "ERMHDR" ||
                !QRegularExpression("^[0-9]+(?:\\.[0-9]+){1,2}$").match(cells[1]).hasMatch())
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_016, {});
            file.header = cells;
            header = true;
            continue;
        }
        if (cells[0] == "%T") {
            if (cells.size() != 2 || !tableName.match(cells[1]).hasMatch() || file.tables.contains(cells[1]) ||
                (current && current->fields.isEmpty()) || file.tables.size() >= 128)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_017, {});
            auto [it, inserted] = file.tables.emplace(cells[1], Table{cells[1], {}, {}, {}});
            (void)inserted;
            current = &it->second;
        } else if (cells[0] == "%F") {
            if (!current || !current->fields.isEmpty() || cells.size() < 2)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_018, {});
            std::set<QString> names;
            for (qsizetype i = 1; i < cells.size(); ++i)
                if (!fieldName.match(cells[i]).hasMatch() || !names.insert(cells[i]).second)
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_019, {});
            current->fields = cells.mid(1);
        } else if (cells[0] == "%R") {
            if (!current || current->fields.isEmpty() || cells.size() != current->fields.size() + 1)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_020, {diagnosticArgument(QString::number(lineNumber))});
            current->rows.push_back({cells.mid(1), lineNumber});
        } else if (cells[0] == "%E") {
            if (cells.size() != 1 || (current && current->fields.isEmpty()))
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_021, {});
            ended = true;
        } else
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_022, {diagnosticArgument(QString::number(lineNumber))});
    }
    if (!header || !ended)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_023, {});
    return file;
}
const Table &table(File &file, const char *name) {
    auto [it, inserted] = file.tables.try_emplace(name, Table{name, {}, {}, {}});
    (void)inserted;
    if (it->second.rows.size() > 10000)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_024, {diagnosticArgument(QString::fromLatin1(name))});
    return it->second;
}
void unique(const Table &t, const QString &field) {
    std::set<QString> seen;
    for (const auto &r : t.rows)
        if (!seen.insert(id(t, r, field)).second)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_025, {diagnosticArgument(path(t, r, field))});
}
void projectRef(const Table &t, const Row &r, const QString &project, const QString &field = "proj_id") {
    if (id(t, r, field) != project)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_026, {diagnosticArgument(path(t, r, field))});
}
struct Record {
    QString name;
    std::map<QString, QString> attrs;
    std::vector<Record> children;
};
class CalendarParser {
public:
    explicit CalendarParser(const QString &s, int &count) : s_(s), count_(count) {}
    Record parse() {
        auto r = node(1);
        space();
        if (pos_ != s_.size())
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_027, {});
        return r;
    }

private:
    const QString &s_;
    qsizetype pos_ = 0;
    int &count_;
    void space() {
        while (pos_ < s_.size() && s_[pos_].isSpace())
            ++pos_;
    }
    void expect(QChar c) {
        space();
        if (pos_ >= s_.size() || s_[pos_++] != c)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_028, {});
    }
    QString until(QChar c) {
        auto start = pos_;
        while (pos_ < s_.size() && s_[pos_] != c)
            ++pos_;
        if (pos_ == s_.size())
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_029, {});
        return s_.mid(start, pos_ - start).trimmed();
    }
    Record node(int depth) {
        if (depth > 32 || ++count_ > 300000)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_030, {});
        expect('(');
        auto head = until('(');
        expect('(');
        auto divider = head.indexOf("||");
        if (divider < 1)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_031, {});
        integer(head.left(divider), "clndr_data/record");
        Record out;
        out.name = head.mid(divider + 2).trimmed();
        if (out.name.size() > 128)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_032, {});
        auto attrs = until(')');
        expect(')');
        if (!attrs.isEmpty()) {
            auto parts = attrs.split('|', Qt::KeepEmptyParts);
            if (parts.size() % 2)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_033, {});
            for (qsizetype i = 0; i < parts.size(); i += 2)
                if (!out.attrs.emplace(parts[i], parts[i + 1]).second)
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_034, {});
        }
        expect('(');
        space();
        while (pos_ < s_.size() && s_[pos_] == '(') {
            out.children.push_back(node(depth + 1));
            space();
        }
        expect(')');
        expect(')');
        return out;
    }
};
int clockMinute(const QString &s) {
    if (s == "24:00")
        return 1440;
    if (!QRegularExpression("^[0-2][0-9]:[0-5][0-9]$").match(s).hasMatch() || s.left(2).toInt() > 23)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_035, {});
    return s.left(2).toInt() * 60 + s.right(2).toInt();
}
const Record *find(const Record &r, const QString &name) {
    const Record *result = nullptr;
    for (const auto &c : r.children)
        if (c.name == name) {
            if (result)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_036, {diagnosticArgument(name)});
            result = &c;
        }
    return result;
}
void graph(const Project &p) {
    std::map<std::string, int> incoming;
    std::map<std::string, std::vector<std::string>> outgoing;
    for (const auto &t : p.tasks)
        incoming[t.id] = 0;
    for (const auto &t : p.tasks)
        for (const auto &pred : t.predecessors) {
            if (t.id == pred || !incoming.contains(pred))
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_037, {});
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
        for (const auto &to : outgoing[id])
            if (!--incoming[to])
                ready.push_back(to);
    }
    if (visited != p.tasks.size())
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_038, {});
}
QJsonObject reportFor(const Project &p, bool importing) {
    return {{"contract", "jobs-xer-plan-v1"},
            {"direction", importing ? "import" : "export"},
            {"documentId", qs(p.documentId)},
            {"revision", QString::number(p.revision)},
            {"nativeSnapshotProfile", qs(p.calculationProfile)},
            {"targetPolicy", QJsonObject{{"calculationProfile", effortProfile},
                                         {"projectStartAnchor", "09:00"},
                                         {"lagCalendar", "successor-working-calendar"}}},
            {"compatibility",
             QJsonObject{{"import", importing ? "mapped-subset" : "not-applicable"},
                         {"originalPreservation", importing ? "exact-xer-bytes" : "exact-native-input-snapshot"},
                         {"jobsRecomputation", "unverified"},
                         {"externalRoundTrip", "unverified"}}}};
}
const std::map<QString, ConstraintType> constraintMap{{"", ConstraintType::ASAP},
                                                      {"CS_ALAP", ConstraintType::ALAP},
                                                      {"CS_MSOA", ConstraintType::SNET},
                                                      {"CS_MSOB", ConstraintType::SNLT},
                                                      {"CS_MEOA", ConstraintType::FNET},
                                                      {"CS_MEOB", ConstraintType::FNLT},
                                                      {"CS_MANDSTART", ConstraintType::MSO},
                                                      {"CS_MANDFIN", ConstraintType::MFO},
                                                      {"CS_MSO", ConstraintType::MSO},
                                                      {"CS_MEO", ConstraintType::MFO}};
} // namespace

QByteArray readPrimaveraXer(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_039, {diagnosticArgument(f.errorString())});
    if (f.size() > byteLimit)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_040, {});
    auto b = f.read(byteLimit + 1);
    if (f.error() != QFileDevice::NoError || b.size() > byteLimit || !f.atEnd())
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_041, {});
    return b;
}
PrimaveraXerExchange importPrimaveraXer(const QByteArray &bytes, const QString &sourceName, XerEncoding encoding) {
    auto file = parse(decode(bytes, encoding));
    auto &projects = table(file, "PROJECT");
    if (projects.rows.size() != 1)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_042, {});
    const auto &pr = projects.rows.front();
    const auto projectId = id(projects, pr, "proj_id");
    auto code = get(projects, pr, "proj_short_name");
    Project p;
    p.documentId = str(QUuid::createUuid().toString(QUuid::WithoutBraces));
    p.calculationProfile = effortProfile;
    p.name = str(code);
    p.currency = str(file.header[8]);
    currencyDecimals(p.currency);
    auto start = stamp(get(projects, pr, "plan_start_date"), path(projects, pr, "plan_start_date"));
    p.startDate = str(start.left(10));
    QJsonArray diagnostics, mapping;
    diagnostic(diagnostics, "PROJECT",
               "기간 기반 고정 기간 투영 · 공수/자원 종속 일정·실적/원가/P6 재계산 호환은 미검증입니다.");
    diagnostic(diagnostics, "CALENDAR",
               "수입 작업/자원에 달력을 명시 배정합니다. 새 native 작업은 JOBS 기본 달력을 사용합니다. 기간 기반 "
               "일정은 작업 달력을 사용합니다.");
    if (start.mid(11) != "09:00")
        diagnostic(diagnostics, "PROJECT/plan_start_date", "시작 날짜는 유지 · native 시작 기준은 09:00");
    auto map = [&](const QString &kind, const QString &source, const std::string &target,
                   const QString &activityCode = QString{}) {
        mapping.append(
            QJsonObject{{"kind", kind}, {"xerId", source}, {"jobsId", qs(target)}, {"activityCode", activityCode}});
    };
    const auto &options = table(file, "SCHEDOPTIONS");
    if (options.rows.size() > 1)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_043, {});
    QString lagPolicy;
    if (!options.rows.empty()) {
        projectRef(options, options.rows[0], projectId);
        lagPolicy = get(options, options.rows[0], "sched_calendar_on_relationship_lag");
    }
    if (lagPolicy != "rcal_Successor")
        diagnostic(diagnostics, "SCHEDOPTIONS/sched_calendar_on_relationship_lag",
                   "원본 지연 달력 정책 " + (lagPolicy.isEmpty() ? "미기록" : lagPolicy) +
                       " → JOBS 후행 작업 달력 기준; 계산 동등성 미검증");
    const auto &ct = table(file, "CALENDAR");
    unique(ct, "clndr_id");
    std::map<QString, const Row *> calendarRows;
    for (const auto &r : ct.rows) {
        auto ref = get(ct, r, "proj_id");
        if (!ref.isEmpty() && QString::number(integer(ref, "CALENDAR/proj_id")) != projectId)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_044, {});
        calendarRows[id(ct, r, "clndr_id")] = &r;
    }
    std::map<QString, TaskCalendar> calendars;
    std::set<QString> resolving;
    int calendarNodes = 0;
    std::size_t holidayCount = 0;
    std::function<void(const QString &, int)> resolve = [&](const QString &calendarId, int depth) {
        if (calendars.contains(calendarId))
            return;
        if (depth > 32 || resolving.contains(calendarId) || !calendarRows.contains(calendarId))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_045, {diagnosticArgument(calendarId)});
        resolving.insert(calendarId);
        const auto &r = *calendarRows.at(calendarId);
        TaskCalendar c;
        c.id = "xer-cal-" + str(calendarId);
        c.name = str(get(ct, r, "clndr_name"));
        c.weekdays.clear();
        c.periods.clear();
        auto parent = get(ct, r, "base_clndr_id");
        if (!parent.isEmpty()) {
            parent = QString::number(integer(parent, "CALENDAR/base_clndr_id"));
            resolve(parent, depth + 1);
            c.holidays = calendars.at(parent).holidays;
        }
        auto data = get(ct, r, "clndr_data");
        CalendarParser parser(data, calendarNodes);
        auto root = parser.parse();
        if (root.name != "CalendarData" || !root.attrs.empty())
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_046, {});
        for (const auto &section : root.children)
            if (section.name != "DaysOfWeek" && section.name != "Exceptions") {
                if (section.name == "VIEW")
                    diagnostic(diagnostics, "CALENDAR/" + calendarId + "/VIEW", "달력 표시 옵션은 원본에만 보존");
                else
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_047, {diagnosticArgument(section.name)});
            }
        auto *week = find(root, "DaysOfWeek");
        if (!week || week->children.size() != 7 || !week->attrs.empty())
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_048, {});
        std::set<int> days;
        for (const auto &day : week->children) {
            int type = int(integer(day.name, "CALENDAR/day", 1, 7));
            if (!days.insert(type).second || !day.attrs.empty())
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_049, {});
            std::vector<WorkPeriod> periods;
            for (const auto &hours : day.children) {
                if (hours.attrs.size() != 2 || !hours.attrs.contains("s") || !hours.attrs.contains("f") ||
                    !hours.children.empty())
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_050, {});
                auto end = clockMinute(hours.attrs.at("f"));
                periods.push_back({clockMinute(hours.attrs.at("s")), end == 0 ? 1440 : end});
            }
            if (!periods.empty()) {
                c.weekdays.push_back(type == 1 ? 7 : type - 1);
                if (c.periods.empty())
                    c.periods = periods;
                else if (c.periods != periods)
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_051, {});
            }
        }
        std::sort(c.weekdays.begin(), c.weekdays.end());
        std::set<std::string> holidays(c.holidays.begin(), c.holidays.end()), ownDates;
        if (auto *exceptions = find(root, "Exceptions")) {
            if (!exceptions->attrs.empty())
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_052, {});
            for (const auto &exception : exceptions->children) {
                if (exception.attrs.size() != 1 || !exception.attrs.contains("d") || !exception.children.empty())
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_053, {});
                auto day = QDate(1899, 12, 30)
                               .addDays(integer(exception.attrs.at("d"), "CALENDAR/exception", -693593, 2958465));
                auto date = str(day.toString(Qt::ISODate));
                if (!validDate(date) || !ownDates.insert(date).second)
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_054, {});
                holidays.insert(date);
            }
        }
        if (holidays.size() > 10000)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_055, {});
        holidayCount += holidays.size();
        if (holidayCount > 100000)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_056, {});
        c.holidays.assign(holidays.begin(), holidays.end());
        calendars[calendarId] = c;
        p.calendars.push_back(c);
        map("calendar", calendarId, c.id);
        resolving.erase(calendarId);
    };
    for (const auto &[calendarId, r] : calendarRows) {
        (void)r;
        resolve(calendarId, 0);
    }
    auto defaultCalendar = id(projects, pr, "clndr_id");
    if (!calendars.contains(defaultCalendar))
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_057, {});
    auto calendar = [&](const Table &t, const Row &r) {
        auto ref = get(t, r, "clndr_id");
        if (ref.isEmpty())
            ref = defaultCalendar;
        else
            ref = QString::number(integer(ref, path(t, r, "clndr_id")));
        if (!calendars.contains(ref))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_058, {diagnosticArgument(ref)});
        return calendars.at(ref).id;
    };
    const auto &wbs = table(file, "PROJWBS");
    unique(wbs, "wbs_id");
    std::map<QString, std::string> wbsIds;
    QString rootId;
    std::map<QString, QString> parents;
    for (const auto &r : wbs.rows) {
        projectRef(wbs, r, projectId);
        auto wid = id(wbs, r, "wbs_id");
        auto rootFlag = get(wbs, r, "proj_node_flag");
        if (rootFlag != "Y" && rootFlag != "N")
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_059, {});
        if (rootFlag == "Y") {
            if (!rootId.isEmpty())
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_060, {});
            rootId = wid;
            p.name = str(get(wbs, r, "wbs_name"));
            wbsIds[wid] = "";
            map("project-root", wid, "");
            if (!get(wbs, r, "parent_wbs_id").isEmpty())
                diagnostic(diagnostics, path(wbs, r, "parent_wbs_id"),
                           "프로젝트 루트의 외부 EPS 부모는 native 계층에 포함하지 않음");
        } else {
            wbsIds[wid] = "xer-wbs-" + str(wid);
            parents[wid] = get(wbs, r, "parent_wbs_id");
        }
    }
    if (rootId.isEmpty())
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_061, {});
    for (const auto &r : wbs.rows) {
        auto wid = id(wbs, r, "wbs_id");
        if (wid == rootId)
            continue;
        auto parent = QString::number(integer(parents.at(wid), "PROJWBS/parent_wbs_id"));
        if (!wbsIds.contains(parent))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_062, {});
        p.wbsNodes.push_back({wbsIds.at(wid), str(get(wbs, r, "wbs_name")), wbsIds.at(parent)});
        map("wbs", wid, wbsIds.at(wid));
    }
    const auto &tasks = table(file, "TASK");
    unique(tasks, "task_id");
    std::map<QString, std::string> taskIds;
    for (const auto &r : tasks.rows) {
        projectRef(tasks, r, projectId);
        auto tid = id(tasks, r, "task_id"), wid = id(tasks, r, "wbs_id");
        if (!wbsIds.contains(wid))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_063, {});
        auto type = get(tasks, r, "task_type");
        if (type != "TT_Task" && type != "TT_Rsrc" && type != "TT_Mile" && type != "TT_FinMile")
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_064, {diagnosticArgument(type)});
        Task t;
        t.id = "xer-task-" + str(tid);
        t.name = str(get(tasks, r, "task_name"));
        t.wbsId = wbsIds.at(wid);
        t.calendarId = calendar(tasks, r);
        auto d = get(tasks, r, "target_drtn_hr_cnt");
        if (!d.isEmpty())
            t.durationMinutes = minutes(d, path(tasks, r, "target_drtn_hr_cnt"), diagnostics);
        if (type == "TT_Mile" || type == "TT_FinMile") {
            if (t.durationMinutes != 0)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_065, {});
            diagnostic(diagnostics, path(tasks, r, "task_type"), "시작/완료 마일스톤을 native 0분 이벤트로 투영");
        } else if (type == "TT_Rsrc")
            diagnostic(diagnostics, path(tasks, r, "task_type"), "자원 종속 작업을 기간 기반 작업으로 투영");
        auto durationType = get(tasks, r, "duration_type");
        if (durationType != "DT_FixedDrtn")
            diagnostic(diagnostics, path(tasks, r, "duration_type"),
                       "작업 유형/공수 유지 규칙은 기간 기반 고정 기간으로 투영");
        if (!get(tasks, r, "cstr_type2").isEmpty() || !get(tasks, r, "cstr_date2").isEmpty())
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_066, {});
        auto constraint = get(tasks, r, "cstr_type");
        if (!constraintMap.contains(constraint))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_067, {diagnosticArgument(constraint)});
        t.constraintType = constraintMap.at(constraint);
        auto at = get(tasks, r, "cstr_date");
        if (t.constraintType != ConstraintType::ASAP && t.constraintType != ConstraintType::ALAP)
            t.constraintAt = str(stamp(at, path(tasks, r, "cstr_date")));
        else if (!at.isEmpty())
            diagnostic(diagnostics, path(tasks, r, "cstr_date"), "날짜 없는 제약의 원본 시각은 적용하지 않음");
        if (constraint == "CS_MSO" || constraint == "CS_MEO")
            diagnostic(diagnostics, path(tasks, r, "cstr_type"),
                       "Start/Finish On을 native 하드 고정 날짜로 투영 · 의미 차이 있음");
        taskIds[tid] = t.id;
        map("task", tid, t.id, get(tasks, r, "task_code"));
        p.tasks.push_back(std::move(t));
    }
    std::map<std::string, std::size_t> taskIndex;
    for (std::size_t i = 0; i < p.tasks.size(); ++i)
        taskIndex[p.tasks[i].id] = i;
    const auto &links = table(file, "TASKPRED");
    unique(links, "task_pred_id");
    for (const auto &r : links.rows) {
        projectRef(links, r, projectId);
        projectRef(links, r, projectId, "pred_proj_id");
        auto to = id(links, r, "task_id"), from = id(links, r, "pred_task_id");
        if (!taskIds.contains(to) || !taskIds.contains(from))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_068, {});
        auto type = get(links, r, "pred_type");
        if (type != "PR_FS" && type != "PR_SS" && type != "PR_FF" && type != "PR_SF")
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_069, {});
        auto lag = minutes(get(links, r, "lag_hr_cnt"), path(links, r, "lag_hr_cnt"), diagnostics, true);
        auto &t = p.tasks[taskIndex.at(taskIds.at(to))];
        t.predecessors.push_back(taskIds.at(from));
        if (type != "PR_FS" || lag)
            t.dependencyRules.push_back({taskIds.at(from), dependencyTypeFromToken(str(type.mid(3))), lag});
    }
    const auto &resources = table(file, "RSRC");
    unique(resources, "rsrc_id");
    std::map<QString, std::string> resourceIds;
    const auto &rates = table(file, "RSRCRATE");
    unique(rates, "rsrc_rate_id");
    std::map<QString, std::vector<const Row *>> rateRows;
    for (const auto &r : rates.rows)
        rateRows[id(rates, r, "rsrc_id")].push_back(&r);
    for (const auto &r : resources.rows) {
        auto rid = id(resources, r, "rsrc_id");
        auto type = get(resources, r, "rsrc_type");
        if (type != "RT_Labor" && type != "RT_Equip")
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_070, {diagnosticArgument(type)});
        Resource resource;
        resource.id = "xer-resource-" + str(rid);
        resource.name = str(get(resources, r, "rsrc_name"));
        resource.type = type == "RT_Equip" ? "equipment" : "labor";
        resource.calendarId = calendar(resources, r);
        resource.maxUnits.reset();
        const auto &rows = rateRows[rid];
        if (rows.size() == 1) {
            const auto &rate = *rows.front();
            auto at = get(rates, rate, "start_date"), capacity = get(rates, rate, "max_qty_per_hr");
            if (!at.isEmpty())
                stamp(at, path(rates, rate, "start_date"));
            if (!capacity.isEmpty() && (at.isEmpty() || at <= qs(p.startDate) + " 09:00"))
                resource.maxUnits = units(capacity, path(rates, rate, "max_qty_per_hr"), true);
        }
        if (!resource.maxUnits)
            diagnostic(diagnostics, path(resources, r, "rsrc_id"),
                       "가용량 미정 · 단일 유효 기본 가용량 이외 곡선을 변환하지 않음");
        resourceIds[rid] = resource.id;
        map("resource", rid, resource.id);
        p.resources.push_back(std::move(resource));
    }
    for (const auto &[rid, rows] : rateRows) {
        (void)rows;
        if (!resourceIds.contains(rid))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_071, {});
    }
    const auto &assignments = table(file, "TASKRSRC");
    unique(assignments, "taskrsrc_id");
    for (const auto &r : assignments.rows) {
        projectRef(assignments, r, projectId);
        auto tid = id(assignments, r, "task_id"), rid = id(assignments, r, "rsrc_id");
        if (!taskIds.contains(tid) || !resourceIds.contains(rid))
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_072, {});
        auto curve = get(assignments, r, "curv_id");
        if (!curve.isEmpty() && curve != "0")
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_073, {});
        p.assignments.push_back(
            {taskIds.at(tid), resourceIds.at(rid),
             units(get(assignments, r, "target_qty_per_hr"), path(assignments, r, "target_qty_per_hr"), false)});
        map("assignment", id(assignments, r, "taskrsrc_id"), taskIds.at(tid) + "/" + resourceIds.at(rid));
    }
    for (const auto &[name, t] : file.tables) {
        if (t.rows.empty())
            continue;
        if (t.used.empty()) {
            diagnostic(diagnostics, name, "미지원 테이블 · " + QString::number(t.rows.size()) + "행 원본에 보존");
            continue;
        }
        for (const auto &field : t.fields)
            if (!t.used.contains(field))
                diagnostic(diagnostics, name + "/" + field, "미지원 필드 · 원본에 보존");
    }
    validate(p);
    graph(p);
    auto report = reportFor(p, true);
    report["source"] = QJsonObject{{"name", QFileInfo(sourceName).fileName()},
                                   {"sha256", hash(bytes)},
                                   {"encoding", encodingName(encoding)},
                                   {"header", QJsonArray::fromStringList(file.header)},
                                   {"version", file.header[1]},
                                   {"producer", "unverified"},
                                   {"projectCode", code},
                                   {"lagPolicy", lagPolicy}};
    report["mapping"] = mapping;
    report["diagnostics"] = diagnostics;
    return {std::move(p), bytes, report, true};
}

namespace {
QString timeText(int minutes) {
    return QString("%1:%2").arg(minutes / 60, 2, 10, QChar('0')).arg(minutes % 60, 2, 10, QChar('0'));
}
QString record(const QString &name, const QString &attrs, const QStringList &children = {}) {
    return "(0||" + name + "(" + attrs + ")(" + children.join("") + "))";
}
QString calendarData(const TaskCalendar &c) {
    QStringList days, exceptions;
    for (int day = 1; day <= 7; ++day) {
        int native = day == 1 ? 7 : day - 1;
        QStringList periods;
        if (std::find(c.weekdays.begin(), c.weekdays.end(), native) != c.weekdays.end()) {
            int i = 0;
            for (const auto &p : c.periods)
                periods << record(QString::number(i++), "s|" + timeText(p.begin) + "|f|" + timeText(p.end));
        }
        days << record(QString::number(day), {}, periods);
    }
    int i = 0;
    for (const auto &holiday : c.holidays)
        exceptions << record(
            QString::number(i++),
            "d|" + QString::number(QDate(1899, 12, 30).daysTo(QDate::fromString(qs(holiday), Qt::ISODate))));
    return record("CalendarData", {}, {record("DaysOfWeek", {}, days), record("Exceptions", {}, exceptions)});
}
QString unitText(int units) {
    return QString::number(units / 100) + "." + QString::number(units % 100).rightJustified(2, '0');
}
QString hourText(std::int64_t minutes) {
    const auto sign = minutes < 0 ? "-" : "";
    auto n = std::abs(minutes);
    auto micros = (n * 1000000 + 30) / 60;
    return sign + QString::number(micros / 1000000) + "." + QString::number(micros % 1000000).rightJustified(6, '0');
}
} // namespace
PrimaveraXerExchange exportPrimaveraXer(const Project &p) {
    validate(p);
    graph(p);
    if (p.tasks.size() > 10000 || p.wbsNodes.size() >= 10000 || p.resources.size() > 10000 ||
        p.assignments.size() > 10000 || p.calendars.size() >= 10000)
        throw AppError(ErrorCode::FILE_PRIMAVERAXER_074, {});
    QByteArray bytes("\xef\xbb\xbf");
    auto line = [&](const QStringList &cells) {
        for (const auto &cell : cells) {
            if (cell.size() > 1024 * 1024)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_075, {});
            for (auto c : cell)
                if (forbiddenTextCharacter(c))
                    throw AppError(ErrorCode::FILE_PRIMAVERAXER_076, {});
        }
        bytes += cells.join('\t').toUtf8() + "\r\n";
        if (bytes.size() > byteLimit)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_077, {});
    };
    auto header = [&](const QString &name, const QStringList &fields) {
        line({"%T", name});
        QStringList values{"%F"};
        values.append(fields);
        line(values);
    };
    auto row = [&](const QStringList &values) {
        QStringList cells{"%R"};
        cells.append(values);
        line(cells);
    };
    QJsonArray diagnostics, mapping;
    auto map = [&](const QString &kind, int id, const std::string &target) {
        mapping.append(QJsonObject{{"kind", kind},
                                   {"xerId", QString::number(id)},
                                   {"jobsId", qs(target)},
                                   {"activityCode", kind == "task" ? "J" + QString::number(id) : QString{}}});
    };
    const auto projectCode = "JOBS-" + hash(qs(p.documentId).toUtf8()).left(12);
    line({"ERMHDR", "20.12", qs(p.startDate), "Project", "JOBS", "JOBS", "dbxDatabaseNoName", "Project Management",
          qs(p.currency)});
    diagnostic(diagnostics, "PROJECT",
               "기간 기반 계획 투영 · P6 계산/재저장 호환은 미검증. 전체 현재 입력/승인·실적/원가는 source.jobs 참조.");
    header("CURRTYPE",
           {"curr_id", "decimal_digit_cnt", "curr_symbol", "decimal_symbol", "digit_group_symbol", "pos_curr_fmt_type",
            "neg_curr_fmt_type", "curr_type", "curr_short_name", "group_digit_cnt", "base_exch_rate"});
    row({"1", QString::number(currencyDecimals(p.currency)), qs(p.currency), ".", ",", "#1.1", "(#1.1)", qs(p.currency),
         qs(p.currency), "3", "1"});
    header("PROJECT", {"proj_id", "proj_short_name", "clndr_id", "plan_start_date", "def_task_type",
                       "def_duration_type", "task_code_prefix", "task_code_base", "task_code_step"});
    row({"1", projectCode, "1", qs(p.startDate) + " 09:00", "TT_Task", "DT_FixedDrtn", "J", "1", "1"});
    header("SCHEDOPTIONS", {"schedoptions_id", "proj_id", "sched_calendar_on_relationship_lag"});
    row({"1", "1", "rcal_Successor"});
    header("CALENDAR", {"clndr_id", "default_flag", "clndr_name", "proj_id", "base_clndr_id", "clndr_type",
                        "day_hr_cnt", "week_hr_cnt", "rsrc_private", "clndr_data"});
    std::map<std::string, int> calendars{{"", 1}};
    TaskCalendar standard;
    standard.name = "JOBS standard";
    standard.holidays = p.holidays;
    std::vector<TaskCalendar> allCalendars{standard};
    allCalendars.insert(allCalendars.end(), p.calendars.begin(), p.calendars.end());
    int next = 1;
    for (const auto &c : allCalendars) {
        if (c.holidays.size() > 10000)
            throw AppError(ErrorCode::FILE_PRIMAVERAXER_078, {});
        int minutes = 0;
        for (const auto &period : c.periods)
            minutes += period.end - period.begin;
        int uid = next++;
        calendars[c.id] = uid;
        map("calendar", uid, c.id);
        row({QString::number(uid),
             uid == 1 ? "Y" : "N",
             qs(c.name),
             "1",
             {},
             "CA_Project",
             hourText(minutes),
             hourText(minutes * int(c.weekdays.size())),
             "N",
             calendarData(c)});
    }
    header("PROJWBS",
           {"wbs_id", "proj_id", "seq_num", "proj_node_flag", "wbs_short_name", "wbs_name", "parent_wbs_id"});
    row({"1", "1", "0", "Y", projectCode, qs(p.name), {}});
    map("project-root", 1, "");
    std::map<std::string, int> wbs{{"", 1}};
    next = 2;
    for (const auto &g : p.wbsNodes) {
        wbs[g.id] = next;
        map("wbs", next++, g.id);
    }
    for (const auto &g : p.wbsNodes)
        row({QString::number(wbs.at(g.id)), "1", QString::number(wbs.at(g.id)), "N",
             "W" + QString::number(wbs.at(g.id)), qs(g.name), QString::number(wbs.at(g.parentId))});
    header("TASK", {"task_id", "proj_id", "wbs_id", "clndr_id", "task_code", "task_name", "task_type", "duration_type",
                    "status_code", "complete_pct_type", "target_drtn_hr_cnt", "remain_drtn_hr_cnt", "cstr_type",
                    "cstr_date", "cstr_type2", "cstr_date2"});
    std::map<std::string, int> tasks;
    next = 1;
    const std::map<ConstraintType, QString> reverse{
        {ConstraintType::ASAP, ""},          {ConstraintType::ALAP, "CS_ALAP"}, {ConstraintType::MSO, "CS_MANDSTART"},
        {ConstraintType::MFO, "CS_MANDFIN"}, {ConstraintType::SNET, "CS_MSOA"}, {ConstraintType::SNLT, "CS_MSOB"},
        {ConstraintType::FNET, "CS_MEOA"},   {ConstraintType::FNLT, "CS_MEOB"}};
    for (const auto &t : p.tasks) {
        int uid = next++;
        tasks[t.id] = uid;
        map("task", uid, t.id);
        auto duration = t.durationMinutes ? hourText(*t.durationMinutes) : QString{};
        row({QString::number(uid),
             "1",
             QString::number(wbs.at(t.wbsId)),
             QString::number(calendars.at(t.calendarId)),
             "J" + QString::number(uid),
             qs(t.name),
             t.durationMinutes == 0 ? "TT_Mile" : "TT_Task",
             "DT_FixedDrtn",
             "TK_NotStart",
             "CP_Drtn",
             duration,
             duration,
             reverse.at(t.constraintType),
             t.constraintAt ? qs(*t.constraintAt) : QString{},
             {},
             {}});
        if (!t.durationMinutes)
            diagnostic(diagnostics, "TASK/" + qs(t.id) + "/target_drtn_hr_cnt",
                       "미정 기간 공백 · 외부 앱 기본값 해석 가능");
        if (t.deadlineAt)
            diagnostic(diagnostics, "TASK/" + qs(t.id) + "/deadline",
                       "소프트 마감일을 expected finish/하드 제약으로 변환하지 않음");
        if (t.taskType != TaskType::FixedDuration || t.workUnitMinutes || !t.breaks.empty())
            diagnostic(diagnostics, "TASK/" + qs(t.id), "공수/작업 유형/분할 규칙 대신 명목 기간을 투영");
    }
    header("TASKPRED",
           {"task_pred_id", "task_id", "pred_task_id", "proj_id", "pred_proj_id", "pred_type", "lag_hr_cnt"});
    next = 1;
    for (const auto &t : p.tasks)
        for (const auto &d : dependencies(t)) {
            if (next > 10000)
                throw AppError(ErrorCode::FILE_PRIMAVERAXER_079, {});
            row({QString::number(next++), QString::number(tasks.at(t.id)), QString::number(tasks.at(d.predecessorId)),
                 "1", "1", "PR_" + qs(enumToken(d.type)), hourText(d.lagMinutes)});
        }
    header("RSRC", {"rsrc_id", "clndr_id", "curr_id", "rsrc_name", "rsrc_short_name", "rsrc_type", "active_flag"});
    std::map<std::string, int> resources;
    next = 1;
    for (const auto &r : p.resources) {
        int uid = next++;
        resources[r.id] = uid;
        map("resource", uid, r.id);
        row({QString::number(uid), QString::number(calendars.at(r.calendarId)), "1", qs(r.name),
             "R" + QString::number(uid), r.type == "equipment" ? "RT_Equip" : "RT_Labor", "Y"});
        if (!r.rates.empty() || !r.exceptions.empty())
            diagnostic(diagnostics, "RSRC/" + qs(r.id), "단가/날짜별 가용량 예외는 source.jobs에만 보존");
        if (!r.maxUnits)
            diagnostic(diagnostics, "RSRC/" + qs(r.id) + "/capacity", "가용량 미정 · 외부 앱 기본값 해석 가능");
    }
    header("RSRCRATE", {"rsrc_rate_id", "rsrc_id", "start_date", "max_qty_per_hr"});
    next = 1;
    for (const auto &r : p.resources)
        if (r.maxUnits)
            row({QString::number(next++), QString::number(resources.at(r.id)), qs(p.startDate) + " 00:00",
                 unitText(*r.maxUnits)});
    header("TASKRSRC", {"taskrsrc_id", "task_id", "proj_id", "rsrc_id", "target_qty_per_hr", "remain_qty_per_hr",
                        "target_qty", "remain_qty"});
    next = 1;
    for (const auto &a : p.assignments) {
        map("assignment", next, a.taskId + "/" + a.resourceId);
        const auto &task = p.tasks[std::size_t(tasks.at(a.taskId) - 1)];
        auto quantity = task.durationMinutes ? QString::number(double(*task.durationMinutes) * a.units / 6000.0, 'f', 6)
                                             : QString{};
        row({QString::number(next++), QString::number(tasks.at(a.taskId)), "1",
             QString::number(resources.at(a.resourceId)), unitText(a.units), unitText(a.units), quantity, quantity});
    }
    line({"%E"});
    importPrimaveraXer(bytes, "project.xer");
    if (!p.baselines.empty() || !p.progress.empty() || !p.costEntries.empty() || !p.scenarios.empty() || p.statusAt ||
        !p.taskCosts.empty())
        diagnostic(diagnostics, "PROJECT/history-costs", "기준선·실적·원가·상태일·시나리오는 source.jobs에만 보존");
    diagnostic(diagnostics, "PROJECT/IDs",
               "새 XER ID/Activity Code를 생성합니다. P6의 기존 프로젝트/활동 업데이트 동일성은 보장하지 않습니다.");
    auto report = reportFor(p, false);
    report["source"] = QJsonObject{{"name", "source.jobs"}, {"product", "JOBS"}};
    report["outputProjectCode"] = projectCode;
    report["outputEncoding"] = "UTF-8 BOM";
    report["outputVersion"] = "20.12";
    report["xerSha256"] = hash(bytes);
    report["mapping"] = mapping;
    report["diagnostics"] = diagnostics;
    return {p, bytes, report, false};
}
QString publishPrimaveraXer(const PrimaveraXerExchange &e, const QString &destination) {
    return publishPlanExchange(e.project, e.xer, e.report, e.importing, "xer", destination);
}
QString primaveraXerPreview(const PrimaveraXerExchange &e) {
    auto text = QString("%1\n작업 %2 · WBS %3 · 달력 %4 · 자원 %5 · 배정 %6\n\n기간 기반 계획 변환 · P6 재계산/왕복 "
                        "호환: 미검증\nJOBS "
                        "지연 정책: 후행 작업 달력 · 시작일 기준 09:00\n원본 보존: %7\n\n원본/진단은 교환 폴더에 "
                        "보관합니다. .jobs만 사본 "
                        "저장/이동하면 동반 파일은 자동 복제되지 않습니다.\n\n")
                    .arg(qs(e.project.name))
                    .arg(e.project.tasks.size())
                    .arg(e.project.wbsNodes.size())
                    .arg(e.project.calendars.size())
                    .arg(e.project.resources.size())
                    .arg(e.project.assignments.size())
                    .arg(e.importing ? "original.xer 바이트 보존" : "현재 입력/이력의 source.jobs 사본");
    int count = 0;
    for (const auto &d : e.report["diagnostics"].toArray()) {
        if (count++ == 2000) {
            text += "이후 진단은 conversion.json 참조\n";
            break;
        }
        auto obj = d.toObject();
        text += obj["path"].toString() + ": " + obj["message"].toString() + "\n";
    }
    return text;
}
} // namespace jobs
