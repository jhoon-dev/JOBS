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
void validate(const Project& p) {
    if (blank(p.documentId) || blank(p.name)) throw std::runtime_error("프로젝트 ID와 이름은 필수입니다.");
    if (!validDate(p.startDate)) throw std::runtime_error("시작일은 유효한 YYYY-MM-DD 날짜여야 합니다.");
    if (p.calendar != "standard-mon-fri-8h-v1") throw std::runtime_error("지원하지 않는 근무달력입니다.");
    if (p.calculationProfile != "native-fs0-workminutes-v1") throw std::runtime_error("지원하지 않는 계산 정책입니다.");
    std::unordered_set<std::string> holidays;
    for (const auto& date : p.holidays)
        if (!validDate(date) || !holidays.insert(date).second) throw std::runtime_error("휴일 날짜가 잘못되었거나 중복되었습니다.");
    if (p.revision == 0 || p.revision > 9007199254740991ULL)
        throw std::runtime_error("지원하지 않는 revision입니다.");
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
}
