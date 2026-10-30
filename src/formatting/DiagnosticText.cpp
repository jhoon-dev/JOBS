#include "formatting/DiagnosticText.h"
#include "formatting/DiagnosticCatalog.h"
#include <charconv>

namespace jobs {
namespace {
std::string interpolate(std::string_view pattern, const std::vector<std::string> &arguments) {
    std::string result;
    std::vector<bool> used(arguments.size(), false);
    for (std::size_t at = 0; at < pattern.size();) {
        if (pattern[at] == '{') {
            const auto close = pattern.find('}', at + 1);
            std::size_t index = 0;
            if (close != std::string_view::npos) {
                const auto converted = std::from_chars(pattern.data() + at + 1, pattern.data() + close, index);
                if (converted.ec == std::errc{} && converted.ptr == pattern.data() + close) {
                    if (index < arguments.size()) {
                        result += arguments[index];
                        used[index] = true;
                    }
                    at = close + 1;
                    continue;
                }
            }
        }
        result += pattern[at++];
    }
    // The English fallback and unknown templates never hide diagnostic evidence.
    for (std::size_t i = 0; i < arguments.size(); ++i)
        if (!used[i] && !arguments[i].empty())
            result += " · " + arguments[i];
    return result;
}
} // namespace
std::string diagnosticMessage(const Diagnostic &diagnostic, DiagnosticLanguage language) {
    const auto &key = diagnostic.textKey.empty() ? diagnostic.code : diagnostic.textKey;
    std::string_view pattern =
        language == DiagnosticLanguage::Korean ? "오류 정보를 확인하세요." : "Review the diagnostic details.";
    for (const auto &entry : diagnosticcatalog::entries)
        if (entry.key == key) {
            pattern = language == DiagnosticLanguage::Korean ? entry.korean : entry.english;
            break;
        }
    auto result = interpolate(pattern, diagnostic.arguments);
    for (const auto &cause : diagnostic.causes)
        result += "\n[" + cause.code + "] " + diagnosticMessage(cause, language);
    return result;
}
std::string diagnosticRecommendation(const Diagnostic &diagnostic, DiagnosticLanguage language) {
    const auto &code = diagnostic.code;
    if (code.starts_with("POLICY_") || code.ends_with("_POLICY") || code == "BASELINE_SCOPE" ||
        code == "RESOURCE_NOT_CHECKED" || code == "RESOURCE_DELAY" || code == "RESOURCE_WAIT" ||
        code == "WORKSPACE_UNMAPPED_RESOURCES" || code == "WORKSPACE_CANCELLED")
        return {};
    if (language == DiagnosticLanguage::English)
        return "Review the related task, resource, calendar or file and correct the indicated input before retrying.";
    if (code == "FILE_JSONRECOVERYSTORE_015" || code == "FILE_JSONRECOVERYSTORE_016" ||
        code == "FILE_JSONRECOVERYSTORE_017" || code == "FILE_JSONRECOVERYSTORE_018" ||
        code == "FILE_JSONRECOVERYSTORE_019")
        return "다른 검증된 복구 세대를 선택하거나 명시 저장본을 여세요. 손상된 복구본은 원인 확인 전까지 보존하세요.";
    if (code.find("FILE") != std::string::npos || code.find("SAVE") != std::string::npos)
        return "파일 경로·권한·열린 원본의 변경 여부를 확인하세요. 원본을 보존하려면 새 경로에 사본을 저장하세요.";
    if (code.find("DURATION") != std::string::npos || code.starts_with("WORK_"))
        return "관련 작업의 기간·공수·작업 유형과 자원 배정을 확인한 뒤 다시 계산하세요.";
    if (code.find("CALENDAR") != std::string::npos || code.find("CONSTRAINT") != std::string::npos)
        return "관련 작업의 근무 달력·제약 날짜·선행관계를 확인하고 유효한 근무 구간으로 수정하세요.";
    if (code.find("DEPENDENCY") != std::string::npos || code.find("PREDECESSOR") != std::string::npos)
        return "관련 작업의 선행 참조와 관계·지연 시간을 확인하고 순환을 해소하세요.";
    if (code.find("CAPACITY") != std::string::npos || code.find("OVERLOAD") != std::string::npos)
        return "공유 용량·근무 달력·동시간대 배정률을 확인하고 배정 또는 작업 일정을 조정하세요.";
    return "관련 대상과 입력값을 확인해 수정한 뒤 다시 시도하세요.";
}
std::string formatDiagnostic(const Diagnostic &diagnostic, DiagnosticLanguage language) {
    auto result = "[" + diagnostic.code + "] " + diagnosticMessage(diagnostic, language);
    const auto append = [&](const char *name, const auto &values) {
        if (values.empty())
            return;
        result += "\n";
        result += name;
        for (const auto &value : values)
            result += " " + value;
    };
    append(language == DiagnosticLanguage::Korean ? "작업:" : "Tasks:", diagnostic.taskIds);
    append(language == DiagnosticLanguage::Korean ? "자원:" : "Resources:", diagnostic.resourceIds);
    append(language == DiagnosticLanguage::Korean ? "달력:" : "Calendars:", diagnostic.calendarIds);
    append("WBS:", diagnostic.wbsIds);
    if (!diagnostic.projectId.empty())
        result += "\nProject: " + diagnostic.projectId;
    if (!diagnostic.path.empty())
        result += "\nPath: " + diagnostic.path;
    const auto recommendation = diagnosticRecommendation(diagnostic, language);
    return recommendation.empty() ? result : result + "\n" + recommendation;
}
std::string errorText(const std::exception &error, DiagnosticLanguage language) {
    return formatDiagnostic(diagnosticFromException(error), language);
}
} // namespace jobs
