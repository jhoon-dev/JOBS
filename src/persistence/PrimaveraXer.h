#pragma once
#include "domain/Project.h"
#include <QByteArray>
#include <QJsonObject>
#include <QString>
namespace jobs {
enum class XerEncoding { Utf8, Windows1252 };
struct PrimaveraXerExchange {
    Project project;
    QByteArray xer;
    QJsonObject report;
    bool importing = true;
};
QByteArray readPrimaveraXer(const QString &path);
PrimaveraXerExchange importPrimaveraXer(const QByteArray &, const QString &sourceName, XerEncoding = XerEncoding::Utf8);
PrimaveraXerExchange exportPrimaveraXer(const Project &);
QString publishPrimaveraXer(const PrimaveraXerExchange &, const QString &destination);
QString primaveraXerPreview(const PrimaveraXerExchange &);
} // namespace jobs
