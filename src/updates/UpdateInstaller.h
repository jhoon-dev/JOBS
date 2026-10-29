#pragma once
#include <QString>
#include <QUrl>
namespace jobs {
bool markDownloadedInstaller(const QString &path, const QUrl &source);
bool launchUpdateInstaller(const QString &path);
} // namespace jobs
