#include "domain/Project.h"
#include <chrono>
#include <stdexcept>
#include <unordered_set>
#include <unordered_map>
#include <algorithm>

namespace jobs {
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
void validatePlan(const PlanInput& p) {
    if (blank(p.name)) throw std::runtime_error("프로젝트 이름은 필수입니다.");
    if (!validDate(p.startDate)) throw std::runtime_error("시작일은 유효한 YYYY-MM-DD 날짜여야 합니다.");
    if (p.calendar != "standard-mon-fri-8h-v1") throw std::runtime_error("지원하지 않는 근무달력입니다.");
    if (p.calculationProfile != "native-fs0-workminutes-v1") throw std::runtime_error("지원하지 않는 계산 정책입니다.");
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
        if (blank(t.id) || blank(t.name) || !ids.insert(t.id).second || nodes.contains(t.id))
            throw std::runtime_error("작업 ID 중복 또는 작업 이름 누락이 있습니다.");
        if (!t.wbsId.empty() && !nodes.contains(t.wbsId)) throw std::runtime_error("작업의 WBS가 존재하지 않습니다.");
        if (t.durationMinutes && (*t.durationMinutes < 0 || *t.durationMinutes > 5000000))
            throw std::runtime_error("기간은 0~5,000,000 근무분 또는 미정이어야 합니다.");
        std::unordered_set<std::string> refs;
        for (const auto& id : t.predecessors)
            if (blank(id) || !refs.insert(id).second) throw std::runtime_error("선행 작업 ID 누락 또는 중복입니다.");
    }
}
void validate(const Project& p) {
    validatePlan(p);
    if (blank(p.documentId) || p.revision == 0 || p.revision > 9007199254740991ULL)
        throw std::runtime_error("프로젝트 ID 또는 revision이 잘못되었습니다.");
    if (p.statusAt && !validStamp(*p.statusAt)) throw std::runtime_error("상태일은 YYYY-MM-DD HH:mm이어야 합니다.");
    std::unordered_set<std::string> baselineIds;
    for (std::size_t i = 0; i < p.baselines.size(); ++i) {
        const auto& b = p.baselines[i]; validatePlan(b.input);
        if (blank(b.id) || !baselineIds.insert(b.id).second || blank(b.name) || blank(b.approvedBy) || blank(b.reason) ||
            !validUtcStamp(b.createdAt) || blank(b.sourceDocumentId) || b.inputRevision == 0 || b.inputRevision >= p.revision ||
            b.version != i + 1 || b.engineVersion != "jobs-cpm-0.2" || !validStamp(b.projectFinish) || b.schedule.size() != b.input.tasks.size())
            throw std::runtime_error("기준선 메타데이터·버전·계산 결과가 잘못되었습니다.");
        std::unordered_set<std::string> taskIds;
        for (const auto& t : b.input.tasks) taskIds.insert(t.id);
        for (const auto& row : b.schedule) {
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
        if (blank(e.id) || !entryIds.insert(e.id).second || !taskIds.contains(e.taskId) || !validStamp(e.statusAt) ||
            !validUtcStamp(e.recordedAt) || blank(e.reporter) || blank(e.reason) || blank(e.sourceDocumentId) || e.recordedRevision == 0 || e.recordedRevision > p.revision ||
            e.actualDurationMinutes < 0 || e.actualDurationMinutes > 5000000 || e.remainingMinutes < 0 || e.remainingMinutes > 5000000 ||
            (e.planDurationMinutes && (*e.planDurationMinutes < 0 || *e.planDurationMinutes > 5000000)) ||
            e.previousEntryId != previous[e.taskId]) throw std::runtime_error("실적 보고 메타데이터·수치·이력 연결이 잘못되었습니다.");
        if ((e.actualStart && (!validStamp(*e.actualStart) || *e.actualStart > e.statusAt)) ||
            (e.actualFinish && (!validStamp(*e.actualFinish) || !e.actualStart || *e.actualFinish < *e.actualStart || *e.actualFinish > e.statusAt)) ||
            (!e.actualStart && e.actualDurationMinutes != 0) || (e.actualFinish && e.remainingMinutes != 0) ||
            (e.actualStart && !e.actualFinish && e.remainingMinutes == 0))
            throw std::runtime_error("실제 날짜·상태일·잔여 기간이 모순됩니다. 완료 시 실제 완료일과 잔여 0을 함께 입력하세요.");
        if (!e.actualStart && e.remainingMinutes == 0 && e.planDurationMinutes.value_or(1) != 0)
            throw std::runtime_error("기간이 있는 작업의 잔여를 0으로 보고하려면 실제 시작·완료를 입력하세요.");
        if (e.actualStart && e.actualDurationMinutes > civilMinute(e.actualFinish.value_or(e.statusAt)) - civilMinute(*e.actualStart))
            throw std::runtime_error("실제 소요 기간이 실제 시작부터 완료/상태일까지의 경과시간보다 큽니다. 공수와 기간을 구분해 입력하세요.");
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
}
