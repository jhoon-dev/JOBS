#include "domain/Diagnostics.h"
#include <algorithm>

namespace jobs {
AppError::AppError(ErrorCode code, std::vector<std::string> arguments, ErrorContext context, std::string textKey)
    : AppError(Diagnostic{std::string(errorCodeToken(code)),
                          std::move(arguments),
                          std::move(context.taskIds),
                          std::move(textKey),
                          std::move(context.resourceIds),
                          std::move(context.calendarIds),
                          std::move(context.wbsIds),
                          std::move(context.projectId),
                          std::move(context.path),
                          {}}) {}
AppError::AppError(Diagnostic diagnostic) : std::runtime_error(diagnostic.code), diagnostic_(std::move(diagnostic)) {}
Diagnostic diagnosticFromException(const std::exception &error, std::string wrapperCode,
                                   std::vector<std::string> tasks) {
    const auto *typed = dynamic_cast<const AppError *>(&error);
    Diagnostic cause = typed ? typed->diagnostic() : Diagnostic{"INTERNAL_ERROR", {error.what()}, {}};
    if (wrapperCode.empty())
        return cause;
    Diagnostic result{std::move(wrapperCode), {}, std::move(tasks)};
    for (const auto &id : cause.taskIds)
        if (std::find(result.taskIds.begin(), result.taskIds.end(), id) == result.taskIds.end())
            result.taskIds.push_back(id);
    result.resourceIds = cause.resourceIds;
    result.calendarIds = cause.calendarIds;
    result.wbsIds = cause.wbsIds;
    result.projectId = cause.projectId;
    result.path = cause.path;
    result.causes.push_back(std::move(cause));
    return result;
}
} // namespace jobs
