#include "domain/Project.h"
#include "domain/CalculationControl.h"
#include <chrono>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace jobs {
bool isEffort(const PlanInput& p) { return p.calculationProfile==effortProfile; }
bool hasConstraints(const PlanInput& p) { return p.calculationProfile==constraintProfile || isEffort(p); }
bool isPdm(const PlanInput& p) { return p.calculationProfile==pdmProfile || hasConstraints(p); }
bool usesWork(const Task& t) { return t.taskType=="fixed-work" || t.workDriven; }
std::string taskTypeName(const std::string& t) { return t=="fixed-work"?"고정 공수":t=="fixed-units"?"고정 단위":"고정 기간"; }
std::string engineFor(const PlanInput& p) { return isEffort(p)?"jobs-effort-1.0":hasConstraints(p)?"jobs-pdm-1.1":isPdm(p)?"jobs-pdm-1.0":"jobs-cpm-0.2"; }
std::string levelingPolicyFor(const PlanInput& p) { return isEffort(p)?"serial-effort-split-v1":hasConstraints(p)?"serial-no-split-constrained-v1":isPdm(p)?"serial-no-split-pdm-v1":"serial-no-split-v1"; }
std::string constraintName(const std::string& type) {
    if(type=="ASAP")return "가능한 빨리";
    if(type=="ALAP")return "가능한 늦게";
    if(type=="SNET")return "시작 이후";
    if(type=="SNLT")return "시작 이전";
    if(type=="FNET")return "완료 이후";
    if(type=="FNLT")return "완료 이전";
    if(type=="MSO")return "시작 고정";
    if(type=="MFO")return "완료 고정";
    return "알 수 없는 제약";
}
std::vector<Dependency> dependencies(const Task& task) {
    std::vector<Dependency> out;
    for (const auto& id : task.predecessors) {
        auto it = std::find_if(task.dependencyRules.begin(), task.dependencyRules.end(), [&](const auto& d) { return d.predecessorId == id; });
        out.push_back(it == task.dependencyRules.end() ? Dependency{id, "FS", 0} : *it);
    }
    return out;
}
std::vector<std::string> dependencyLabels(const Task& task) {
    std::vector<std::string> out;
    for (const auto& d : dependencies(task)) out.push_back(d.predecessorId + " " + d.type + (d.lagMinutes >= 0 ? "+" : "") + std::to_string(d.lagMinutes) + "m");
    return out;
}
std::string taskCalendarName(const PlanInput& p, const Task& task) {
    for (const auto& c : p.calendars) if (c.id == task.calendarId) return c.name;
    return "프로젝트 기본 (월–금 09–12 / 13–18)";
}
bool validDate(const std::string& s) {
    if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
    for (std::size_t i = 0; i < s.size(); ++i)
        if (i != 4 && i != 7 && (s[i] < '0' || s[i] > '9')) return false;
    const int y = std::stoi(s.substr(0, 4));
    return y >= 1 && std::chrono::year_month_day{
        std::chrono::year{y}, std::chrono::month{unsigned(std::stoi(s.substr(5, 2)))},
        std::chrono::day{unsigned(std::stoi(s.substr(8, 2)))}}.ok();
}
static bool blank(const std::string& s) {
    return s.empty() || std::all_of(s.begin(), s.end(), [](unsigned char c) { return c <= ' '; });
}
static std::int64_t civilMinute(const std::string& s) {
    const auto date = std::chrono::year{std::stoi(s.substr(0, 4))}/unsigned(std::stoi(s.substr(5, 2)))/unsigned(std::stoi(s.substr(8, 2)));
    return std::int64_t(std::chrono::sys_days{date}.time_since_epoch().count()) * 1440 + std::stoi(s.substr(11, 2)) * 60 + std::stoi(s.substr(14, 2));
}
bool validStamp(const std::string& s) {
    if (s.size() != 16 || s[10] != ' ' || s[13] != ':' || !validDate(s.substr(0, 10))) return false;
    for (auto i : {11, 12, 14, 15}) if (s[std::size_t(i)] < '0' || s[std::size_t(i)] > '9') return false;
    return std::stoi(s.substr(11, 2)) < 24 && std::stoi(s.substr(14, 2)) < 60;
}
bool validUtcStamp(const std::string& s) {
    if (s.size() != 20 || s[10] != 'T' || s[16] != ':' || s[19] != 'Z') return false;
    auto local = s.substr(0, 16); local[10] = ' ';
    return validStamp(local) && s[17] >= '0' && s[17] <= '5' && s[18] >= '0' && s[18] <= '9';
}
int currencyDecimals(const std::string& c) {
    if (c == "KRW" || c == "JPY") return 0;
    if (c == "USD" || c == "EUR") return 2;
    throw std::runtime_error("지원 통화는 KRW/JPY/USD/EUR입니다. 통화 환산은 지원하지 않습니다.");
}
void validatePlan(const PlanInput& p) {
    currencyDecimals(p.currency);
    if (p.costProfile != "cost-day-minor-half-up-v1") throw std::runtime_error("지원하지 않는 원가 계산 정책입니다.");
    if (blank(p.name)) throw std::runtime_error("프로젝트 이름은 필수입니다.");
    if (!validDate(p.startDate)) throw std::runtime_error("시작일은 유효한 YYYY-MM-DD 날짜여야 합니다.");
    if (p.calendar != "standard-mon-fri-8h-v1") throw std::runtime_error("지원하지 않는 근무달력입니다.");
    if (p.calculationProfile != "native-fs0-workminutes-v1" && !isPdm(p)) throw std::runtime_error("지원하지 않는 계산 정책입니다.");
    std::unordered_set<std::string> calendarIds;
    for (const auto& c : p.calendars) {
        calculationCheckpoint();
        if (blank(c.id) || blank(c.name) || !calendarIds.insert(c.id).second || c.weekdays.empty() || c.periods.empty()) throw std::runtime_error("작업 달력 ID·이름·근무 요일·시간대가 필요합니다.");
        std::unordered_set<int> weekdays;
        for (int day : c.weekdays) if (day < 1 || day > 7 || !weekdays.insert(day).second) throw std::runtime_error("근무 요일은 중복 없이 월=1~일=7입니다.");
        int end = -1;
        for (const auto& period : c.periods) { if (period.begin < 0 || period.end > 1440 || period.begin >= period.end || period.begin < end) throw std::runtime_error("근무 시간대는 정렬된 비중복 00:00~24:00 구간이어야 합니다."); end = period.end; }
        std::unordered_set<std::string> dates;
        for (const auto& d : c.holidays) if (!validDate(d) || !dates.insert(d).second) throw std::runtime_error("작업 달력 휴일 오류/중복입니다.");
    }
    if (!p.calendars.empty() && !isPdm(p)) throw std::runtime_error("작업 달력에는 PDM 계산 정책이 필요합니다.");
    std::unordered_set<std::string> holidays;
    for (const auto& date : p.holidays)
        if (!validDate(date) || !holidays.insert(date).second) throw std::runtime_error("휴일 날짜가 잘못되었거나 중복되었습니다.");
    std::unordered_set<std::string> ids;
    std::unordered_map<std::string, std::size_t> nodes;
    for (std::size_t i = 0; i < p.wbsNodes.size(); ++i) {
        const auto& node = p.wbsNodes[i];
        if (blank(node.id) || blank(node.name) || !nodes.emplace(node.id, i).second)
            throw std::runtime_error("WBS ID 중복 또는 이름 누락입니다.");
    }
    for (const auto& node : p.wbsNodes)
        if (!node.parentId.empty() && !nodes.contains(node.parentId)) throw std::runtime_error("WBS 부모가 존재하지 않습니다.");
    std::vector<int> color(p.wbsNodes.size(), 0);
    for (std::size_t i = 0; i < p.wbsNodes.size(); ++i) {
        auto j = i; std::vector<std::size_t> chain;
        while (color[j] == 0) {
            color[j] = 1; chain.push_back(j);
            const auto& parent = p.wbsNodes[j].parentId;
            if (parent.empty()) break;
            j = nodes.at(parent);
            if (color[j] == 1) throw std::runtime_error("WBS 부모 관계에 순환이 있습니다.");
        }
        for (auto index : chain) color[index] = 2;
    }
    for (const auto& t : p.tasks) {
        calculationCheckpoint();
        if (blank(t.id) || blank(t.name) || !ids.insert(t.id).second || nodes.contains(t.id))
            throw std::runtime_error("작업 ID 중복 또는 작업 이름 누락이 있습니다.");
        if (!t.wbsId.empty() && !nodes.contains(t.wbsId)) throw std::runtime_error("작업의 WBS가 존재하지 않습니다.");
        if (t.durationMinutes && (*t.durationMinutes < 0 || *t.durationMinutes > 5000000))
            throw std::runtime_error("기간은 0~5,000,000 근무분 또는 미정이어야 합니다.");
        std::unordered_set<std::string> refs;
        for (const auto& id : t.predecessors)
            if (blank(id) || !refs.insert(id).second) throw std::runtime_error("선행 작업 ID 누락 또는 중복입니다.");
        if (!t.calendarId.empty() && !calendarIds.contains(t.calendarId)) throw std::runtime_error("작업 달력이 존재하지 않습니다.");
        std::unordered_set<std::string> rules;
        for (const auto& d : t.dependencyRules) {
            if (!refs.contains(d.predecessorId) || !rules.insert(d.predecessorId).second ||
                (d.type != "FS" && d.type != "SS" && d.type != "FF" && d.type != "SF") || d.lagMinutes < -5000000 || d.lagMinutes > 5000000 || (d.type == "FS" && d.lagMinutes == 0)) throw std::runtime_error("선행관계 종류·근무분 지연/선행·중복 속성 오류입니다.");
        }
        if ((!t.dependencyRules.empty() || !t.calendarId.empty()) && !isPdm(p)) throw std::runtime_error("확장 선행관계에는 PDM 계산 정책이 필요합니다.");
        const bool dated=t.constraintType!="ASAP" && t.constraintType!="ALAP";
        if ((t.constraintType!="ASAP" && t.constraintType!="ALAP" && t.constraintType!="SNET" && t.constraintType!="SNLT" && t.constraintType!="FNET" && t.constraintType!="FNLT" && t.constraintType!="MSO" && t.constraintType!="MFO") || dated!=t.constraintAt.has_value() || (t.constraintAt&&!validStamp(*t.constraintAt)) || (t.deadlineAt&&!validStamp(*t.deadlineAt))) throw std::runtime_error("작업 제약 종류·제약 시각·마감일이 잘못되었습니다. 날짜는 YYYY-MM-DD HH:mm입니다.");
        if ((t.constraintType!="ASAP" || t.constraintAt || t.deadlineAt) && !hasConstraints(p)) throw std::runtime_error("작업 제약/마감일에는 제약 계산 정책이 필요합니다.");
        if ((t.taskType!="fixed-duration" && t.taskType!="fixed-work" && t.taskType!="fixed-units") ||
            (t.workUnitMinutes && (*t.workUnitMinutes<0 || *t.workUnitMinutes>maxWork)) ||
            (t.taskType=="fixed-duration" && (t.workDriven || t.workUnitMinutes)) ||
            (t.taskType=="fixed-work" && !t.workDriven) || (t.workDriven!=t.workUnitMinutes.has_value())) throw std::runtime_error("작업 유형·계산 기준·공수 값이 일치하지 않습니다.");
        std::string last;
        if(t.breaks.size()>1000)throw std::runtime_error("작업 중단 구간은 최대 1,000개입니다.");
        for(const auto& gap:t.breaks) {
            if(!validStamp(gap.start)||!validStamp(gap.finish)||gap.start>=gap.finish||(!last.empty()&&gap.start<last)) throw std::runtime_error("중단 구간은 시작 < 재개 시각이며 정렬되고 겹치지 않아야 합니다.");
            last=gap.finish;
        }
        if((t.taskType!="fixed-duration" || t.workDriven || !t.breaks.empty())&&!isEffort(p))throw std::runtime_error("공수·분할 작업에는 공수 일정 정책이 필요합니다.");
    }
    std::unordered_set<std::string> resources, pairs;
    const auto validUnits = [](const std::optional<int>& n) { return !n || (*n >= 0 && *n <= 100000); };
    for (const auto& r : p.resources) {
        calculationCheckpoint();
        if(!r.calendarId.empty()&&(!isEffort(p)||!calendarIds.contains(r.calendarId)))throw std::runtime_error("자원 달력이 없거나 공수 일정 정책이 아닙니다.");
        if (blank(r.id) || blank(r.name) || !resources.insert(r.id).second || ids.contains(r.id) || nodes.contains(r.id) ||
            (r.type != "labor" && r.type != "equipment") || !validUnits(r.maxUnits))
            throw std::runtime_error("자원 ID·이름·종류·가용량이 잘못되었습니다. 가용량은 미정 또는 0~100,000%입니다.");
        std::unordered_set<std::string> dates;
        for (const auto& d : r.exceptions) if (!validDate(d.date) || !dates.insert(d.date).second || !validUnits(d.units))
            throw std::runtime_error("자원 가용량 예외 날짜 중복 또는 값 오류입니다.");
        dates.clear();
        for (const auto& rate : r.rates) if (!validDate(rate.effectiveDate) || !dates.insert(rate.effectiveDate).second ||
            (rate.hourlyMinor && (*rate.hourlyMinor < 0 || *rate.hourlyMinor > 1000000000LL)))
            throw std::runtime_error("유효일 단가가 잘못되었습니다. 단가는 미정 또는 시간당 0~1,000,000,000 최소 통화단위입니다.");
    }
    for (const auto& a : p.assignments) {
        calculationCheckpoint();
        // Length prefix avoids collisions even if imported IDs contain separators.
        auto pair = std::to_string(a.taskId.size()) + ":" + a.taskId + a.resourceId;
        if (!ids.contains(a.taskId) || !resources.contains(a.resourceId) || a.units <= 0 || a.units > 100000 || !pairs.insert(pair).second)
            throw std::runtime_error("배정할 작업·자원이 없거나 배정률 중복/범위 오류입니다. 1~100,000%를 사용하세요.");
    }
    std::unordered_set<std::string> costs;
    for (const auto& c : p.taskCosts) if (!ids.contains(c.taskId) || !costs.insert(c.taskId).second ||
        (c.fixedMinor && (*c.fixedMinor < 0 || *c.fixedMinor > maxMoney)) ||
        (c.accrual != "uniform" && c.accrual != "start" && c.accrual != "finish") ||
        (c.credit != "physical" && c.credit != "zero-hundred"))
        throw std::runtime_error("작업 고정비·발생 방식·획득가치 측정법이 잘못되었습니다.");
}
void validate(const Project& p) {
    validatePlan(p);
    if (blank(p.documentId) || p.revision == 0 || p.revision > 9007199254740991ULL)
        throw std::runtime_error("프로젝트 ID 또는 revision이 잘못되었습니다.");
    if (p.statusAt && !validStamp(*p.statusAt)) throw std::runtime_error("상태일은 YYYY-MM-DD HH:mm이어야 합니다.");
    std::unordered_set<std::string> baselineIds;
    for (std::size_t i = 0; i < p.baselines.size(); ++i) {
        calculationCheckpoint();
        const auto& b = p.baselines[i]; validatePlan(b.input);
        if (blank(b.id) || !baselineIds.insert(b.id).second || blank(b.name) || blank(b.approvedBy) || blank(b.reason) ||
            !validUtcStamp(b.createdAt) || blank(b.sourceDocumentId) || b.inputRevision == 0 || b.inputRevision >= p.revision ||
            b.version != i + 1 || b.engineVersion != engineFor(b.input) || !validStamp(b.projectFinish) || b.schedule.size() != b.input.tasks.size())
            throw std::runtime_error("기준선 메타데이터·버전·계산 결과가 잘못되었습니다.");
        std::unordered_set<std::string> taskIds;
        for (const auto& t : b.input.tasks) taskIds.insert(t.id);
        for (const auto& row : b.schedule) {
        calculationCheckpoint();
            if (!taskIds.erase(row.taskId) || !validStamp(row.start) || !validStamp(row.finish) || row.start > row.finish ||
                row.completedActual || row.remainingStart)
                throw std::runtime_error("기준선의 작업 결과가 입력과 일치하지 않습니다.");
        }
    }
    std::unordered_set<std::string> taskIds;
    for (const auto& t : p.tasks) taskIds.insert(t.id);
    std::unordered_map<std::string, std::string> previous;
    std::unordered_set<std::string> entryIds;
    for (const auto& e : p.progress) {
        calculationCheckpoint();
        if(e.remainingWorkUnitMinutes&&!isEffort(p))throw std::runtime_error("명시 잔여 공수에는 공수 일정 정책이 필요합니다.");
        if(e.remainingWorkUnitMinutes && (*e.remainingWorkUnitMinutes<0 || *e.remainingWorkUnitMinutes>maxWork || (e.actualFinish&&*e.remainingWorkUnitMinutes!=0))) throw std::runtime_error("잔여 공수는 0~500,000,000,000 단위분이며 완료 보고에는 0이어야 합니다.");
        if (blank(e.id) || !entryIds.insert(e.id).second || !taskIds.contains(e.taskId) || !validStamp(e.statusAt) ||
            !validUtcStamp(e.recordedAt) || blank(e.reporter) || blank(e.reason) || blank(e.sourceDocumentId) || e.recordedRevision == 0 || e.recordedRevision > p.revision ||
            e.actualDurationMinutes < 0 || e.actualDurationMinutes > 5000000 || e.remainingMinutes < 0 || e.remainingMinutes > 5000000 ||
            (e.planDurationMinutes && (*e.planDurationMinutes < 0 || *e.planDurationMinutes > 5000000)) ||
            e.previousEntryId != previous[e.taskId]) throw std::runtime_error("실적 보고 메타데이터·수치·이력 연결이 잘못되었습니다.");
        if ((e.actualStart && (!validStamp(*e.actualStart) || *e.actualStart > e.statusAt)) ||
            (e.actualFinish && (!validStamp(*e.actualFinish) || !e.actualStart || *e.actualFinish < *e.actualStart || *e.actualFinish > e.statusAt)) ||
            (!e.actualStart && e.actualDurationMinutes != 0) || (e.actualFinish && e.remainingMinutes != 0) ||
            (e.actualStart && !e.actualFinish && e.remainingMinutes == 0 && !e.remainingWorkUnitMinutes))
            throw std::runtime_error("실제 날짜·상태일·잔여 기간이 모순됩니다. 완료 시 실제 완료일과 잔여 0을 함께 입력하세요.");
        if (!e.actualStart && e.remainingMinutes == 0 && e.planDurationMinutes.value_or(1) != 0)
            throw std::runtime_error("기간이 있는 작업의 잔여를 0으로 보고하려면 실제 시작·완료를 입력하세요.");
        if (e.actualStart && e.actualDurationMinutes > civilMinute(e.actualFinish.value_or(e.statusAt)) - civilMinute(*e.actualStart))
            throw std::runtime_error("실제 소요 기간이 실제 시작부터 완료/상태일까지의 경과시간보다 큽니다. 공수와 기간을 구분해 입력하세요.");
        previous[e.taskId] = e.id;
    }
    std::unordered_set<std::string> scenarios;
    for (const auto& s : p.scenarios) {
        calculationCheckpoint();
        if (blank(s.id) || !scenarios.insert(s.id).second || blank(s.name) || !validUtcStamp(s.createdAt) || blank(s.sourceDocumentId) ||
            !s.inputRevision || s.inputRevision >= p.revision || s.policy != levelingPolicyFor(s.input) ||
            (s.priority != "task-order" && s.priority != "shortest-first" && s.priority != "longest-first") ||
            s.horizonWorkDays < 1 || s.horizonWorkDays > 5200)
            throw std::runtime_error("평준화 시나리오의 메타데이터·정책·탐색 범위가 잘못되었습니다.");
        Project snapshot; static_cast<PlanInput&>(snapshot) = s.input;
        snapshot.documentId = s.sourceDocumentId; snapshot.revision = s.inputRevision;
        snapshot.statusAt = s.statusAt; snapshot.progress = s.progress;
        validate(snapshot);
    }
    previous.clear(); entryIds.clear();
    for (const auto& e : p.costEntries) {
        calculationCheckpoint();
        if (blank(e.id) || !entryIds.insert(e.id).second || !taskIds.contains(e.taskId) || !validStamp(e.statusAt) ||
            !validUtcStamp(e.recordedAt) || blank(e.reporter) || blank(e.reason) || blank(e.sourceDocumentId) || e.currency != p.currency ||
            !e.recordedRevision || e.recordedRevision > p.revision || e.previousEntryId != previous[e.taskId] ||
            (e.actualMinor && (*e.actualMinor < 0 || *e.actualMinor > maxMoney)) ||
            (e.physicalBasisPoints && (*e.physicalBasisPoints < 0 || *e.physicalBasisPoints > 10000)))
            throw std::runtime_error("원가·물리 진척 보고의 통화·수치·출처·이력 연결이 잘못되었습니다.");
        previous[e.taskId] = e.id;
    }
}
const ProgressEntry* progressAsOf(const Project& p, const std::string& taskId) {
    if (!p.statusAt) return nullptr;
    const ProgressEntry* result = nullptr;
    for (const auto& entry : p.progress) if (entry.taskId == taskId && entry.statusAt <= *p.statusAt &&
        (!result || result->statusAt <= entry.statusAt)) result = &entry;
    return result;
}
const CostEntry* costAsOf(const Project& p, const std::string& taskId, const std::string& stamp) {
    const CostEntry* result = nullptr;
    for (const auto& e : p.costEntries) if (e.taskId == taskId && e.statusAt <= stamp && (!result || e.statusAt >= result->statusAt)) result = &e;
    return result;
}
}
