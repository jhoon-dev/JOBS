#pragma once
#include "domain/Diagnostics.h"
#include <QFileInfo>
#include <QString>

namespace jobs {
inline void requireNativeFilePath(const QString &path, const char *suffix) {
    if (QFileInfo(path).suffix().compare(QString::fromLatin1(suffix), Qt::CaseInsensitive) != 0)
        throw AppError(ErrorCode::FILE_EXTENSION_UNSUPPORTED, {std::string(".") + suffix},
                       ErrorContext{.path = path.toUtf8().toStdString()});
}
} // namespace jobs
