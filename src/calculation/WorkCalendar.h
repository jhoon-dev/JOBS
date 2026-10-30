#pragma once
#include "domain/Project.h"
#include <chrono>
#include <memory>
namespace jobs {
class WorkCalendar {
    struct Interval {
        std::int64_t begin, end, prefix;
    };
    std::shared_ptr<const std::vector<Interval>> intervals_;
    std::string cacheKey_;
    std::chrono::sys_days anchor_, horizon_;
    bool elapsed_ = false;
    void initialize(const PlanInput &, const Task *);

public:
    explicit WorkCalendar(const PlanInput &plan);
    WorkCalendar(const PlanInput &, const Task &);
    std::string boundary(std::int64_t offset, bool finish) const;
    std::int64_t offset(const std::string &civilStamp) const;
    std::int64_t startAtLeast(const std::string &) const;
    std::int64_t finishAtLeast(const std::string &) const;
    // Milestone coordinates distinguish both sides of a non-working gap.
    std::int64_t pointOffset(const std::string &) const;
    std::string pointBoundary(std::int64_t) const;
    std::int64_t pointStartAtLeast(const std::string &) const;
    std::int64_t pointEndAtMost(const std::string &) const;
    std::int64_t maxPointOffset() const;
    std::string shift(const std::string &, std::int64_t minutes) const;
    std::vector<std::pair<std::string, std::string>> segments(const std::string &start,
                                                              const std::string &finish) const;
    int dayMinutes() const {
        return elapsed_ ? 1440 : 480;
    }
    std::int64_t maxOffset() const {
        return intervals_->empty() ? 0 : intervals_->back().prefix + intervals_->back().end - intervals_->back().begin;
    }
    std::vector<std::pair<std::int64_t, std::int64_t>> civilIntervals() const;
    const std::string &cacheKey() const {
        return cacheKey_;
    }
};
} // namespace jobs
