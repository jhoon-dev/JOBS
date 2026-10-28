#include "domain/Wbs.h"
#include <unordered_map>
#include <stdexcept>
namespace jobs {
std::unordered_set<std::string> wbsSubtree(const PlanInput &p, const std::string &id) {
    std::unordered_map<std::string, std::vector<std::string>> children;
    bool found = id.empty();
    for (const auto &node : p.wbsNodes) {
        children[node.parentId].push_back(node.id);
        found |= node.id == id;
    }
    if (!found)
        throw std::runtime_error("선택한 WBS가 존재하지 않습니다.");
    std::unordered_set<std::string> result;
    std::vector<std::string> pending{id};
    while (!pending.empty()) {
        auto current = std::move(pending.back());
        pending.pop_back();
        if (!result.insert(current).second)
            continue;
        auto it = children.find(current);
        if (it != children.end())
            for (const auto &child : it->second)
                pending.push_back(child);
    }
    return result;
}
std::vector<std::size_t> wbsTaskRows(const PlanInput &p, const std::string &id) {
    const auto members = wbsSubtree(p, id);
    std::vector<std::size_t> rows;
    for (std::size_t i = 0; i < p.tasks.size(); ++i)
        if (members.contains(p.tasks[i].wbsId))
            rows.push_back(i);
    return rows;
}
} // namespace jobs
