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
        throw std::runtime_error(
            "기간은 0~5,000,000 정수 근무분으로 환산되는 시간(예: 8, 1.5) 또는 분(1m)입니다. 빈칸은 미정입니다.");
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
            throw std::runtime_error("선행 입력은 프로젝트 원본 행 번호입니다: 1, 2SS+2h, 3FF-30m (d=480 근무분).");
        const auto &predecessor = p.tasks[std::size_t(number - 1)];
        if (predecessor.id == taskId)
            throw std::runtime_error("자기 자신을 선행 작업으로 지정할 수 없습니다.");
        const auto lag = match.captured(3).toLower();
        double minutes = 0;
        if (!lag.isEmpty()) {
            minutes = lag.chopped(1).toDouble(&okay) * (lag.endsWith('d') ? 480 : lag.endsWith('h') ? 60 : 1);
            if (!okay || !std::isfinite(minutes) || std::abs(minutes) > 5000000 ||
                std::abs(minutes - std::round(minutes)) > 0.000001)
                throw std::runtime_error("지연/선행은 ±5,000,000 정수 근무분 이내입니다.");
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
        throw std::runtime_error("붙여넣기 텍스트는 최대 4 MiB입니다.");
    if (text.isEmpty())
        throw std::runtime_error("붙여넣을 텍스트가 없습니다.");
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
            throw std::runtime_error("붙여넣기는 최대 30,000개 입력 셀입니다.");
    };
    auto finishRow = [&] {
        finishField();
        if (rows.size() >= 10000)
            throw std::runtime_error("붙여넣기는 최대 10,000행입니다.");
        if (!rows.isEmpty() && row.size() != rows.front().size())
            throw std::runtime_error("붙여넣기 " + std::to_string(rows.size() + 1) + "행의 열 수가 다릅니다.");
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
            throw std::runtime_error("닫힌 인용부호 뒤에 잘못된 문자가 있습니다.");
        if (c == '"' && !started) {
            quoted = true;
            started = true;
        } else {
            field += c;
            started = true;
        }
    }
    if (quoted)
        throw std::runtime_error("닫히지 않은 TSV 인용부호가 있습니다.");
    if (started || closed || !field.isEmpty() || !row.isEmpty())
        finishRow();
    return rows;
}
} // namespace jobs
