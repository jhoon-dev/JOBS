#pragma once
#include "application/Document.h"
#include <QString>
namespace jobs {
struct TaskViewSpec { std::string wbs; QString query; int filter=0, sort=0; };
struct TaskViewProjection { std::vector<std::size_t> rows; std::size_t scopeCount=0; };
TaskViewProjection projectTaskView(const Project&,const ScheduleResult*,const TaskViewSpec&);
void prepareTaskView(BulkView&,const Project& before,const Project& after,const TaskViewSpec&,
                     std::vector<std::size_t> previousRows,std::string token,std::uint64_t calculationGeneration);
}
