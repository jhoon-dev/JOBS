#include "updates/UpdateInstaller.h"
#include <QCoreApplication>
#include <QFile>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shellapi.h>
#endif
namespace jobs {
bool markDownloadedInstaller(const QString &path, const QUrl &source) {
#ifdef Q_OS_WIN
    QFile zone(path + ":Zone.Identifier");
    const QByteArray data = "[ZoneTransfer]\r\nZoneId=3\r\nHostUrl=" + source.toEncoded() + "\r\n";
    return zone.open(QIODevice::WriteOnly | QIODevice::Truncate) && zone.write(data) == data.size() && zone.flush();
#else
    Q_UNUSED(path)
    Q_UNUSED(source)
    return true;
#endif
}
bool launchUpdateInstaller(const QString &path) {
#ifdef Q_OS_WIN
    const auto file = path.toStdWString();
    const auto arguments = ("/WAITPID=" + QString::number(QCoreApplication::applicationPid())).toStdWString();
    SHELLEXECUTEINFOW execute{};
    execute.cbSize = sizeof(execute);
    execute.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    execute.lpVerb = L"open";
    execute.lpFile = file.c_str();
    execute.lpParameters = arguments.c_str();
    execute.nShow = SW_SHOWNORMAL;
    if (!ShellExecuteExW(&execute))
        return false;
    if (execute.hProcess)
        CloseHandle(execute.hProcess);
    return true;
#else
    Q_UNUSED(path)
    return false;
#endif
}
} // namespace jobs
