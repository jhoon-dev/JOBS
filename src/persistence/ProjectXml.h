#pragma once
#include "domain/Project.h"
#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace jobs {
// A prepared, immutable-by-convention preview. No filesystem or Document mutation.
struct ProjectXmlExchange {
    Project project;
    QByteArray xml;
    QJsonObject report;
    bool importing = true;
};
QByteArray readProjectXml(const QString& path);
ProjectXmlExchange importProjectXml(const QByteArray&, const QString& sourceName);
ProjectXmlExchange exportProjectXml(const Project&);
// Create-only directory publication on the same filesystem. Returns the new directory.
QString publishProjectXml(const ProjectXmlExchange&, const QString& destination);
QString projectXmlPreview(const ProjectXmlExchange&);
}
