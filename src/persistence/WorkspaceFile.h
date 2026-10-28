#pragma once
#include "domain/Workspace.h"
#include <QByteArray>
#include <QString>

namespace jobs {
// Strict in-memory decoder: path resolution is lexical and never opens referenced files.
Workspace decodeWorkspaceFile(const QByteArray &, const QString &baseDirectory);
} // namespace jobs
