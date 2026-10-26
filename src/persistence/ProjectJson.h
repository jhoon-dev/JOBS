#pragma once
#include "domain/Project.h"
#include <QJsonObject>
namespace jobs::json {
QJsonObject encodeProject(const Project& project);
Project decodeProject(const QJsonObject& object, int schema);
}
