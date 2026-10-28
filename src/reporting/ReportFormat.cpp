#include "formatting/QtDiagnosticText.h"
#include "reporting/ReportExport.h"
#include "formatting/Money.h"
#include <QPainter>
#include <QPainterPath>
#include <QDate>
#include <QRegularExpression>
#include <cmath>
#include <stdexcept>
#include <algorithm>
#include <set>
namespace jobs {
QString reportText(const std::string &s) {
    auto text = QString::fromUtf8(s.data(), qsizetype(s.size()));
    if (text.toUtf8().toStdString() != s)
        throw AppError(ErrorCode::REPORTFORMAT_001, {});
    for (auto c : text.toUcs4())
        if ((c < 32 && c != 9 && c != 10 && c != 13) || c == 0xfffe || c == 0xffff)
            throw AppError(ErrorCode::REPORTFORMAT_002, {});
    return text;
}
QString numericCell(const ReportCell &c, const std::string &currency) {
    double n;
    if (const auto *i = std::get_if<std::int64_t>(&c.value)) {
        if (c.format == CellFormat::Money) {
            if (*i < -maxMoney || *i > maxMoney)
                throw AppError(ErrorCode::REPORTFORMAT_003, {});
            return moneyui::format(*i, currency);
        }
        n = double(*i);
        if (c.format == CellFormat::Hours)
            n /= 60;
        if (c.format == CellFormat::WorkHours)
            n /= 6000;
        if (c.format == CellFormat::Units)
            n /= 100;
        if (c.format == CellFormat::PhysicalPercent)
            n /= 10000;
    } else if (const auto *d = std::get_if<double>(&c.value))
        n = *d;
    else
        return {};
    if (!std::isfinite(n))
        throw AppError(ErrorCode::REPORTFORMAT_004, {});
    return QString::number(n, 'g', 17);
}
QString displayCell(const ReportCell &c, const std::string &currency) {
    if (std::holds_alternative<std::monostate>(c.value))
        return "미정";
    if (const auto *s = std::get_if<std::string>(&c.value))
        return reportText(*s);
    if (c.format == CellFormat::Money && std::holds_alternative<std::int64_t>(c.value))
        return numericCell(c, currency);
    auto raw = numericCell(c, currency);
    if (c.format == CellFormat::Units || c.format == CellFormat::PhysicalPercent)
        return QString::number(raw.toDouble() * 100, 'f', 1) + "%";
    if (c.format == CellFormat::Hours || c.format == CellFormat::WorkHours)
        return QString::number(raw.toDouble(), 'f', 2);
    if (c.format == CellFormat::Ratio)
        return QString::number(raw.toDouble(), 'f', 4);
    return raw;
}
void validateReportExport(const ProjectReport &r) {
    if (r.tables.empty() || r.tables.size() > 100 || r.documentId.empty() || r.options.reportId.empty() ||
        !validUtcStamp(r.options.generatedAt))
        throw AppError(ErrorCode::REPORTFORMAT_005, {});
    currencyDecimals(r.currency);
    reportText(r.documentId);
    reportText(r.options.reportId);
    reportText(r.projectName);
    reportText(r.scopeName);
    std::set<QString> titles;
    std::size_t cells = 0;
    for (const auto &t : r.tables) {
        auto title = reportText(t.title);
        reportText(t.note);
        if (title.isEmpty() || title.size() > 31 || title.contains(QRegularExpression("[\\[\\]:*?/\\\\]")) ||
            title.startsWith('\'') || title.endsWith('\'') || !titles.insert(title.toCaseFolded()).second ||
            t.columns.empty() || t.columns.size() > 16384)
            throw AppError(ErrorCode::REPORTFORMAT_006, {});
        for (const auto &col : t.columns) {
            reportText(col.title);
            if (col.width < 1 || col.width > 255)
                throw AppError(ErrorCode::REPORTFORMAT_007, {});
        }
        for (const auto &row : t.rows) {
            if (row.size() != t.columns.size())
                throw AppError(ErrorCode::REPORT_COLUMN_MISMATCH, {});
            for (const auto &c : row)
                displayCell(c, r.currency);
            cells += row.size();
            if (cells > 1000000)
                throw AppError(ErrorCode::REPORTFORMAT_008, {});
        }
    }
}
void paintCostTrend(QPainter &p, const QRectF &area, const ProjectReport &r, const QFont &font) {
    p.save();
    p.setFont(font);
    p.fillRect(area, Qt::white);
    p.setPen(QColor("#24384c"));
    p.drawText(QRectF(area.left() + 8, area.top() + 4, area.width() - 16, 24),
               "기준선 코호트 PV / EV / AC 누적 (" + reportText(r.currency) + ")");
    if (r.costTrend.empty()) {
        p.drawText(area, Qt::AlignCenter, "기간 데이터 없음");
        p.restore();
        return;
    }
    const QRectF plot(area.left() + 100, area.top() + 52, std::max(1.0, area.width() - 125),
                      std::max(1.0, area.height() - 90));
    std::int64_t maximum = 1;
    for (const auto &d : r.costTrend)
        for (auto n : {d.pv, d.ev, d.ac})
            if (n)
                maximum = std::max(maximum, *n);
    const auto first = QDate::fromString(reportText(r.costTrend.front().date), Qt::ISODate),
               last = QDate::fromString(reportText(r.costTrend.back().date), Qt::ISODate);
    const auto range = std::max<qint64>(1, first.daysTo(last));
    p.drawLine(plot.bottomLeft(), plot.topLeft());
    p.drawLine(plot.bottomLeft(), plot.bottomRight());
    p.drawText(QRectF(area.left(), plot.top() - 8, 94, 20), Qt::AlignRight, moneyui::format(maximum, r.currency));
    p.drawText(QRectF(area.left(), plot.bottom() - 10, 94, 20), Qt::AlignRight, "0");
    const QColor colors[]{QColor("#2578bc"), QColor("#218156"), QColor("#b5680c")};
    const QString labels[]{"PV", "EV", "AC"};
    for (int s = 0; s < 3; ++s) {
        p.setPen(QPen(colors[s], 1.7));
        p.drawText(QRectF(plot.left() + s * 55, area.top() + 27, 52, 22), labels[s]);
        QPainterPath line;
        bool connected = false;
        for (const auto &d : r.costTrend) {
            auto n = s == 0 ? d.pv : s == 1 ? d.ev : d.ac;
            if (!n) {
                connected = false;
                continue;
            }
            QPointF at(plot.left() + double(first.daysTo(QDate::fromString(reportText(d.date), Qt::ISODate))) / range *
                                         plot.width(),
                       plot.bottom() - double(*n) / maximum * plot.height());
            if (connected)
                line.lineTo(at);
            else
                line.moveTo(at);
            connected = true;
            p.drawEllipse(at, 1.8, 1.8);
        }
        p.drawPath(line);
    }
    p.setPen(QColor("#24384c"));
    p.drawText(QRectF(plot.left(), plot.bottom() + 6, 140, 22), reportText(r.costTrend.front().date));
    p.drawText(QRectF(plot.right() - 140, plot.bottom() + 6, 140, 22), Qt::AlignRight,
               reportText(r.costTrend.back().date));
    p.restore();
}
} // namespace jobs
