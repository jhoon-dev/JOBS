#pragma once
#include "formatting/DiagnosticText.h"
#include <QString>
namespace jobs {
inline std::string diagnosticArgument(const QString &value) {
    return value.toUtf8().toStdString();
}
inline QString qErrorText(const std::exception &error) {
    return QString::fromStdString(errorText(error));
}
inline QString qDiagnosticText(const Diagnostic &diagnostic) {
    return QString::fromStdString(formatDiagnostic(diagnostic));
}
} // namespace jobs
