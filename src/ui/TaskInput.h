#pragma once
#include "domain/Project.h"
#include <QStringList>
namespace jobs {
std::optional<std::int64_t> taskDurationInput(const QString&);
std::vector<Dependency> taskDependencyInput(const QString&,const Project&,const std::string& taskId);
QString encodeTaskTsv(const QList<QStringList>&);
QList<QStringList> decodeTaskTsv(const QString&);
}
