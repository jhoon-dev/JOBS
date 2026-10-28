#pragma once
#include "domain/ErrorCodes.h"
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace jobs {
struct ErrorContext {
    std::vector<std::string> taskIds = {}, resourceIds = {}, calendarIds = {}, wbsIds = {};
    std::string projectId = {}, path = {};
};
struct Diagnostic {
    std::string code;
    std::vector<std::string> arguments;
    std::vector<std::string> taskIds;
    std::string textKey = {};
    std::vector<std::string> resourceIds = {}, calendarIds = {}, wbsIds = {};
    std::string projectId = {}, path = {};
    std::vector<Diagnostic> causes = {};
};
class AppError : public std::runtime_error {
public:
    explicit AppError(ErrorCode code, std::vector<std::string> arguments = {}, ErrorContext context = {},
                      std::string textKey = {});
    explicit AppError(Diagnostic diagnostic);
    const Diagnostic &diagnostic() const noexcept {
        return diagnostic_;
    }
    const std::string &code() const noexcept {
        return diagnostic_.code;
    }

private:
    Diagnostic diagnostic_;
};
Diagnostic diagnosticFromException(const std::exception &, std::string wrapperCode = {},
                                   std::vector<std::string> tasks = {});
inline std::string diagnosticArgument(const std::string &value) {
    return value;
}
inline std::string diagnosticArgument(std::string_view value) {
    return std::string(value);
}
inline std::string diagnosticArgument(const char *value) {
    return value ? value : "";
}
template <class T>
    requires requires(const T &value) { std::to_string(value); }
std::string diagnosticArgument(const T &value) {
    return std::to_string(value);
}
} // namespace jobs
