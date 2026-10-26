#pragma once
#include "domain/Project.h"
#include <unordered_set>
namespace jobs {
// Empty ID means the project root, including ungrouped tasks.
std::unordered_set<std::string> wbsSubtree(const Project& project, const std::string& id);
std::vector<std::size_t> wbsTaskRows(const Project& project, const std::string& id);
}
