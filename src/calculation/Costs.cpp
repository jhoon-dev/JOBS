#include "calculation/Costs.h"
#include "calculation/WorkCalendar.h"
#include "calculation/Effort.h"
#include "calculation/Tracking.h"
#include "domain/Wbs.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <unordered_map>
#include <stdexcept>
#include <limits>
#include <memory>
namespace jobs {
namespace {
using Minute = std::int64_t;
Money add(Money a, Money b) { if (!a || !b || *a > maxMoney - *b) return {}; return *a + *b; }
Money subtract(Money a, Money b) { if (!a || !b) return {}; return *a - *b; }
// Exact nonnegative integer half-up rounding, without nonportable 128-bit arithmetic.
Money fraction(std::int64_t a, std::int64_t b, std::int64_t denominator) {
    const auto q = a / denominator, r = a % denominator;
    if (b && (q > maxMoney / b || r > (std::numeric_limits<std::int64_t>::max() - denominator / 2) / b)) return {};
    return add(q * b, (r * b + denominator / 2) / denominator);
}
Money estimate(long double n) { if (!std::isfinite(n) || n < 0 || n > maxMoney) return {}; return std::int64_t(std::floor(n + 0.5L)); }
void derive(Evm& e, const std::string& formula) {
    e.cv = subtract(e.ev, e.ac); e.sv = subtract(e.ev, e.pv);
    if (e.ev && e.ac && *e.ac > 0) e.cpi = double(*e.ev) / double(*e.ac);
    if (e.ev && e.pv && *e.pv > 0) e.spi = double(*e.ev) / double(*e.pv);
    if (!e.bac || !e.ev || !e.ac) return;
    auto remaining = *e.bac - *e.ev;
    if (formula == "budget-remaining-v1") e.etc = remaining;
    else if (e.cpi && *e.cpi > 0 && (formula != "cpi-spi-v1" || (e.spi && *e.spi > 0))) {
        auto value = static_cast<long double>(remaining) * *e.ac / *e.ev;
        if (formula == "cpi-spi-v1") value = value * *e.pv / *e.ev;
        e.etc = estimate(value);
    }
    e.eac = add(e.ac, e.etc);
}
struct Segment { Minute start, finish; int units; Money rate; Minute dayWorkBefore = 0; };
struct Bucket { Minute day; std::vector<Segment> spans; Money before = 0, total = 0; };
struct Budget {
    std::string startStamp, finishStamp, accrual = "uniform", credit = "physical";
    Minute start = 0, finish = 0;
    Money fixed;
    std::vector<Bucket> days;
    std::vector<std::string> issues;
    bool valid = false;
    std::shared_ptr<WorkCalendar> work;
    Minute elapsed(const std::string& stamp, const WorkCalendar& clock) const {
        if (stamp <= startStamp) return 0;
        const auto& cal=work?*work:clock;
        return cal.offset(std::min(stamp,finishStamp))-cal.offset(startStamp);
    }
    Money on(const std::string& date, const WorkCalendar& calendar) const {
        if (!valid) return {};
        if (date < startStamp.substr(0, 10) || date > finishStamp.substr(0, 10)) return 0;
        Money resource = 0;
        const auto offset = calendar.offset(date + " 09:00");
        auto it = std::lower_bound(days.begin(), days.end(), offset / calendar.dayMinutes(), [](const auto& d, Minute day) { return d.day < day; });
        if (it != days.end() && it->day == offset / calendar.dayMinutes() && calendar.boundary(it->day * calendar.dayMinutes(), false).substr(0, 10) == date) resource = it->total;
        Money f = 0;
        if (accrual == "uniform" && finish > start) {
            const auto before = elapsed(date + " 00:00",calendar);
            const auto after = elapsed(work?calendar.boundary((offset/calendar.dayMinutes()+1)*calendar.dayMinutes(),true):date + " 23:59",calendar);
            const auto total=elapsed(finishStamp,calendar);
            if (after > before && total>0) f = fixed ? subtract(fraction(*fixed, after, total), fraction(*fixed, before, total)) : Money{};
        } else if (date == (accrual == "start" ? startStamp : finishStamp).substr(0, 10)) f = fixed;
        return add(resource, f);
    }
    Money at(const std::string& stamp, const WorkCalendar& calendar) const {
        if (!valid) return {};
        if (stamp < startStamp) return 0;
        auto offset = stamp >= finishStamp ? finish : calendar.offset(stamp);
        auto elapsedWork = elapsed(stamp,calendar);
        Money resource = 0;
        if (!days.empty() && elapsedWork > 0) {
            auto it = std::lower_bound(days.begin(), days.end(), offset / calendar.dayMinutes(), [](const auto& d, Minute day) { return d.day < day; });
            if (it == days.end()) resource = add(days.back().before, days.back().total);
            else {
                resource = it->before;
                for (const auto& s : it->spans) {
                    auto n = std::clamp(offset - s.start, Minute{0}, s.finish - s.start);
                    if (n > 0) resource = add(resource, s.rate ? subtract(fraction(*s.rate, s.dayWorkBefore+n*s.units, 6000),fraction(*s.rate,s.dayWorkBefore,6000)) : Money{});
                }
            }
        }
        Money f = 0;
        const bool earned = accrual == "start" ? stamp >= startStamp : stamp >= finishStamp;
        if (accrual == "uniform" && finish > start) {
            if (elapsedWork > 0) f = fixed ? fraction(*fixed, elapsedWork, elapsed(finishStamp,calendar)) : Money{};
        } else if (earned) f = fixed;
        return add(resource, f);
    }
};
const TaskCost* config(const PlanInput& p, const std::string& id) { for (const auto& c : p.taskCosts) if (c.taskId == id) return &c; return nullptr; }
using CalendarCache=std::map<std::string,std::shared_ptr<WorkCalendar>>;
Budget budget(const PlanInput& p, const Task& task, const TaskSchedule* row, const WorkCalendar& calendar, CalendarCache& cache) {
    Budget b;
    if (const auto* c = config(p, task.id)) { b.fixed = c->fixedMinor; b.accrual = c->accrual; b.credit = c->credit; }
    if (!b.fixed) b.issues.push_back("고정비 미정: 비용 없음은 0으로 입력하세요.");
    if (!row) { b.issues.push_back("유효한 논리 일정이 없어 비용 발생일을 산정할 수 없습니다."); return b; }
    b.valid = true; b.start = row->earlyStartMinutes; b.finish = row->earlyFinishMinutes; b.startStamp = row->start; b.finishStamp = row->finish;
    if (isPdm(p)) { auto& shared=cache[isEffort(p)?task.id:task.calendarId]; if (!shared) shared=std::make_shared<WorkCalendar>(p,task); b.work=shared; }
    std::vector<std::pair<Minute,Minute>> workSpans;
    if (b.work) { for (const auto& [begin,end]:b.work->segments(row->start,row->finish)) workSpans.emplace_back(calendar.offset(begin),calendar.offset(end)); }
    else workSpans.emplace_back(b.start,b.finish);
    std::map<Minute, Bucket> days;
    const auto effortSpans=isEffort(p)?executionSpans(p,task,*row):std::vector<WorkSpan>{};
    for (const auto& a : p.assignments) if (a.taskId == task.id) {
        const auto r = std::find_if(p.resources.begin(), p.resources.end(), [&](const auto& x) { return x.id == a.resourceId; });
        bool warned = false;
        std::map<Minute,Minute> workInDay;
        std::vector<WorkSpan> assigned;
        if(isEffort(p)){for(const auto& s:effortSpans)if(s.resourceId==a.resourceId)assigned.push_back(s);}
        else for(const auto& [begin,finish]:workSpans)assigned.push_back({a.resourceId,begin,finish,a.units});
        for (const auto& s:assigned) for (auto at = s.start; at < s.finish;) {
            const auto finish=s.finish;
            const auto end = std::min(finish, (at / calendar.dayMinutes() + 1) * calendar.dayMinutes()); auto date = calendar.boundary(at, false).substr(0, 10);
            const Resource::Rate* rate = nullptr;
            for (const auto& v : r->rates) if (v.effectiveDate <= date && (!rate || v.effectiveDate > rate->effectiveDate)) rate = &v;
            Money amount = rate ? rate->hourlyMinor : Money{};
            if (!amount && !warned) { b.issues.push_back("단가 미정: " + r->name + " [" + r->id + "] " + date); warned = true; }
            auto& d = days[at / calendar.dayMinutes()]; d.day = at / calendar.dayMinutes(); auto& before=workInDay[d.day];
            d.spans.push_back({at, end, s.units, amount,before});
            d.total = add(d.total, amount ? subtract(fraction(*amount,before+(end-at)*s.units,6000),fraction(*amount,before,6000)) : Money{}); before+=(end-at)*s.units; at = end;
        }
    }
    Money prefix = 0;
    for (auto& [day, d] : days) { (void)day; d.before = prefix; prefix = add(prefix, d.total); b.days.push_back(std::move(d)); }
    if (!b.at(b.finishStamp, calendar) && b.issues.empty()) b.issues.push_back("비용 계산/합계가 금액 한도(9조 최소 통화단위)를 초과했습니다.");
    return b;
}
const ProgressEntry* progressAt(const Project& p, const std::string& id, const std::string& stamp) {
    const ProgressEntry* result = nullptr;
    for (const auto& e : p.progress) if (e.taskId == id && e.statusAt <= stamp && (!result || e.statusAt >= result->statusAt)) result = &e;
    return result;
}
Money actual(const Project& p, const std::string& id, const std::string& stamp) {
    auto* e = costAsOf(p, id, stamp); return e ? e->actualMinor : Money{};
}
Money earned(const Project& p, const std::string& id, const Budget& b, Money bac, const std::string& stamp) {
    if (!bac) return {};
    if (*bac == 0) return 0;
    if (b.credit == "zero-hundred") { const auto* e = progressAt(p, id, stamp); return e ? Money{e->actualFinish ? *bac : 0} : Money{}; }
    const auto* e = costAsOf(p, id, stamp); return e && e->physicalBasisPoints ? fraction(*bac, *e->physicalBasisPoints, 10000) : Money{};
}
Evm combine(const std::vector<CostTaskRow>& rows, const std::set<std::string>& ids, const std::string& formula, bool hasStatus) {
    Evm e; e.bac = e.pv = e.ev = e.ac = 0;
    for (const auto& row : rows) if (row.baselineMember && ids.contains(row.taskId)) {
        e.bac = add(e.bac, row.evm.bac); e.pv = add(e.pv, row.evm.pv); e.ev = add(e.ev, row.evm.ev); e.ac = add(e.ac, row.evm.ac);
    }
    if (!hasStatus) { e.pv.reset(); e.ev.reset(); e.ac.reset(); }
    derive(e, formula); return e;
}
}
CostAnalysis analyzeCosts(const Project& p, const std::string& baselineId, const std::string& formula,
                          const std::optional<std::vector<std::string>>& taskScope) {
    validate(p);
    if (formula != "budget-remaining-v1" && formula != "cpi-v1" && formula != "cpi-spi-v1") throw std::runtime_error("지원하지 않는 EAC 산식입니다.");
    CostAnalysis out; out.documentId = p.documentId; out.revision = p.revision; out.statusAt = p.statusAt; out.currency = p.currency; out.baselineId = baselineId; out.formula = formula;
    const Baseline* baseline = nullptr;
    for (const auto& b : p.baselines) if (b.id == baselineId) baseline = &b;
    if (!baselineId.empty() && !baseline) throw std::runtime_error("비교할 원가 기준선이 없습니다.");
    if (baseline) verifyBaseline(*baseline);
    std::set<std::string> selected;
    if (taskScope) {
        std::set<std::string> known; for (const auto& t : p.tasks) known.insert(t.id);
        if (baseline) for (const auto& t : baseline->input.tasks) known.insert(t.id);
        for (const auto& id : *taskScope) { if (!known.contains(id)) throw std::runtime_error("원가 집계 범위의 작업이 존재하지 않습니다."); selected.insert(id); }
    }
    const bool compatible = baseline && baseline->input.currency == p.currency;
    if (baseline && !compatible) out.diagnostics.push_back({"CURRENCY_MISMATCH", "기준선과 현재 통화가 다릅니다. 환산 없이 EVM·혼합 합계를 산정하지 않습니다.", {}});
    if (!baseline) out.diagnostics.push_back({"NO_COST_BASELINE", "기준선을 선택하면 BAC/PV/EV를 계산합니다. 현재 계획 예산은 기준선 예산과 구분합니다.", {}});
    if (!p.statusAt) out.diagnostics.push_back({"NO_COST_STATUS", "상태일을 설정하면 누적 실제원가·성과를 조회합니다.", {}});
    auto plan = calculatePlan(p); WorkCalendar calendar(p);
    std::optional<WorkCalendar> baselineCalendar; if (baseline) baselineCalendar.emplace(baseline->input);
    std::unordered_map<std::string, const TaskSchedule*> dates, frozenDates;
    for (const auto& row : plan.tasks) if (plan.status == CalculationStatus::Valid) dates.emplace(row.taskId, &row);
    if (baseline) for (const auto& row : baseline->schedule) frozenDates.emplace(row.taskId, &row);
    std::map<std::string, Budget> current, frozen;
    CalendarCache currentCalendars,frozenCalendars;
    for (const auto& t : p.tasks) current.emplace(t.id, budget(p, t, dates.contains(t.id) ? dates.at(t.id) : nullptr, calendar,currentCalendars));
    if (compatible) for (const auto& t : baseline->input.tasks) frozen.emplace(t.id, budget(baseline->input, t, frozenDates.at(t.id), *baselineCalendar,frozenCalendars));
    if (taskScope) { std::erase_if(current, [&](const auto& item) { return !selected.contains(item.first); }); std::erase_if(frozen, [&](const auto& item) { return !selected.contains(item.first); }); }
    std::vector<Task> all; for (const auto& t : p.tasks) if (current.contains(t.id)) all.push_back(t);
    if (baseline) for (const auto& t : baseline->input.tasks) if ((!taskScope || selected.contains(t.id)) && !current.contains(t.id)) all.push_back(t);
    out.plan = 0; out.actual = p.statusAt ? Money{0} : Money{}; std::set<std::string> cohort;
    for (const auto& t : all) {
        CostTaskRow row; row.taskId = t.id; row.name = t.name; row.baselineMember = baseline && frozenDates.contains(t.id);
        if (current.contains(t.id)) {
            const auto& b = current.at(t.id); row.plan = b.at(b.finishStamp, calendar); row.issues = b.issues; out.plan = add(out.plan, row.plan);
            if (p.statusAt) out.actual = add(out.actual, actual(p, t.id, *p.statusAt));
        }
        if (p.statusAt) { row.evm.ac = actual(p, t.id, *p.statusAt); if (const auto* e = costAsOf(p, t.id, *p.statusAt)) row.physicalBasisPoints = e->physicalBasisPoints; }
        if (row.evm.ac && current.contains(t.id)) ++out.actualCoverage;
        if (row.baselineMember) { cohort.insert(t.id); ++out.baselineTasks; }
        if (frozen.contains(t.id)) {
            const auto& b = frozen.at(t.id); row.credit = b.credit; row.evm.bac = b.at(b.finishStamp, *baselineCalendar);
            for (const auto& issue : b.issues) row.issues.push_back("기준선: " + issue);
            if (p.statusAt) { row.evm.pv = b.at(*p.statusAt, *baselineCalendar); row.evm.ev = earned(p, t.id, b, row.evm.bac, *p.statusAt); }
        }
        if (row.evm.ev && row.baselineMember) ++out.evCoverage;
        if (p.statusAt && !row.evm.ac) row.issues.push_back("실제원가 미보고/미정");
        if (p.statusAt && row.baselineMember && !row.evm.ev) row.issues.push_back("EV 예산 또는 성과 측정 근거 미정");
        derive(row.evm, formula); out.tasks.push_back(std::move(row));
    }
    if (compatible) out.evm = combine(out.tasks, cohort, formula, p.statusAt.has_value());
    for (const auto& n : p.wbsNodes) {
        CostGroup group; group.id = n.id; group.name = n.name; group.plan = 0; group.actual = p.statusAt ? Money{0} : Money{};
        for (auto index : wbsTaskRows(p, n.id)) {
            const auto& id = p.tasks[index].id; if (!current.contains(id)) continue;
            group.plan = add(group.plan, current.at(id).at(current.at(id).finishStamp, calendar));
            if (p.statusAt) group.actual = add(group.actual, actual(p, id, *p.statusAt));
        }
        out.currentWbs.push_back(std::move(group));
    }
    if (compatible) for (const auto& n : baseline->input.wbsNodes) {
        std::set<std::string> ids; for (auto index : wbsTaskRows(baseline->input, n.id)) ids.insert(baseline->input.tasks[index].id);
        CostGroup group; group.id = n.id; group.name = n.name; group.evm = combine(out.tasks, ids, formula, p.statusAt.has_value()); out.baselineWbs.push_back(std::move(group));
    }
    std::set<std::string> days;
    auto addDays = [&](const auto& budgets, const WorkCalendar& cal) {
        for (const auto& [id, b] : budgets) { (void)id; if (!b.valid) continue;
            days.insert(b.startStamp.substr(0, 10)); days.insert(b.finishStamp.substr(0, 10));
            for (auto m = b.start / cal.dayMinutes() * cal.dayMinutes(); m < b.finish; m += cal.dayMinutes()) days.insert(cal.boundary(m, false).substr(0, 10));
        }
    };
    addDays(current, calendar); if (compatible) addDays(frozen, *baselineCalendar);
    if (p.statusAt) {
        days.insert(p.statusAt->substr(0, 10));
        for (const auto& e : p.costEntries) if (e.statusAt <= *p.statusAt && (!taskScope || selected.contains(e.taskId))) days.insert(e.statusAt.substr(0, 10));
        for (const auto& e : p.progress) if (e.statusAt <= *p.statusAt && (!taskScope || selected.contains(e.taskId))) days.insert(e.statusAt.substr(0, 10));
    }
    for (const auto& date : days) {
        const auto end = date + " 23:59"; CostDay d; d.date = date; d.plan = 0; d.budget = d.pv = compatible ? Money{0} : Money{};
        for (const auto& [id, b] : current) { (void)id; d.plan = add(d.plan, b.on(date, calendar)); }
        for (const auto& [id, b] : frozen) { (void)id; d.budget = add(d.budget, b.on(date, *baselineCalendar)); d.pv = add(d.pv, b.at(end, *baselineCalendar)); }
        if (p.statusAt && date <= p.statusAt->substr(0, 10)) {
            d.currentActual = 0; const auto at = std::min(end, *p.statusAt);
            for (const auto& [id, b] : current) { (void)b; d.currentActual = add(d.currentActual, actual(p, id, at)); }
        }
        if (compatible && p.statusAt && date <= p.statusAt->substr(0, 10)) {
            const auto at = std::min(end, *p.statusAt); d.ev = d.ac = 0;
            for (const auto& [id, b] : frozen) { d.ev = add(d.ev, earned(p, id, b, b.at(b.finishStamp, *baselineCalendar), at)); d.ac = add(d.ac, actual(p, id, at)); }
            if (date == p.statusAt->substr(0, 10)) d.pv = out.evm.pv;
        }
        out.days.push_back(std::move(d));
    }
    for (const auto& row : out.tasks) for (const auto& issue : row.issues) out.diagnostics.push_back({"COST_INPUT_OR_LIMIT", issue, {row.taskId}});
    std::set<std::string> currentIds; for (const auto& t : p.tasks) if (current.contains(t.id)) currentIds.insert(t.id);
    if (baseline && cohort != currentIds) out.diagnostics.push_back({"BASELINE_SCOPE", "EVM 총계는 선택 기준선의 작업 ID 범위입니다. 기준선 밖 작업 원가는 현재 계획/실제 총계에만 포함합니다.", {}});
    out.diagnostics.push_back({"EVM_POLICY", "금액 반올림: 자원·배정·근무일별 최소 통화단위 half-up. EVM은 기준선 측정법과 상태일 보고를 사용하며 기간 진척을 물리 진척으로 대체하지 않습니다. CPI/SPI는 합계 금액의 비율입니다.", {}});
    return out;
}
}
