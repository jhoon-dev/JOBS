#pragma once
#include "domain/Project.h"
#include <chrono>
namespace jobs {
class WorkCalendar {
    std::vector<std::chrono::sys_days> days_;
    std::chrono::sys_days anchor_, horizon_;
public:
    explicit WorkCalendar(const PlanInput& plan);
    std::string boundary(std::int64_t offset, bool finish) const;
    std::int64_t offset(const std::string& civilStamp) const;
};
}
