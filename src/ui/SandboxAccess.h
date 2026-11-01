#pragma once
#include <QString>
class QWidget;

namespace jobs {
bool sandboxActive();
void restoreSandboxFolders();
// Keeps folder access alive until shutdown, including asynchronous publication.
// Other distributions return true without changing permissions or settings.
bool authorizeSandboxFile(QWidget *parent, const QString &path);
} // namespace jobs
