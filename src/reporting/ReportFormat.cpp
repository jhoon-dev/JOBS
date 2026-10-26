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
QString reportText(const std::string& s) {
    auto text = QString::fromUtf8(s.data(), qsizetype(s.size()));
    if (text.toUtf8().toStdString() != s) throw std::runtime_error("보고서 원문에 올바르지 않은 UTF-8이 있습니다.");
    for (auto c : text.toUcs4()) if ((c < 32 && c != 9 && c != 10 && c != 13) || c == 0xfffe || c == 0xffff)
        throw std::runtime_error("보고서 원문에 출력할 수 없는 제어 문자가 있습니다.");
    return text;
}
QString numericCell(const ReportCell& c, const std::string& currency) {
    double n;
    if (const auto* i = std::get_if<std::int64_t>(&c.value)) {
        if (c.format == CellFormat::Money) { if (*i < -maxMoney || *i > maxMoney) throw std::runtime_error("보고서 금액 한도 초과입니다."); return moneyui::format(*i, currency); }
        n = double(*i);
        if (c.format == CellFormat::Hours) n /= 60;
        if (c.format == CellFormat::WorkHours) n /= 6000;
        if (c.format == CellFormat::Units) n /= 100;
        if (c.format == CellFormat::PhysicalPercent) n /= 10000;
    } else if (const auto* d = std::get_if<double>(&c.value)) n = *d;
    else return {};
    if (!std::isfinite(n)) throw std::runtime_error("보고서에 유한하지 않은 수치가 있습니다.");
    return QString::number(n, 'g', 17);
}
QString displayCell(const ReportCell& c, const std::string& currency) {
    if (std::holds_alternative<std::monostate>(c.value)) return "미정";
    if (const auto* s = std::get_if<std::string>(&c.value)) return reportText(*s);
    if (c.format == CellFormat::Money && std::holds_alternative<std::int64_t>(c.value)) return numericCell(c, currency);
    auto raw = numericCell(c, currency);
    if (c.format == CellFormat::Units || c.format == CellFormat::PhysicalPercent) return QString::number(raw.toDouble() * 100, 'f', 1) + "%";
    if (c.format == CellFormat::Hours || c.format == CellFormat::WorkHours) return QString::number(raw.toDouble(), 'f', 2);
    if (c.format == CellFormat::Ratio) return QString::number(raw.toDouble(), 'f', 4);
    return raw;
}
void validateReportExport(const ProjectReport& r) {
    if (r.tables.empty() || r.tables.size() > 100 || r.documentId.empty() || r.options.reportId.empty() || !validUtcStamp(r.options.generatedAt)) throw std::runtime_error("보고서 출처/생성 시각/시트 수가 잘못되었습니다.");
    currencyDecimals(r.currency); reportText(r.documentId); reportText(r.options.reportId); reportText(r.projectName); reportText(r.scopeName);
    std::set<QString> titles; std::size_t cells = 0;
    for (const auto& t : r.tables) {
        auto title = reportText(t.title); reportText(t.note);
        if (title.isEmpty() || title.size() > 31 || title.contains(QRegularExpression("[\\[\\]:*?/\\\\]")) || title.startsWith('\'') || title.endsWith('\'') || !titles.insert(title.toCaseFolded()).second || t.columns.empty() || t.columns.size() > 16384) throw std::runtime_error("보고서 시트 이름/열이 잘못되었습니다.");
        for (const auto& col : t.columns) { reportText(col.title); if (col.width < 1 || col.width > 255) throw std::runtime_error("보고서 열 너비가 잘못되었습니다."); }
        for (const auto& row : t.rows) { if (row.size() != t.columns.size()) throw std::runtime_error("보고서 열 개수가 일치하지 않습니다."); for (const auto& c : row) displayCell(c, r.currency); cells += row.size(); if (cells > 1000000) throw std::runtime_error("보고서 100만 셀 한도 초과입니다."); }
    }
}
void paintCostTrend(QPainter& p, const QRectF& area, const ProjectReport& r, const QFont& font) {
    p.save(); p.setFont(font); p.fillRect(area, Qt::white); p.setPen(QColor("#24384c"));
    p.drawText(QRectF(area.left() + 8, area.top() + 4, area.width() - 16, 24), "기준선 코호트 PV / EV / AC 누적 (" + reportText(r.currency) + ")");
    if (r.costTrend.empty()) { p.drawText(area, Qt::AlignCenter, "기간 데이터 없음"); p.restore(); return; }
    const QRectF plot(area.left() + 100, area.top() + 52, std::max(1.0, area.width() - 125), std::max(1.0, area.height() - 90));
    std::int64_t maximum = 1; for (const auto& d : r.costTrend) for (auto n : {d.pv, d.ev, d.ac}) if (n) maximum = std::max(maximum, *n);
    const auto first = QDate::fromString(reportText(r.costTrend.front().date), Qt::ISODate), last = QDate::fromString(reportText(r.costTrend.back().date), Qt::ISODate);
    const auto range = std::max<qint64>(1, first.daysTo(last));
    p.drawLine(plot.bottomLeft(), plot.topLeft()); p.drawLine(plot.bottomLeft(), plot.bottomRight());
    p.drawText(QRectF(area.left(), plot.top() - 8, 94, 20), Qt::AlignRight, moneyui::format(maximum, r.currency)); p.drawText(QRectF(area.left(), plot.bottom() - 10, 94, 20), Qt::AlignRight, "0");
    const QColor colors[]{QColor("#2578bc"), QColor("#218156"), QColor("#b5680c")}; const QString labels[]{"PV", "EV", "AC"};
    for (int s = 0; s < 3; ++s) {
        p.setPen(QPen(colors[s], 1.7)); p.drawText(QRectF(plot.left() + s * 55, area.top() + 27, 52, 22), labels[s]);
        QPainterPath line; bool connected = false;
        for (const auto& d : r.costTrend) {
            auto n = s == 0 ? d.pv : s == 1 ? d.ev : d.ac; if (!n) { connected = false; continue; }
            QPointF at(plot.left() + double(first.daysTo(QDate::fromString(reportText(d.date), Qt::ISODate))) / range * plot.width(), plot.bottom() - double(*n) / maximum * plot.height());
            if (connected) line.lineTo(at); else line.moveTo(at); connected = true; p.drawEllipse(at, 1.8, 1.8);
        }
        p.drawPath(line);
    }
    p.setPen(QColor("#24384c")); p.drawText(QRectF(plot.left(), plot.bottom() + 6, 140, 22), reportText(r.costTrend.front().date));
    p.drawText(QRectF(plot.right() - 140, plot.bottom() + 6, 140, 22), Qt::AlignRight, reportText(r.costTrend.back().date)); p.restore();
}
}
