#pragma once
#include "domain/Project.h"
#include <QByteArray>
#include <QJsonObject>
#include <QString>
namespace jobs {
QString publishPlanExchange(const Project &, const QByteArray &, QJsonObject, bool importing, const QString &extension,
                            const QString &destination);
}
