#include "formatting/DiagnosticText.h"
#include "formatting/DiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "calculation/Reports.h"
#include "calculation/Constraints.h"
#include "calculation/Effort.h"
#include "domain/Wbs.h"
#include <algorithm>
#include <set>
#include <map>
#include <unordered_map>
#include <stdexcept>
namespace jobs {
namespace {
ReportCell text(std::string s) {
    return {std::move(s), CellFormat::Text};
}
ReportCell number(std::int64_t n, CellFormat format = CellFormat::Integer) {
    return {n, format};
}
ReportCell optional(std::optional<std::int64_t> n, CellFormat format) {
    return n ? number(*n, format) : ReportCell{{}, format};
}
ReportCell ratio(std::optional<double> n) {
    return n ? ReportCell{*n, CellFormat::Ratio} : ReportCell{{}, CellFormat::Ratio};
}
ReportCell stamp(const std::optional<std::string> &s) {
    return s ? text(*s) : ReportCell{};
}
std::string state(CalculationStatus s) {
    return s == CalculationStatus::Cancelled    ? "취소"
           : s == CalculationStatus::Valid      ? "유효"
           : s == CalculationStatus::Incomplete ? "입력 불완전"
           : s == CalculationStatus::Stale      ? "오래된 결과"
                                                : "충돌";
}
std::string path(const PlanInput &p, std::string id) {
    std::vector<std::string> names;
    while (!id.empty()) {
        auto it = std::find_if(p.wbsNodes.begin(), p.wbsNodes.end(), [&](const auto &n) { return n.id == id; });
        if (it == p.wbsNodes.end())
            break;
        names.push_back(it->name);
        id = it->parentId;
    }
    std::string result;
    for (auto it = names.rbegin(); it != names.rend(); ++it) {
        if (!result.empty())
            result += " / ";
        result += *it;
    }
    return result.empty() ? "프로젝트 전체" : result;
}
bool inRange(const ReportOptions &o, const std::string &d) {
    return (!o.fromDate || d >= *o.fromDate) && (!o.toDate || d <= *o.toDate);
}
} // namespace
bool includeTable(const ReportTable &t, ReportFamily f) {
    return f == ReportFamily::All || t.family == ReportFamily::All || t.family == f;
}
ProjectReport buildReport(const Project &p, const ReportOptions &o) {
    validate(p);
    if (o.reportId.empty() || !validUtcStamp(o.generatedAt) || (o.fromDate && !validDate(*o.fromDate)) ||
        (o.toDate && !validDate(*o.toDate)) || (o.fromDate && o.toDate && *o.fromDate > *o.toDate))
        throw AppError(ErrorCode::REPORT_OPTIONS_INVALID, {});
    ProjectReport report;
    report.projectName = p.name;
    report.documentId = p.documentId;
    report.revision = p.revision;
    report.currency = p.currency;
    report.options = o;
    report.statusAt = p.statusAt;
    report.scopeName = path(p, o.wbsId);
    const auto rows = wbsTaskRows(p, o.wbsId);
    std::set<std::string> ids;
    std::vector<std::string> idList;
    for (auto row : rows) {
        ids.insert(p.tasks[row].id);
        idList.push_back(p.tasks[row].id);
    }
    const auto comparison = compareToBaseline(p, o.baselineId);
    const auto &active = comparison.forecast;
    report.scheduleStatus = active.status;
    const auto costs = analyzeCosts(p, o.baselineId, o.formula, o.wbsId.empty() ? std::nullopt : std::optional{idList});
    const auto load = resourceLoad(p, active);
    auto scoped = p;
    std::erase_if(scoped.assignments, [&](const auto &a) { return !ids.contains(a.taskId); });
    const auto ownLoad = resourceLoad(scoped, active);
    std::set<std::string> resourceIds, assignedTasks;
    for (const auto &a : scoped.assignments) {
        resourceIds.insert(a.resourceId);
        assignedTasks.insert(a.taskId);
    }
    std::map<std::pair<std::string, std::string>, std::int64_t> ownWork;
    for (const auto &d : ownLoad.days)
        ownWork[{d.resourceId, d.date}] = d.demandUnitMinutes;
    std::unordered_map<std::string, const TaskSchedule *> plans, forecasts;
    for (const auto &s : comparison.plan.tasks)
        if (comparison.plan.status == CalculationStatus::Valid)
            plans[s.taskId] = &s;
    for (const auto &s : active.tasks)
        if (active.status == CalculationStatus::Valid)
            forecasts[s.taskId] = &s;
    std::unordered_map<std::string, const TaskComparison *> comparisons;
    for (const auto &t : comparison.tasks)
        comparisons[t.taskId] = &t;
    const auto rollup = summarizeWbs(p, &active, o.wbsId);
    std::int64_t complete = 0, reported = 0, critical = 0, delayed = 0, comparable = 0, overMinutes = 0,
                 unknownDays = 0, overloadDays = 0;
    for (auto row : rows) {
        const auto &id = p.tasks[row].id;
        const auto *e = progressAsOf(p, id);
        if (e) {
            ++reported;
            if (e->actualFinish)
                ++complete;
        }
        if (plans.contains(id) && plans.at(id)->critical)
            ++critical;
        const auto &c = *comparisons.at(id);
        if (c.forecastFinishVariance) {
            ++comparable;
            if (*c.forecastFinishVariance > 0)
                ++delayed;
        }
    }
    for (const auto &d : load.days)
        if (resourceIds.contains(d.resourceId) && inRange(o, d.date)) {
            overMinutes += d.overloadMinutes;
            if (!d.capacity)
                ++unknownDays;
            if (d.overloadMinutes)
                ++overloadDays;
        }
    auto kpi = [&](const std::string &id, const std::string &title, ReportCell value, const std::string &unit,
                   const std::string &note) { report.kpis.push_back({id, title, unit, note, std::move(value)}); };
    kpi("tasks", "선택 작업", number(std::int64_t(rows.size())), "개", "현재 WBS의 고유 작업 수");
    kpi("completed", "실제 완료", p.statusAt ? number(complete) : ReportCell{}, "개",
        "상태일 기준 완료 보고. 기간/물리 진척 평균이 아닙니다.");
    kpi("finish", p.statusAt ? "잔여 예측 완료" : "계획 완료", stamp(rollup.finish), "",
        "선택 작업 범위. 논리 일정은 자원 타당성을 보장하지 않습니다.");
    kpi("critical", "계획 임계 작업",
        comparison.plan.status == CalculationStatus::Valid ? number(critical) : ReportCell{}, "개",
        "현재 선행관계/달력의 계획 CPM 기준. 잔여 예측/평준화의 임계가 아닙니다.");
    kpi("delayed", "기준선 대비 지연", comparable ? number(delayed) : ReportCell{}, "개",
        "완료 편차를 비교할 수 있는 " + std::to_string(comparable) + "개 작업 중 지연");
    kpi("overload", "과부하 자원시간",
        active.status == CalculationStatus::Valid && !unknownDays ? number(overMinutes, CellFormat::Hours)
                                                                  : ReportCell{},
        "h",
        "일별 명세 범위의 공유 자원 수요. 미정 가용량 " + std::to_string(unknownDays) + "건, 알려진 초과 " +
            std::to_string(overMinutes) + "분");
    kpi("plan", "현재 계획 예산", optional(costs.plan, CellFormat::Money), p.currency, "현재 선택 작업 범위");
    kpi("actual", "누적 실제원가", optional(costs.actual, CellFormat::Money), p.currency,
        "상태일 누적 보고. 단가로 소급 산정하지 않습니다.");
    kpi("bac", "기준선 예산 BAC", optional(costs.evm.bac, CellFormat::Money), p.currency,
        "선택 기준선 코호트. 현재 WBS 선택 시 작업 ID 교집합");
    kpi("ev", "획득가치 EV", optional(costs.evm.ev, CellFormat::Money), p.currency,
        "기준선에 고정된 물리 진척/완료 0/100 측정법");
    kpi("cpi", "원가 효율 CPI", ratio(costs.evm.cpi), "배", "합계 EV / 합계 AC. 0분모/누락은 미정");
    kpi("eac", "완료 예상원가 EAC", optional(costs.evm.eac, CellFormat::Money), p.currency, o.formula);
    auto table = [&](std::string id, std::string title, ReportFamily family, std::string note,
                     std::vector<ReportColumn> columns) -> ReportTable & {
        report.tables.push_back({std::move(id), std::move(title), std::move(note), family, std::move(columns), {}});
        return report.tables.back();
    };
    auto &dashboard = table(
        "dashboard", "대시보드", ReportFamily::All,
        "미정은 확인된 0과 다릅니다. 금액/완료일은 전체 계획 또는 상태일 기준이며 일별 필터는 명세에만 적용합니다.",
        {{"지표", 26}, {"값", 25}, {"단위", 10}, {"근거", 65}});
    for (const auto &k : report.kpis)
        dashboard.rows.push_back({text(k.title), k.value, text(k.unit), text(k.note)});
    dashboard.rows.push_back(
        {text("일정 계산 상태"), text(state(active.status)), text(""), text(active.calculationPolicy)});
    dashboard.rows.push_back({text("일정 실적 보고"), p.statusAt ? number(reported) : ReportCell{}, text("개"),
                              text("선택 " + std::to_string(rows.size()) + "개 작업 중 상태일 기준")});
    dashboard.rows.push_back({text("자원 배정 작업"), number(std::int64_t(assignedTasks.size())), text("개"),
                              text("미배정 작업은 수요 검증에 포함되지 않습니다.")});
    dashboard.rows.push_back({text("과부하 자원·일"),
                              active.status == CalculationStatus::Valid ? number(overloadDays) : ReportCell{},
                              text("건"), text("선택 자원의 전체 프로젝트 공유 수요. 일별 명세 기간 기준")});
    auto &schedule = table("schedule", "계획 일정", ReportFamily::Schedule,
                           "전체 실행 그래프를 계산한 후 WBS 작업을 선택합니다. 총여유는 각 작업 달력의 "
                           "근무시간(h)입니다. 미계산/충돌 날짜를 과거 캐시로 채우지 않습니다.",
                           {{"작업 ID", 28},
                            {"작업", 24},
                            {"WBS", 24},
                            {"기간(분)", 12},
                            {"계획 시작", 21},
                            {"계획 완료", 21},
                            {"여유(h)", 12},
                            {"임계", 10},
                            {"작업 달력", 30}});
    for (auto row : rows) {
        const auto &t = p.tasks[row];
        auto *s = plans.contains(t.id) ? plans.at(t.id) : nullptr;
        schedule.rows.push_back(
            {text(t.id), text(t.name), text(t.wbsId.empty() ? "미분류" : path(p, t.wbsId)),
             optional(isEffort(p) && s ? std::optional<std::int64_t>{TaskExecution(p, t).duration(*s)}
                                       : t.durationMinutes,
                      CellFormat::Integer),
             s ? text(s->start) : ReportCell{}, s ? text(s->finish) : ReportCell{},
             s ? number(s->totalFloatMinutes, CellFormat::Hours) : ReportCell{},
             s ? text(s->critical ? "임계" : "여유") : ReportCell{}, text(taskCalendarName(p, t))});
    }
    if (hasConstraints(p)) {
        auto &timing =
            table("constraints", "작업 제약과 마감", ReportFamily::Schedule,
                  "날짜 제약은 선행/달력과 함께 적용합니다. 마감일은 경고 기준이며 CPM 여유를 바꾸지 않습니다. 마감 "
                  "여유는 해당 작업 달력 근무시간입니다. 비근무 경과는 0근무분이어도 civil 시각으로 초과를 판정합니다.",
                  {{"작업 ID", 28},
                   {"작업", 24},
                   {"제약", 20},
                   {"제약 시각", 21},
                   {"마감일", 21},
                   {"활성 완료", 21},
                   {"마감 여유(h)", 16},
                   {"마감 상태", 14}});
        for (auto row : rows) {
            const auto &t = p.tasks[row];
            const auto *s = forecasts.contains(t.id) ? forecasts.at(t.id) : nullptr;
            timing.rows.push_back({text(t.id), text(t.name),
                                   text(enumToken(t.constraintType) + " · " + constraintName(t.constraintType)),
                                   stamp(t.constraintAt), stamp(t.deadlineAt), s ? text(s->finish) : ReportCell{},
                                   s ? optional(deadlineSlack(p, t, s->finish), CellFormat::Hours) : ReportCell{},
                                   text(!t.deadlineAt               ? "미설정"
                                        : !s                        ? "계산 불가"
                                        : s->finish > *t.deadlineAt ? "마감 초과"
                                                                    : "마감 이내")});
        }
    }
    if (isEffort(p)) {
        auto &effort =
            table("effort", "공수와 작업 유형", ReportFamily::Schedule,
                  "기간은 작업 달력의 활성 근무분, 공수는 자원 달력 교집합 배정량입니다. 잔여 공수는 명시 보고값이며 "
                  "실제 공수를 역산하지 않습니다. 입력 기간은 공수 기준 작업의 일정 계산에 사용하지 않습니다.",
                  {{"작업 ID", 28},
                   {"작업", 24},
                   {"유형", 16},
                   {"계산 기준", 12},
                   {"입력 공수(h)", 16},
                   {"계획 공수(h)", 16},
                   {"계산 기간(h)", 16},
                   {"명시 잔여 공수(h)", 20}});
        for (auto index : rows) {
            const auto &t = p.tasks[index];
            const auto *e = progressAsOf(p, t.id);
            std::optional<std::int64_t> work, duration;
            if (plans.contains(t.id)) {
                TaskExecution model(p, t);
                work = model.work(*plans.at(t.id));
                duration = model.duration(*plans.at(t.id));
            }
            effort.rows.push_back(
                {text(t.id), text(t.name), text(taskTypeName(t.taskType)), text(usesWork(t) ? "공수" : "기간"),
                 optional(t.workUnitMinutes, CellFormat::WorkHours), optional(work, CellFormat::WorkHours),
                 optional(duration, CellFormat::Hours),
                 optional(e ? e->remainingWorkUnitMinutes : std::optional<std::int64_t>{}, CellFormat::WorkHours)});
        }
        auto &breaks = table("taskBreaks", "분할 작업 중단", ReportFamily::Schedule,
                             "절대 시각 [중단 시작, 재개). 작업만 중단하며 다른 작업의 자원은 계속 사용할 수 있습니다. "
                             "일정 이동 후에도 중단 시각은 유지됩니다.",
                             {{"작업 ID", 28}, {"작업", 24}, {"중단 시작", 21}, {"재개 시각", 21}});
        for (auto index : rows)
            for (const auto &b : p.tasks[index].breaks)
                breaks.rows.push_back(
                    {text(p.tasks[index].id), text(p.tasks[index].name), text(b.start), text(b.finish)});
        auto &calendars = table("resourceCalendars", "자원별 근무 달력", ReportFamily::Resources,
                                "빈 자원 달력은 프로젝트 기본 달력입니다. 작업 달력과 교집합을 사용하며 일별 가용량 "
                                "예외는 이 근무시간 안의 최대 배정률입니다.",
                                {{"자원 ID", 28}, {"자원", 24}, {"달력 ID", 28}, {"달력", 36}});
        for (const auto &r : p.resources)
            if (resourceIds.contains(r.id)) {
                Task t;
                t.calendarId = r.calendarId;
                calendars.rows.push_back({text(r.id), text(r.name), text(r.calendarId), text(taskCalendarName(p, t))});
            }
        auto &spans =
            table("executionWork", "실행 배정 구간", ReportFamily::Resources,
                  "활성 일정의 잔여/계획 배정입니다. 중단·비근무 시간 제외, 마지막 1분 공수 배분 포함. 날짜 필터와 "
                  "겹치는 구간만 표시하며 공수는 잘린 구간에 한정합니다.",
                  {{"작업 ID", 28}, {"자원 ID", 28}, {"시작", 21}, {"완료", 21}, {"배정률", 12}, {"공수(h)", 14}});
        WorkCalendar clock(p);
        if (active.status == CalculationStatus::Valid)
            for (auto index : rows) {
                const auto &t = p.tasks[index];
                auto pending = p.statusAt ? pendingTask(p, t) : t;
                for (auto span : executionSpans(p, pending, *forecasts.at(t.id))) {
                    const auto begin = clock.boundary(span.start, false), end = clock.boundary(span.finish, true);
                    if (o.fromDate && end <= *o.fromDate + " 00:00")
                        continue;
                    if (o.toDate && begin > *o.toDate + " 23:59")
                        continue;
                    if (o.fromDate && begin < *o.fromDate + " 00:00")
                        span.start = clock.offset(*o.fromDate + " 00:00");
                    if (o.toDate && end > *o.toDate + " 23:59")
                        span.finish = clock.offset(*o.toDate + " 23:59") + 1;
                    if (span.finish > span.start)
                        spans.rows.push_back(
                            {text(t.id), text(span.resourceId), text(clock.boundary(span.start, false)),
                             text(clock.boundary(span.finish, true)), number(span.units, CellFormat::Units),
                             number((span.finish - span.start) * span.units, CellFormat::WorkHours)});
                }
            }
    }
    auto &forecast =
        table("forecast", "실적과 예측", ReportFamily::Schedule,
              "실제 날짜를 보존합니다. 편차는 선택 기준선 달력의 근무시간이며 기준선 없는 작업은 미정입니다.",
              {{"작업 ID", 28},
               {"작업", 24},
               {"실제 시작", 21},
               {"실제 완료", 21},
               {"잔여(h)", 12},
               {"예측 완료", 21},
               {"완료 편차(h)", 14},
               {"기준선 시작", 21},
               {"기준선 완료", 21}});
    for (auto row : rows) {
        const auto &t = p.tasks[row];
        const auto &c = *comparisons.at(t.id);
        forecast.rows.push_back({text(t.id), text(t.name), stamp(c.actualStart), stamp(c.actualFinish),
                                 optional(c.remainingMinutes, CellFormat::Hours), stamp(c.forecastFinish),
                                 optional(c.forecastFinishVariance, CellFormat::Hours), stamp(c.baselineStart),
                                 stamp(c.baselineFinish)});
    }
    auto &links = table("dependencies", "실행 선행관계", ReportFamily::Schedule,
                        "선택 WBS 밖 관계도 유지합니다. 지연/선행은 후행 작업 달력 근무분입니다.",
                        {{"작업 ID", 28},
                         {"작업", 30},
                         {"선행 ID", 28},
                         {"선행 작업", 30},
                         {"관계", 8},
                         {"지연/선행(m)", 16},
                         {"후행 달력", 30}});
    for (auto row : rows)
        for (const auto &d : dependencies(p.tasks[row])) {
            auto it =
                std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto &t) { return t.id == d.predecessorId; });
            links.rows.push_back({text(p.tasks[row].id), text(p.tasks[row].name), text(d.predecessorId),
                                  text(it == p.tasks.end() ? "누락 참조" : it->name), text(enumToken(d.type)),
                                  number(d.lagMinutes), text(taskCalendarName(p, p.tasks[row]))});
        }
    auto &assignments =
        table("assignments", "자원 배정", ReportFamily::Resources,
              "정률 배정. 작업 유형에 따라 기간·공수·단위를 유지합니다. 100%는 자원 1단위이며 단가/등급은 생산성 "
              "배수가 아닙니다.",
              {{"작업 ID", 28}, {"작업", 25}, {"자원 ID", 28}, {"자원", 25}, {"종류", 12}, {"배정률", 12}});
    std::unordered_map<std::string, const Resource *> resources;
    for (const auto &r : p.resources)
        resources[r.id] = &r;
    std::unordered_map<std::string, std::string> names;
    for (const auto &t : p.tasks)
        names[t.id] = t.name;
    for (const auto &a : scoped.assignments)
        assignments.rows.push_back(
            {text(a.taskId), text(names.at(a.taskId)), text(a.resourceId), text(resources.at(a.resourceId)->name),
             text(resources.at(a.resourceId)->type == "labor" ? "인력" : "장비"), number(a.units, CellFormat::Units)});
    auto &dailyLoad = table("resourceLoad", "기간별 자원 수요", ReportFamily::Resources,
                            "선택 자원에 대한 전체 프로젝트 공유 수요입니다. 선택 공수와 전체 공수를 구분하고 수요가 "
                            "있는 날짜만 표시합니다. 상태일 설정 시 잔여 수요만 산정합니다.",
                            {{"자원 ID", 28},
                             {"자원", 23},
                             {"날짜", 15},
                             {"선택 공수(h)", 14},
                             {"전체 공수(h)", 14},
                             {"가용률", 12},
                             {"전체 최대배정", 16}});
    for (const auto &d : load.days)
        if (resourceIds.contains(d.resourceId) && inRange(o, d.date))
            dailyLoad.rows.push_back({text(d.resourceId), text(resources.at(d.resourceId)->name), text(d.date),
                                      number(ownWork[{d.resourceId, d.date}], CellFormat::WorkHours),
                                      number(d.demandUnitMinutes, CellFormat::WorkHours),
                                      d.capacity ? number(*d.capacity, CellFormat::Units) : ReportCell{},
                                      number(d.peakUnits, CellFormat::Units)});
    auto &exceptions = table("resourceOverload", "자원 과부하와 미정", ReportFamily::Resources,
                             "동시간대 최대 수요로 판단합니다. 과부하 시간은 자원별 구간 합계이며 프로젝트 경과시간과 "
                             "다릅니다. 미정 가용량을 0%로 취급하지 않습니다.",
                             {{"자원 ID", 28},
                              {"자원", 23},
                              {"날짜", 15},
                              {"최대 사용률", 14},
                              {"과부하(h)", 14},
                              {"초과 공수(h)", 14},
                              {"상태", 16}});
    for (const auto &d : load.days)
        if (resourceIds.contains(d.resourceId) && inRange(o, d.date) && (!d.capacity || d.overloadMinutes))
            exceptions.rows.push_back(
                {text(d.resourceId), text(resources.at(d.resourceId)->name), text(d.date),
                 d.capacity && *d.capacity > 0 ? ReportCell{double(d.peakUnits) / *d.capacity, CellFormat::Units}
                                               : ReportCell{},
                 d.capacity ? number(d.overloadMinutes, CellFormat::Hours) : ReportCell{},
                 d.capacity ? number(d.excessUnitMinutes, CellFormat::WorkHours) : ReportCell{},
                 text(d.capacity ? (*d.capacity == 0 ? "가용 0% 초과" : "과부하") : "가용량 미정")});
    auto &costRows = table(
        "costs", "작업별 원가", ReportFamily::Costs,
        "금액 단위 " + p.currency +
            ". 현재 예산/AC는 현재 작업, BAC/PV/EV는 선택 기준선 코호트입니다. 단가로 실제원가를 재평가하지 않습니다.",
        {{"작업 ID", 28}, {"작업", 25}, {"현재 예산", 17}, {"BAC", 17}, {"PV", 17}, {"EV", 17}, {"AC", 17}});
    for (const auto &r : costs.tasks)
        costRows.rows.push_back({text(r.taskId), text(r.name), optional(r.plan, CellFormat::Money),
                                 optional(r.evm.bac, CellFormat::Money), optional(r.evm.pv, CellFormat::Money),
                                 optional(r.evm.ev, CellFormat::Money), optional(r.evm.ac, CellFormat::Money)});
    auto &evmRows = table(
        "evm", "획득가치 분석", ReportFamily::Costs,
        "CV=EV-AC, SV=EV-PV는 금액 편차이며 날짜 지연이 아닙니다. CPI/SPI는 합계의 비율, EAC 가정은 " + o.formula +
            "입니다.",
        {{"작업 ID", 28}, {"작업", 25}, {"CV", 17}, {"SV", 17}, {"CPI", 12}, {"SPI", 12}, {"EAC", 17}, {"ETC", 17}});
    for (const auto &r : costs.tasks)
        evmRows.rows.push_back({text(r.taskId), text(r.name), optional(r.evm.cv, CellFormat::Money),
                                optional(r.evm.sv, CellFormat::Money), ratio(r.evm.cpi), ratio(r.evm.spi),
                                optional(r.evm.eac, CellFormat::Money), optional(r.evm.etc, CellFormat::Money)});
    auto &groups = table(
        "costGroups", "원가 집계", ReportFamily::Costs,
        "현재/기준선 WBS를 구분합니다. 상위/하위 행을 다시 더하지 마세요. 현재 선택 작업 ID만 각 구조에 집계합니다.",
        {{"범위", 32}, {"현재 예산", 17}, {"현재 실제", 17}, {"BAC", 17}, {"EV", 17}, {"AC", 17}, {"CPI", 12}});
    groups.rows.push_back({text("현재 선택 범위"), optional(costs.plan, CellFormat::Money),
                           optional(costs.actual, CellFormat::Money), text("해당없음"), text("해당없음"),
                           text("해당없음"), text("해당없음")});
    groups.rows.push_back({text("기준선 선택 코호트"), text("해당없음"), text("해당없음"),
                           optional(costs.evm.bac, CellFormat::Money), optional(costs.evm.ev, CellFormat::Money),
                           optional(costs.evm.ac, CellFormat::Money), ratio(costs.evm.cpi)});
    for (const auto &g : costs.currentWbs) {
        auto members = wbsTaskRows(p, g.id);
        if (std::none_of(members.begin(), members.end(), [&](auto row) { return ids.contains(p.tasks[row].id); }))
            continue;
        groups.rows.push_back({text("현재 WBS " + path(p, g.id)), optional(g.plan, CellFormat::Money),
                               optional(g.actual, CellFormat::Money), text("해당없음"), text("해당없음"),
                               text("해당없음"), text("해당없음")});
    }
    const Baseline *baseline = nullptr;
    for (const auto &b : p.baselines)
        if (b.id == o.baselineId)
            baseline = &b;
    for (const auto &g : costs.baselineWbs) {
        if (!baseline)
            break;
        auto members = wbsTaskRows(baseline->input, g.id);
        if (!o.wbsId.empty() && std::none_of(members.begin(), members.end(),
                                             [&](auto row) { return ids.contains(baseline->input.tasks[row].id); }))
            continue;
        groups.rows.push_back({text("기준선 WBS " + path(baseline->input, g.id)), text("해당없음"), text("해당없음"),
                               optional(g.evm.bac, CellFormat::Money), optional(g.evm.ev, CellFormat::Money),
                               optional(g.evm.ac, CellFormat::Money), ratio(g.evm.cpi)});
    }
    auto &days = table("costDays", "기간별 원가", ReportFamily::Costs,
                       "일별 계획/예산과 누적 PV/EV/AC를 구분합니다. 현재 실제와 기준선 코호트 AC는 범위가 다를 수 "
                       "있습니다. 상태일 이후 EV/AC를 예측하지 않습니다.",
                       {{"날짜", 16},
                        {"현재 계획 일별", 19},
                        {"기준선 예산 일별", 20},
                        {"PV 누적", 18},
                        {"EV 누적", 18},
                        {"기준선 AC 누적", 19},
                        {"현재 실제 누적", 19}});
    for (const auto &d : costs.days)
        if (inRange(o, d.date)) {
            days.rows.push_back({text(d.date), optional(d.plan, CellFormat::Money),
                                 optional(d.budget, CellFormat::Money), optional(d.pv, CellFormat::Money),
                                 optional(d.ev, CellFormat::Money), optional(d.ac, CellFormat::Money),
                                 optional(d.currentActual, CellFormat::Money)});
            report.costTrend.push_back(d);
        }
    auto &registry = table("resourceRegistry", "자원 등록과 가용량", ReportFamily::Resources,
                           "전체 프로젝트에서는 미배정 자원도 표시합니다. 날짜별 가용량은 해당 날짜에만 기본 가용량을 "
                           "대체합니다. 미정과 0%를 구분합니다.",
                           {{"자원 ID", 28}, {"자원", 25}, {"종류", 14}, {"적용 날짜", 18}, {"가용률", 14}});
    for (const auto &r : p.resources)
        if (o.wbsId.empty() || resourceIds.contains(r.id)) {
            registry.rows.push_back({text(r.id), text(r.name), text(r.type == "labor" ? "인력" : "장비"), text("기본"),
                                     r.maxUnits ? number(*r.maxUnits, CellFormat::Units) : ReportCell{}});
            for (const auto &d : r.exceptions)
                registry.rows.push_back({text(r.id), text(r.name), text(r.type == "labor" ? "인력" : "장비"),
                                         text(d.date), d.units ? number(*d.units, CellFormat::Units) : ReportCell{}});
        }
    auto &diagnostics = table(
        "diagnostics", "진단", ReportFamily::All,
        "입력 불완전/충돌/미정은 원자료와 함께 보존합니다. 날짜/금액이 없다는 사실을 정상 0으로 해석하지 않습니다.",
        {{"분야", 16}, {"코드", 27}, {"설명", 76}, {"관련 작업 ID", 30}});
    auto append = [&](const std::string &source, const std::vector<Diagnostic> &ds) {
        for (const auto &d : ds) {
            std::string refs;
            for (const auto &id : d.taskIds) {
                if (!refs.empty())
                    refs += ", ";
                refs += id;
            }
            diagnostics.rows.push_back({text(source), text(d.code), text(diagnosticMessage(d)), text(refs)});
        }
    };
    append("계획 일정", comparison.plan.diagnostics);
    append("잔여 예측", active.diagnostics);
    append("공유 자원", load.diagnostics);
    append("원가/EVM", costs.diagnostics);
    auto &source = table("source", "출처와 설정", ReportFamily::All,
                         "재계산된 메모리 스냅샷의 정적 보고서입니다. 엑셀 셀 편집이 JOBS 원본을 변경하거나 앱 계산을 "
                         "재실행하지 않습니다.",
                         {{"항목", 32}, {"값", 100}});
    auto meta = [&](const std::string &key, const std::string &value) {
        source.rows.push_back({text(key), text(value)});
    };
    meta("프로젝트", p.name);
    meta("문서 ID", p.documentId);
    meta("입력 revision", std::to_string(p.revision));
    meta("원본 상태", o.unsaved ? "미저장 메모리 revision" : "저장된 revision");
    meta("보고서 ID", o.reportId);
    meta("생성 UTC", o.generatedAt);
    meta("상태일(civil)", p.statusAt.value_or("미설정"));
    meta("현재 WBS 범위", report.scopeName);
    meta("WBS ID", o.wbsId.empty() ? "프로젝트 루트" : o.wbsId);
    meta("일별 명세 시작", o.fromDate.value_or("제한 없음"));
    meta("일별 명세 끝", o.toDate.value_or("제한 없음"));
    meta("일별 필터 의미", "일별 자원/원가 명세·추이에만 적용. 누적 금액·완료일·작업 수는 전체 계획/상태일 기준");
    meta("기준선 ID", baseline ? baseline->id : "없음");
    if (baseline) {
        meta("기준선", baseline->name);
        meta("기준선 버전", std::to_string(baseline->version));
        meta("승인자", baseline->approvedBy);
        meta("승인 사유", baseline->reason);
        meta("기준선 생성 UTC", baseline->createdAt);
        meta("기준선 원본 ID", baseline->sourceDocumentId);
        meta("기준선 입력 revision", std::to_string(baseline->inputRevision));
    }
    meta("계산 엔진", active.engineVersion);
    meta("계획 계산 정책", comparison.plan.calculationPolicy);
    meta("활성 일정 정책", active.calculationPolicy);
    meta("계산 profile", p.calculationProfile);
    meta("원가 정책", p.costProfile);
    meta("EAC 가정", o.formula);
    meta("통화", p.currency);
    meta("기본 달력", p.calendar + " / 월-금 09-12, 13-18 / 1일 480 근무분");
    std::string holidays;
    for (const auto &h : p.holidays) {
        if (!holidays.empty())
            holidays += ", ";
        holidays += h;
    }
    meta("기본 달력 휴일", holidays.empty() ? "없음" : holidays);
    for (const auto &c : p.calendars) {
        std::string detail = c.name + " [" + c.id + "] 요일:";
        for (int day : c.weekdays)
            detail += " " + std::to_string(day);
        detail += " 시간대(자정 후 분):";
        for (const auto &s : c.periods)
            detail += " " + std::to_string(s.begin) + "-" + std::to_string(s.end);
        detail += " 휴일:";
        for (const auto &d : c.holidays)
            detail += " " + d;
        meta("작업 달력 정의", detail);
    }
    for (auto i : rows)
        meta("작업 달력 배정", p.tasks[i].id + " / " + p.tasks[i].name + " / " + taskCalendarName(p, p.tasks[i]));
    meta("자원 범위", "선택 작업 배정 자원. 다른 WBS 작업의 공유 수요를 포함해 가용량과 비교");
    meta("원가 범위",
         "현재 WBS의 작업 ID. EVM은 선택 기준선과의 교집합. 전체 프로젝트 선택 시 삭제된 기준선 작업도 포함");
    meta("미정 표시", "미정은 누락/분모 0/비교 불가를 뜻하며 숫자 0과 구분. 해당없음은 별도 표시");
    meta("보고서 정책", "jobs-report-snapshot-v1");
    std::size_t cells = 0;
    for (const auto &t : report.tables) {
        for (const auto &r : t.rows)
            if (r.size() != t.columns.size())
                throw AppError(ErrorCode::REPORT_COLUMN_MISMATCH, {});
        cells += t.rows.size() * t.columns.size();
    }
    if (cells > 1000000)
        throw AppError(ErrorCode::REPORT_CELL_LIMIT, {});
    return report;
}
} // namespace jobs
