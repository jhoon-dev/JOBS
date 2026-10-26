#include "calculation/WorkCalendar.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <unordered_set>
namespace jobs {
namespace {
using namespace std::chrono;
sys_days date(const std::string& s) {
    return sys_days{year{std::stoi(s.substr(0, 4))}/unsigned(std::stoi(s.substr(5, 2)))/unsigned(std::stoi(s.substr(8, 2)))};
}
std::string stamp(sys_days d, int minute) {
    auto ymd = year_month_day{d}; char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%04d-%02u-%02u %02d:%02d", int(ymd.year()), unsigned(ymd.month()), unsigned(ymd.day()), minute / 60, minute % 60);
    return buffer;
}
}
WorkCalendar::WorkCalendar(const PlanInput& p) {
    std::unordered_set<std::int64_t> holidays;
    for (const auto& h : p.holidays) holidays.insert(date(h).time_since_epoch().count());
    anchor_ = date(p.startDate); horizon_ = anchor_ + std::chrono::days{366 * 20};
    for (auto d = anchor_; d < horizon_ && std::chrono::year_month_day{d}.year() <= std::chrono::year{9999}; d += std::chrono::days{1})
        if (std::chrono::weekday{d}.iso_encoding() <= 5 && !holidays.contains(d.time_since_epoch().count())) days_.push_back(d);
}
std::string WorkCalendar::boundary(std::int64_t offset, bool finish) const {
    if (offset < 0) throw std::runtime_error("계산 offset이 음수입니다.");
    const auto index = finish && offset > 0 ? (offset - 1) / 480 : offset / 480;
    int inDay = int(finish && offset > 0 ? (offset - 1) % 480 + 1 : offset % 480);
    if (index >= std::int64_t(days_.size())) throw std::runtime_error("계획 시작부터 20년 이내에 근무시간이 부족합니다.");
    const auto minute = inDay < 180 || (finish && inDay == 180) ? 9 * 60 + inDay : 13 * 60 + inDay - 180;
    return stamp(days_[std::size_t(index)], minute);
}
std::int64_t WorkCalendar::offset(const std::string& s) const {
    if (!validStamp(s)) throw std::runtime_error("잘못된 현지 날짜·시각입니다.");
    const auto d = date(s);
    if (d < anchor_ || d >= horizon_) throw std::runtime_error("실적/비교 시각이 프로젝트 시작부터 20년의 계산 범위 밖입니다.");
    const auto it = std::lower_bound(days_.begin(), days_.end(), d);
    auto value = std::int64_t(it - days_.begin()) * 480;
    if (it != days_.end() && *it == d) {
        const auto minute = std::stoi(s.substr(11, 2)) * 60 + std::stoi(s.substr(14, 2));
        value += std::clamp(minute - 9 * 60, 0, 180) + std::clamp(minute - 13 * 60, 0, 300);
    }
    return value;
}
}
