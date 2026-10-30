#pragma once
#include "domain/Project.h"
#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <functional>
namespace jobs {
// Optional pre-rename observer supports deterministic interruption/race regression tests.
QString publishPlanExchange(const Project &, const QByteArray &, QJsonObject, bool importing, const QString &extension,
                            const QString &destination, std::function<void()> beforePublish = {});
} // namespace jobs
