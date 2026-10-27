#include "application/Document.h"
#include "calculation/Scheduler.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/PrimaveraXer.h"
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <functional>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool b, const char *m) {
    if (!b)
        throw std::runtime_error(m);
}
void rejects(const std::function<void()> &f) {
    bool rejected = false;
    try {
        f();
    } catch (const std::exception &) {
        rejected = true;
    }
    check(rejected, "expected rejection");
}
QByteArray read(const QString &p) {
    QFile f(p);
    check(f.open(QIODevice::ReadOnly), "file read failed");
    return f.readAll();
}
QByteArray modify(const QByteArray &source, const QString &table, const QString &column, int row, const QString &value) {
    auto lines = QString::fromUtf8(source).split('\n', Qt::KeepEmptyParts);
    bool inside = false;
    QStringList fields;
    for (auto &line : lines) {
        const bool cr = line.endsWith('\r');
        auto text = line;
        if (cr)
            text.chop(1);
        auto cells = text.split('\t', Qt::KeepEmptyParts);
        if (cells[0] == "%T") {
            inside = cells.size() > 1 && cells[1] == table;
            continue;
        }
        if (!inside)
            continue;
        if (cells[0] == "%F")
            fields = cells.mid(1);
        if (cells[0] == "%R" && row-- == 0) {
            auto index = fields.indexOf(column);
            check(index >= 0, "test column missing");
            cells[index + 1] = value;
            line = cells.join('\t') + (cr ? "\r" : "");
            return lines.join('\n').toUtf8();
        }
    }
    throw std::runtime_error("test table row missing");
}
bool diagnostic(const jobs::PrimaveraXerExchange &e, const QString &part) {
    for (const auto &d : e.report["diagnostics"].toArray())
        if (d.toObject()["path"].toString().contains(part))
            return true;
    return false;
}
const jobs::TaskSchedule &scheduled(const jobs::ScheduleResult &r, const std::string &id) {
    for (const auto &t : r.tasks)
        if (t.taskId == id)
            return t;
    throw std::runtime_error("missing schedule");
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        auto source = read(QString::fromUtf8(JOBS_XER_FIXTURE));
        auto imported = jobs::importPrimaveraXer(source, "골든.xer");
        const auto &p = imported.project;
        check(p.name == "한글 XER 계획" && p.tasks.size() == 6 && p.wbsNodes.size() == 2 && p.resources.size() == 2 &&
                  p.assignments.size() == 2,
              "project/WBS/task/resource counts");
        check(p.tasks[0].id == "xer-task-20" && p.tasks[0].wbsId == "xer-wbs-11" && p.tasks[5].wbsId == "xer-wbs-12" &&
                  p.wbsNodes[1].parentId == "xer-wbs-11",
              "ID hierarchy mapping");
        check(p.calendars.size() == 2 && p.calendars[0].holidays == std::vector<std::string>{"2026-10-27"} &&
                  p.calendars[1].holidays == std::vector<std::string>{"2026-10-27", "2026-10-30"},
              "calendar epoch/inherited exceptions");
        check(p.resources[0].maxUnits == 150 && p.resources[1].maxUnits == 0 && p.resources[1].type == "equipment" &&
                  p.assignments[0].units == 50 && p.assignments[1].units == 125,
              "units/capacity zero/equipment");
        check(p.tasks[1].constraintType == "SNET" && p.tasks[1].constraintAt == "2026-10-28 10:00" && p.tasks[5].durationMinutes == 0,
              "constraint/milestone");
        for (std::size_t i = 1; i < 5; ++i) {
            auto edges = jobs::dependencies(p.tasks[i]);
            const std::vector<std::string> types{"FS", "SS", "FF", "SF"};
            check(edges.size() == 1 && edges[0].type == types[i - 1] && edges[0].lagMinutes == ((i == 2 || i == 3) ? -60 : 60),
                  "four links/negative hours lag");
        }
        check(p.progress.empty() && p.costEntries.empty() && p.taskCosts.empty() && diagnostic(imported, "act_start_date") &&
                  diagnostic(imported, "cost_per_qty") && diagnostic(imported, "NOTEBOOK"),
              "facts not invented/loss report");
        auto schedule = jobs::calculate(p);
        check(schedule.status == jobs::CalculationStatus::Valid, "schedule valid");
        check(scheduled(schedule, "xer-task-20").start == "2026-10-26 09:00" &&
                  scheduled(schedule, "xer-task-20").finish == "2026-10-26 18:00",
              "independent original duration dates");
        check(scheduled(schedule, "xer-task-30").start == "2026-10-28 10:00" &&
                  scheduled(schedule, "xer-task-30").finish == "2026-10-28 12:00" &&
                  scheduled(schedule, "xer-task-50").start == "2026-10-26 15:00" &&
                  scheduled(schedule, "xer-task-50").finish == "2026-10-26 17:00",
              "independent lag/holiday/FF dates");
        QTemporaryDir dir;
        check(dir.isValid(), "temp directory");
        auto folder = jobs::publishPrimaveraXer(imported, dir.filePath("import"));
        check(read(QDir(folder).filePath("original.xer")) == source, "original bytes preserved");
        auto report = QJsonDocument::fromJson(read(QDir(folder).filePath("conversion.json"))).object();
        check(report["source"].toObject()["sha256"].toString().toLatin1() ==
                  QCryptographicHash::hash(source, QCryptographicHash::Sha256).toHex(),
              "original hash");
        check(report["source"].toObject()["projectCode"] == "SITE-01" && report["source"].toObject()["encoding"] == "UTF-8" &&
                  report["mapping"].toArray().size() == 15,
              "source code/encoding/mapping");
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.open(QDir(folder).filePath("project.jobs").toStdString());
        check(*doc.project() == p && !doc.dirty(), "native reopen");
        auto native = read(QDir(folder).filePath("project.jobs"));
        rejects([&] { jobs::publishPrimaveraXer(imported, folder); });
        check(read(QDir(folder).filePath("project.jobs")) == native, "existing target preserved");
        rejects([&] { jobs::publishPrimaveraXer(imported, dir.filePath("missing/folder")); });
        auto snapshot = *doc.project();
        auto generation = doc.sessionGeneration();
        auto exported = jobs::exportPrimaveraXer(p);
        auto back = jobs::importPrimaveraXer(exported.xer, "roundtrip.xer");
        check(back.project.tasks.size() == 6 && back.project.wbsNodes.size() == 2 && back.project.resources.size() == 2 &&
                  back.project.assignments.size() == 2,
              "roundtrip counts");
        auto again = jobs::calculate(back.project);
        check(again.status == jobs::CalculationStatus::Valid, "roundtrip schedule");
        for (std::size_t i = 0; i < p.tasks.size(); ++i) {
            const auto &a = p.tasks[i];
            const auto &b = back.project.tasks[i];
            check(a.name == b.name && a.durationMinutes == b.durationMinutes && a.constraintType == b.constraintType &&
                      a.constraintAt == b.constraintAt,
                  "roundtrip input");
            const auto &x = scheduled(schedule, a.id);
            const auto &y = scheduled(again, b.id);
            check(x.start == y.start && x.finish == y.finish && x.totalFloatMinutes == y.totalFloatMinutes, "roundtrip schedule semantics");
        }
        auto output = jobs::publishPrimaveraXer(exported, dir.filePath("export"));
        check(store.load(QDir(output).filePath("source.jobs").toStdString()).project == p && *doc.project() == snapshot &&
                  doc.sessionGeneration() == generation && !doc.dirty() && !doc.canUndo(),
              "export source snapshot/document isolation");
        auto un = p;
        un.tasks[0].durationMinutes.reset();
        un.resources[0].maxUnits.reset();
        auto unExport = jobs::exportPrimaveraXer(un);
        auto unBack = jobs::importPrimaveraXer(unExport.xer, "unknown.xer");
        check(!unBack.project.tasks[0].durationMinutes && !unBack.project.resources[0].maxUnits && diagnostic(unExport, "capacity"),
              "unknown not zero");
        auto one = p;
        one.tasks[0].durationMinutes = 1;
        one.tasks[2].dependencyRules[0].lagMinutes = -1;
        auto oneExport = jobs::exportPrimaveraXer(one);
        auto oneBack = jobs::importPrimaveraXer(oneExport.xer, "one-minute.xer");
        check(oneBack.project.tasks[0].durationMinutes == 1 && oneBack.project.tasks[2].dependencyRules[0].lagMinutes == -1 &&
                  diagnostic(oneBack, "target_drtn_hr_cnt"),
              "one minute decimal quantization");
        jobs::Project around;
        around.documentId = "around-clock";
        around.name = "24-hour";
        around.startDate = "2026-10-26";
        around.calculationProfile = jobs::effortProfile;
        around.calendars = {{"around", "24-hour", {1, 2, 3, 4, 5, 6, 7}, {{0, 1440}}, {}}};
        around.tasks = {{"a", "24-hour task", 60, {}}};
        around.tasks[0].calendarId = "around";
        auto aroundExport = jobs::exportPrimaveraXer(around);
        auto midnight = aroundExport.xer;
        midnight.replace("f|24:00", "f|00:00");
        auto aroundBack = jobs::importPrimaveraXer(midnight, "midnight.xer");
        check(aroundBack.project.calendars.back().periods == std::vector<jobs::WorkPeriod>{{0, 1440}},
              "midnight endpoint/24-hour calendar");
        auto work = p;
        work.tasks[0].taskType = "fixed-work";
        work.tasks[0].workDriven = true;
        work.tasks[0].workUnitMinutes = 24000;
        work.tasks[0].breaks = {{"2026-10-26 10:00", "2026-10-26 11:00"}};
        work.tasks[0].deadlineAt = "2026-10-28 18:00";
        auto projected = jobs::exportPrimaveraXer(work);
        check(diagnostic(projected, "deadline") && diagnostic(projected, "xer-task-20") &&
                  jobs::importPrimaveraXer(projected.xer, "work.xer").project.tasks[0].taskType == "fixed-duration",
              "work/split/deadline explicit loss");
        jobs::Project latin;
        latin.documentId = "latin";
        latin.name = "Latin";
        latin.startDate = "2026-10-26";
        latin.tasks = {{"a", "Café €", 1, {}}};
        auto utf8 = jobs::exportPrimaveraXer(latin).xer;
        auto latinText = QString::fromUtf8(utf8);
        if (latinText.startsWith(QChar(0xfeff)))
            latinText.remove(0, 1);
        latinText.replace(QChar(0x20ac), QChar(0x80));
        auto cp = latinText.toLatin1();
        check(jobs::importPrimaveraXer(cp, "cp.xer", jobs::XerEncoding::Windows1252).project.tasks[0].name == "Café €",
              "Windows-1252 explicit decode");
        rejects([&] { jobs::importPrimaveraXer(cp, "bad-encoding.xer"); });
        rejects([&] { jobs::importPrimaveraXer(utf8, "bom.xer", jobs::XerEncoding::Windows1252); });
        auto bad = [&](const QString &t, const QString &f, int r, const QString &v) {
            rejects([&] { jobs::importPrimaveraXer(modify(source, t, f, r, v), "bad.xer"); });
        };
        bad("TASK", "task_id", 1, "20");
        bad("PROJWBS", "wbs_id", 1, "10");
        bad("PROJWBS", "parent_wbs_id", 1, "12");
        bad("PROJWBS", "parent_wbs_id", 1, "999");
        bad("TASK", "wbs_id", 0, "999");
        bad("TASK", "proj_id", 0, "101");
        bad("TASK", "task_type", 0, "TT_LOE");
        bad("TASK", "task_type", 0, "TT_WBS");
        bad("TASK", "target_drtn_hr_cnt", 0, "0.008333");
        bad("TASK", "target_drtn_hr_cnt", 0, "1e3");
        bad("TASK", "cstr_type2", 0, "CS_MSOA");
        bad("TASK", "cstr_date", 1, "2026-10-28T10:00Z");
        bad("TASKPRED", "pred_proj_id", 0, "101");
        bad("TASKPRED", "pred_task_id", 0, "999");
        bad("TASKPRED", "pred_task_id", 0, "30");
        bad("TASKPRED", "task_id", 0, "20");
        auto cyclic = modify(modify(source, "TASKPRED", "task_id", 1, "20"), "TASKPRED", "pred_task_id", 1, "30");
        rejects([&] { jobs::importPrimaveraXer(cyclic, "cycle.xer"); });
        bad("TASKRSRC", "rsrc_id", 0, "999");
        bad("TASKRSRC", "curv_id", 0, "1");
        bad("TASKRSRC", "target_qty_per_hr", 0, "0.505");
        bad("TASKRSRC", "target_qty_per_hr", 0, "0");
        bad("RSRC", "rsrc_type", 0, "RT_Mat");
        bad("RSRCRATE", "rsrc_id", 0, "999");
        bad("CALENDAR", "base_clndr_id", 0, "2");
        bad("CALENDAR", "clndr_data", 0, "(0||CalendarData()())");
        bad("PROJECT", "clndr_id", 0, "999");
        auto calData = QString("(0||CalendarData()((0||DaysOfWeek()((0||1()())(0||2()((0||0(s|10:00|f|11:00)())))(0||3()((0||0(s|09:00|f|"
                               "12:00)())))(0||4()())(0||5()())(0||6()())(0||7()())))))");
        bad("CALENDAR", "clndr_data", 0, calData);
        auto workingException = source;
        const auto holiday = QByteArray::number(QDate(1899, 12, 30).daysTo(QDate(2026, 10, 27)));
        auto marker = "(0||0(d|" + holiday + ")())";
        check(workingException.contains(marker), "holiday mutation target");
        workingException.replace(marker, "(0||0(d|" + holiday + ")((0||0(s|09:00|f|10:00)()))))");
        rejects([&] { jobs::importPrimaveraXer(workingException, "work-exception.xer"); });
        auto schema = source;
        schema.replace("task_id\tproj_id\twbs_id", "task_id\ttask_id\twbs_id");
        rejects([&] { jobs::importPrimaveraXer(schema, "duplicate-fields.xer"); });
        auto noEnd = source.left(source.lastIndexOf("%E"));
        rejects([&] { jobs::importPrimaveraXer(noEnd, "no-end.xer"); });
        rejects([&] { jobs::importPrimaveraXer(source + "junk", "tail.xer"); });
        auto badMarker = source;
        badMarker.replace("%T\tNOTEBOOK", "%X\tNOTEBOOK");
        rejects([&] { jobs::importPrimaveraXer(badMarker, "marker.xer"); });
        auto lines = QString::fromUtf8(source).split('\n');
        for (qsizetype i = 0; i < lines.size(); ++i)
            if (lines[i].startsWith("%T\tPROJECT")) {
                auto row = lines[i + 2];
                lines.insert(i + 3, row);
                break;
            }
        rejects([&] { jobs::importPrimaveraXer(lines.join('\n').toUtf8(), "multi-project.xer"); });
        rejects([&] { jobs::importPrimaveraXer(QByteArray(16 * 1024 * 1024 + 1, ' '), "large.xer"); });
        auto newline = p;
        newline.tasks[0].name = "bad\nname";
        rejects([&] { jobs::exportPrimaveraXer(newline); });
        newline.tasks[0].name = "bad\xe2\x80\xa8name";
        rejects([&] { jobs::exportPrimaveraXer(newline); });
        bad("TASK", "task_name", 0, QString("bad") + QChar(0x2028) + "name");
        check(*doc.project() == snapshot, "failed import changed document");
        auto wide = source;
        wide.replace("%T\tNOTEBOOK", QByteArray("\t").repeated(257) + "\r\n%T\tNOTEBOOK");
        rejects([&] { jobs::importPrimaveraXer(wide, "wide.xer"); });
        rejects([&] { jobs::importPrimaveraXer(QByteArray("\n").repeated(300002), "line-limit.xer"); });
        auto cells = source;
        cells.replace("%R\t1\t원본에만 보존", "%R\t1\t" + QByteArray(1024 * 1024 + 1, 'X'));
        rejects([&] { jobs::importPrimaveraXer(cells, "cell-limit.xer"); });
        auto budget = source;
        budget.replace("%T\tNOTEBOOK", "%T\tCELL_BUDGET\r\n%F\ta\tb\tc\r\n" + QByteArray("%R\t\t\t\r\n").repeated(250001) + "%T\tNOTEBOOK");
        rejects([&] { jobs::importPrimaveraXer(budget, "cell-budget.xer"); });
        QString nested = "(0||CalendarData()())";
        for (int i = 0; i < 33; ++i)
            nested = "(0||X()(" + nested + "))";
        bad("CALENDAR", "clndr_data", 0, nested);
        auto future = modify(source, "RSRCRATE", "start_date", 0, "2027-10-01 00:00");
        auto futureImport = jobs::importPrimaveraXer(future, "future.xer");
        check(!futureImport.project.resources[0].maxUnits, "future capacity not treated as current");
        auto changedPolicy = jobs::importPrimaveraXer(
            modify(source, "SCHEDOPTIONS", "sched_calendar_on_relationship_lag", 0, "rcal_Predecessor"), "policy.xer");
        check(diagnostic(changedPolicy, "sched_calendar_on_relationship_lag"), "lag calendar policy difference not reported");
        jobs::Document tracked(store);
        tracked.create("승인·실적 보존", "2026-10-26", {"보고"});
        tracked.setDuration(0, 60);
        tracked.saveAs(dir.filePath("tracked.jobs").toStdString());
        tracked.captureBaseline("승인", "검증", "원본 보존", "2026-10-26T00:00:00Z");
        tracked.setStatusAt("2026-10-26 09:30");
        jobs::ProgressEntry progress;
        progress.taskId = tracked.project()->tasks[0].id;
        progress.actualStart = "2026-10-26 09:00";
        progress.actualDurationMinutes = 30;
        progress.remainingMinutes = 30;
        progress.reporter = "현장";
        progress.reason = "계측";
        progress.recordedAt = "2026-10-26T00:30:00Z";
        tracked.recordProgress(progress);
        jobs::CostEntry cost;
        cost.taskId = progress.taskId;
        cost.actualMinor = 1234;
        cost.physicalBasisPoints = 5000;
        cost.reporter = "원가";
        cost.reason = "계측";
        cost.recordedAt = "2026-10-26T00:30:00Z";
        tracked.recordCost(cost);
        auto history = *tracked.project();
        auto historyExport = jobs::exportPrimaveraXer(history);
        auto historyFolder = jobs::publishPrimaveraXer(historyExport, dir.filePath("history"));
        check(diagnostic(historyExport, "history-costs") &&
                  store.load(QDir(historyFolder).filePath("source.jobs").toStdString()).project == history && *tracked.project() == history,
              "approved/actual/cost preservation");
        QFile outputFile("primavera-export.xer");
        check(outputFile.open(QIODevice::WriteOnly) && outputFile.write(exported.xer) == exported.xer.size(), "write independent fixture");
        QFile reportFile("primavera-export.json");
        check(reportFile.open(QIODevice::WriteOnly), "write independent report");
        reportFile.write(QJsonDocument(exported.report).toJson());
        QFile aroundFile("primavera-24h.xer");
        check(aroundFile.open(QIODevice::WriteOnly) && aroundFile.write(aroundExport.xer) == aroundExport.xer.size(), "write 24h fixture");
        std::cout << "Primavera XER planning contract, independent dates, roundtrip, encodings, preservation and rejections passed\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
    return 0;
}
