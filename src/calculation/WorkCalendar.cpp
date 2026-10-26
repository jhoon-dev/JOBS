#include "calculation/WorkCalendar.h"
#include "domain/CalculationControl.h"
#include <algorithm>
#include <cstdio>
#include <stdexcept>
#include <unordered_set>
namespace jobs {
namespace {
using namespace std::chrono;
sys_days date(const std::string& s) { return sys_days{year{std::stoi(s.substr(0,4))}/unsigned(std::stoi(s.substr(5,2)))/unsigned(std::stoi(s.substr(8,2)))}; }
std::string stamp(sys_days anchor, std::int64_t minute) {
    const auto day = anchor + days{static_cast<days::rep>(minute / 1440)}; const auto ymd = year_month_day{day}; const int m = int(minute % 1440); char out[32];
    if (int(ymd.year())>9999) throw std::runtime_error("달력 시각이 지원 연도 범위를 초과했습니다.");
    std::snprintf(out, sizeof(out), "%04d-%02u-%02u %02d:%02d", int(ymd.year()), unsigned(ymd.month()), unsigned(ymd.day()), m / 60, m % 60); return out;
}
}
WorkCalendar::WorkCalendar(const PlanInput& p) { initialize(p, nullptr); }
WorkCalendar::WorkCalendar(const PlanInput& p, const Task& t) { initialize(p, &t); }
void WorkCalendar::initialize(const PlanInput& p, const Task* task) {
    TaskCalendar calendar; calendar.holidays = p.holidays;
    elapsed_ = !task && isPdm(p);
    if (elapsed_) { calendar.weekdays = {1,2,3,4,5,6,7}; calendar.periods = {{0,1440}}; calendar.holidays.clear(); }
    else if (task && !task->calendarId.empty()) {
        auto it = std::find_if(p.calendars.begin(), p.calendars.end(), [&](const auto& c) { return c.id == task->calendarId; });
        if (it == p.calendars.end()) throw std::runtime_error("작업 달력이 없습니다.");
        calendar = *it;
    }
    anchor_ = date(p.startDate); horizon_ = std::min(anchor_ + days{366*20}, sys_days{year{10000}/1/1});
    EvaluationScope evaluation;
    cacheKey_="calendar/"+p.startDate+"/"+std::to_string(elapsed_);
    auto field=[&](const std::string& value){cacheKey_+="/"+std::to_string(value.size())+":"+value;};
    for(int day:calendar.weekdays)field(std::to_string(day));
    field("periods");
    for(const auto& period:calendar.periods){field(std::to_string(period.begin));field(std::to_string(period.end));}
    field("holidays");for(const auto& day:calendar.holidays)field(day);
    if(task&&isEffort(p)){field("breaks");for(const auto& gap:task->breaks){field(gap.start);field(gap.finish);}}
    if(auto cached=evaluationCached(cacheKey_)){intervals_=std::static_pointer_cast<const std::vector<Interval>>(cached);return;}
    std::vector<Interval> intervals;
    std::unordered_set<std::int64_t> holidays; for (const auto& h : calendar.holidays) holidays.insert(date(h).time_since_epoch().count());
    std::int64_t prefix = 0;
    for (auto d = anchor_; d < horizon_; d += days{1}) {
        if((d-anchor_).count()%64==0)calculationCheckpoint();
        if (std::find(calendar.weekdays.begin(), calendar.weekdays.end(), int(weekday{d}.iso_encoding())) == calendar.weekdays.end() || holidays.contains(d.time_since_epoch().count())) continue;
        auto base = (d-anchor_).count()*1440;
        for (const auto& period : calendar.periods) { intervals.push_back({base+period.begin, base+period.end, prefix}); prefix += period.end-period.begin; }
    }
    if(task && isEffort(p) && !task->breaks.empty()) {
        std::vector<Interval> kept; prefix=0;
        std::vector<std::pair<std::int64_t,std::int64_t>> gaps;
        const auto minute=[&](const std::string& s){return (date(s)-anchor_).count()*1440+std::stoi(s.substr(11,2))*60+std::stoi(s.substr(14,2));};
        for(const auto& gap:task->breaks)gaps.emplace_back(minute(gap.start),minute(gap.finish));
        std::size_t cursor=0;
        for(const auto& i:intervals) {
            auto at=i.begin;
            while(cursor<gaps.size()&&gaps[cursor].second<=at)++cursor;
            for(auto j=cursor;j<gaps.size()&&gaps[j].first<i.end;++j) {
                const auto [begin,end]=gaps[j];
                if(begin>at){kept.push_back({at,begin,prefix});prefix+=begin-at;}
                at=std::max(at,end);if(at>=i.end)break;
            }
            if(at<i.end){kept.push_back({at,i.end,prefix});prefix+=i.end-at;}
        }
        intervals=std::move(kept);
    }
    intervals_=std::make_shared<const std::vector<Interval>>(std::move(intervals));
    evaluationRemember(cacheKey_,intervals_);
}

std::vector<std::pair<std::int64_t,std::int64_t>> WorkCalendar::civilIntervals() const {
    std::vector<std::pair<std::int64_t,std::int64_t>> out;out.reserve(intervals_->size());
    for(const auto& i:*intervals_)out.emplace_back(i.begin,i.end);
    return out;
}
std::string WorkCalendar::boundary(std::int64_t value, bool finish) const {
    calculationCheckpoint();
    if (value < 0 || intervals_->empty()) throw std::runtime_error("달력 근무분이 음수이거나 20년 범위에 가용 시간이 없습니다.");
    auto it = std::upper_bound(intervals_->begin(), intervals_->end(), value - (finish && value > 0 ? 1 : 0), [](auto n, const auto& i) { return n < i.prefix; });
    if (it == intervals_->begin()) throw std::runtime_error("달력 범위 밖입니다.");
    --it;
    if (value > it->prefix + it->end-it->begin || (!finish && value == it->prefix + it->end-it->begin)) throw std::runtime_error("계획 시작부터 20년 이내에 근무시간이 부족합니다.");
    return stamp(anchor_, it->begin + value-it->prefix);
}
std::int64_t WorkCalendar::offset(const std::string& s) const {
    calculationCheckpoint();
    if (!validStamp(s)) throw std::runtime_error("잘못된 현지 날짜·시각입니다.");
    const auto d = date(s);
    if (d < anchor_ || d >= horizon_) throw std::runtime_error("시각이 프로젝트 시작부터 20년의 계산 범위 밖입니다.");
    auto civil = (d-anchor_).count()*1440 + std::stoi(s.substr(11,2))*60 + std::stoi(s.substr(14,2));
    auto it = std::upper_bound(intervals_->begin(), intervals_->end(), civil, [](auto n, const auto& i) { return n < i.begin; });
    if (it == intervals_->begin()) return 0;
    --it;
    return it->prefix + std::min(civil-it->begin, it->end-it->begin);
}
std::int64_t WorkCalendar::startAtLeast(const std::string& s) const { auto n = offset(s); if (boundary(n, false) < s) ++n; return n; }
std::int64_t WorkCalendar::finishAtLeast(const std::string& s) const { auto n = offset(s); if (boundary(n, true) < s) ++n; return n; }
std::string WorkCalendar::shift(const std::string& s, std::int64_t minutes) const {
    if (!minutes) return s;
    auto n = std::max(std::int64_t{0}, offset(s)+minutes); return boundary(n, minutes > 0);
}
std::vector<std::pair<std::string,std::string>> WorkCalendar::segments(const std::string& start, const std::string& finish) const {
    std::vector<std::pair<std::string,std::string>> out;
    offset(start); offset(finish);
    const auto civil=[&](const std::string& s){return (date(s)-anchor_).count()*1440+std::stoi(s.substr(11,2))*60+std::stoi(s.substr(14,2));};
    const auto first=std::int64_t(civil(start)),endMinute=std::int64_t(civil(finish));
    for (const auto& i : *intervals_) {
        if (i.begin >= endMinute) break;
        if (i.end > first) out.emplace_back(stamp(anchor_,std::max(i.begin,first)),stamp(anchor_,std::min(i.end,endMinute)));
    }
    return out;
}
}
