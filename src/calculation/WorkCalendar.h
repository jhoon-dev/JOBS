#pragma once
#include "domain/Project.h"
#include <chrono>
namespace jobs {
class WorkCalendar {
    struct Interval { std::int64_t begin, end, prefix; };
    std::vector<Interval> intervals_;
    std::chrono::sys_days anchor_, horizon_;
    bool elapsed_ = false;
    void initialize(const PlanInput&, const Task*);
public:
    explicit WorkCalendar(const PlanInput& plan);
    WorkCalendar(const PlanInput&, const Task&);
    std::string boundary(std::int64_t offset, bool finish) const;
    std::int64_t offset(const std::string& civilStamp) const;
    std::int64_t startAtLeast(const std::string&) const;
    std::int64_t finishAtLeast(const std::string&) const;
    std::string shift(const std::string&, std::int64_t minutes) const;
    std::vector<std::pair<std::string,std::string>> segments(const std::string& start, const std::string& finish) const;
    int dayMinutes() const { return elapsed_ ? 1440 : 480; }
    std::int64_t maxOffset() const { return intervals_.empty()?0:intervals_.back().prefix+intervals_.back().end-intervals_.back().begin; }
    std::vector<std::pair<std::int64_t,std::int64_t>> civilIntervals() const;
};
}
