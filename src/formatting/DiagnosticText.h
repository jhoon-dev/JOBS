#pragma once
#include "domain/Diagnostics.h"
namespace jobs {
enum class DiagnosticLanguage { Korean, English };
std::string diagnosticMessage(const Diagnostic &, DiagnosticLanguage language = DiagnosticLanguage::Korean);
std::string diagnosticRecommendation(const Diagnostic &, DiagnosticLanguage language = DiagnosticLanguage::Korean);
std::string formatDiagnostic(const Diagnostic &, DiagnosticLanguage language = DiagnosticLanguage::Korean);
std::string errorText(const std::exception &, DiagnosticLanguage language = DiagnosticLanguage::Korean);
} // namespace jobs
