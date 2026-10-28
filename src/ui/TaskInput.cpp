#include "formatting/QtDiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "ui/TaskInput.h"
#include <QRegularExpression>
#include "domain/CalculationControl.h"
#include <cmath>
#include <stdexcept>
namespace jobs {
std::optional<std::int64_t> taskDurationInput(const QString &value) {
    const auto text = value.trimmed();
    if (text.isEmpty())
        return {};
    bool okay = false;
    double minutes = 0;
    static thread_local const QRegularExpression minutePattern("^[0-9]+m$"), hourPattern("^[0-9]+(?:\\.[0-9]{1,3})?$");
    if (minutePattern.match(text).hasMatch())
        minutes = text.chopped(1).toDouble(&okay);
    else if (hourPattern.match(text).hasMatch())
        minutes = text.toDouble(&okay) * 60;
    if (!okay || !std::isfinite(minutes) || minutes < 0 || minutes > 5000000 ||
        std::abs(minutes - std::round(minutes)) > 0.000001)
        throw AppError(ErrorCode::TASKINPUT_001, {});
    return std::int64_t(std::round(minutes));
}
std::vector<Dependency> taskDependencyInput(const QString &value, const Project &p, const std::string &taskId) {
    std::vector<Dependency> edges;
    static thread_local const QRegularExpression separator("[\\s,]+"),
        relation("^([0-9]+)(FS|SS|FF|SF)?([+-][0-9]+(?:\\.[0-9]+)?[mhd])?$", QRegularExpression::CaseInsensitiveOption);
    for (const auto &token : value.trimmed().split(separator, Qt::SkipEmptyParts)) {
        calculationCheckpoint();
        const auto match = relation.match(token);
        bool okay = false;
        const auto number = match.captured(1).toULongLong(&okay);
        if (!match.hasMatch() || !okay || number < 1 || number > p.tasks.size())
            throw AppError(ErrorCode::TASKINPUT_002, {});
        const auto &predecessor = p.tasks[std::size_t(number - 1)];
        if (predecessor.id == taskId)
            throw AppError(ErrorCode::TASKINPUT_003, {});
        const auto lag = match.captured(3).toLower();
        double minutes = 0;
        if (!lag.isEmpty()) {
            minutes = lag.chopped(1).toDouble(&okay) * (lag.endsWith('d') ? 480 : lag.endsWith('h') ? 60 : 1);
            if (!okay || !std::isfinite(minutes) || std::abs(minutes) > 5000000 ||
                std::abs(minutes - std::round(minutes)) > 0.000001)
                throw AppError(ErrorCode::TASKINPUT_004, {});
        }
        edges.push_back({predecessor.id,
                         match.captured(2).isEmpty()
                             ? DependencyType::FS
                             : dependencyTypeFromToken(match.captured(2).toUpper().toUtf8().toStdString()),
                         std::int64_t(std::round(minutes))});
    }
    return edges;
}
QString encodeTaskTsv(const QList<QStringList> &rows) {
    QStringList lines;
    for (const auto &row : rows) {
        QStringList fields;
        for (auto field : row) {
            if (field.contains('"') || field.contains('\t') || field.contains('\n') || field.contains('\r')) {
                field.replace("\"", "\"\"");
                field = '"' + field + '"';
            }
            fields << field;
        }
        lines << fields.join('\t');
    }
    return lines.join('\n');
}
QList<QStringList> decodeTaskTsv(const QString &text) {
    constexpr qsizetype limit = 4 * 1024 * 1024;
    if (text.size() > limit || text.toUtf8().size() > limit)
        throw AppError(ErrorCode::TASKINPUT_005, {});
    if (text.isEmpty())
        throw AppError(ErrorCode::TASKINPUT_006, {});
    QList<QStringList> rows;
    QStringList row;
    QString field;
    bool quoted = false, closed = false, started = false;
    qsizetype cells = 0;
    auto finishField = [&] {
        row << field;
        field.clear();
        closed = false;
        started = false;
        if (++cells > 30000)
            throw AppError(ErrorCode::TASKINPUT_007, {});
    };
    auto finishRow = [&] {
        finishField();
        if (rows.size() >= 10000)
            throw AppError(ErrorCode::TASKINPUT_008, {});
        if (!rows.isEmpty() && row.size() != rows.front().size())
            throw AppError(ErrorCode::TASKINPUT_009, {diagnosticArgument(std::to_string(rows.size() + 1))});
        rows << row;
        row.clear();
    };
    for (qsizetype i = 0; i < text.size(); ++i) {
        if (i % 4096 == 0)
            calculationCheckpoint();
        const auto c = text[i];
        if (quoted) {
            if (c == '"') {
                if (i + 1 < text.size() && text[i + 1] == '"') {
                    field += '"';
                    ++i;
                } else {
                    quoted = false;
                    closed = true;
                }
            } else
                field += c;
            continue;
        }
        if (c == '\t') {
            finishField();
            continue;
        }
        if (c == '\r' || c == '\n') {
            finishRow();
            if (c == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
                ++i;
            continue;
        }
        if (closed)
            throw AppError(ErrorCode::TASKINPUT_010, {});
        if (c == '"' && !started) {
            quoted = true;
            started = true;
        } else {
            field += c;
            started = true;
        }
    }
    if (quoted)
        throw AppError(ErrorCode::TASKINPUT_011, {});
    if (started || closed || !field.isEmpty() || !row.isEmpty())
        finishRow();
    return rows;
}
} // namespace jobs
