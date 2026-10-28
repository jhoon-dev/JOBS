#pragma once
#include "domain/Project.h"
#include <QByteArray>

namespace jobs {
inline constexpr qsizetype projectFileLimitBytes = 64 * 1024 * 1024;
// The same bounded, side-effect-free decoder used by load and pre-save verification.
Project decodeProjectFile(const QByteArray& bytes);
}
