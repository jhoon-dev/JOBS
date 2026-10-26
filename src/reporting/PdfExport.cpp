#include "reporting/ReportExport.h"
#include "reporting/ExportFile.h"
#include <QPdfWriter>
#include <QPainter>
#include <QPageSize>
#include <QTextLayout>
#include <QTextOption>
#include <QFontMetricsF>
#include <QFontDatabase>
#include <memory>
#include <algorithm>
#include <cmath>
namespace jobs {
namespace {
struct Wrapped {
    std::unique_ptr<QTextLayout> layout;
    double height = 0;
};
Wrapped wrap(QString text, const QFont& font, double width, QPaintDevice* device, bool right = false) {
    text.replace("\r\n", "\n"); text.replace('\r', '\n'); text.replace('\n', QChar::LineSeparator);
    Wrapped w{std::make_unique<QTextLayout>(text.isEmpty() ? " " : text, font, device), 0};
    QTextOption option; option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere); option.setAlignment(right ? Qt::AlignRight : Qt::AlignLeft); w.layout->setTextOption(option);
    w.layout->beginLayout();
    while (true) { auto line = w.layout->createLine(); if (!line.isValid()) break; line.setLineWidth(std::max(1.0, width)); w.height = std::max(w.height, line.height()); }
    w.layout->endLayout(); w.height = std::ceil(w.height) + 1;
    for (int i = 0; i < w.layout->lineCount(); ++i) w.layout->lineAt(i).setPosition(QPointF(0, i * w.height));
    return w;
}
class Pages {
    QPdfWriter& writer_; QPainter& p_; const ProjectReport& r_; QFont font_;
    double y_ = 0; const double margin_ = 28;
    double width() const { return writer_.width() - margin_ * 2; }
    double bottom() const { return writer_.height() - margin_ - 22; }
    void page() {
        if (pages && !writer_.newPage()) throw std::runtime_error("PDF 새 페이지를 작성할 수 없습니다.");
        if (++pages > 10000) throw std::runtime_error("PDF 페이지 한도(10,000)를 초과했습니다.");
        p_.setPen(QColor("#17324d")); auto title = font_; title.setBold(true); title.setPointSize(13); p_.setFont(title);
        p_.drawText(QRectF(margin_, margin_ - 5, width(), 22), QFontMetricsF(title, &writer_).elidedText("JOBS 프로젝트 보고서 · " + reportText(r_.projectName), Qt::ElideRight, width()));
        auto small = font_; small.setPointSize(8); p_.setFont(small);
        p_.drawText(QRectF(margin_, margin_ + 20, width(), 18), "revision " + QString::number(r_.revision) + " / 생성 UTC " + reportText(r_.options.generatedAt) + " / 보고서 " + reportText(r_.options.reportId).left(16));
        p_.drawLine(QPointF(margin_, margin_ + 42), QPointF(margin_ + width(), margin_ + 42));
        p_.drawText(QRectF(margin_, writer_.height() - margin_ - 13, width() - 70, 20), "문서 " + reportText(r_.documentId).left(20) + " / 전체 출처는 출처와 설정 참조");
        p_.drawText(QRectF(writer_.width() - margin_ - 65, writer_.height() - margin_ - 13, 65, 20), Qt::AlignRight, QString::number(pages) + " 페이지");
        y_ = margin_ + 54; p_.setFont(font_);
    }
    void heading(const ReportTable& t, bool continued) {
        auto bold = font_; bold.setBold(true); bold.setPointSize(font_.pointSize() + 2); p_.setFont(bold); p_.setPen(QColor("#17324d"));
        p_.drawText(QRectF(margin_, y_, width(), 24), reportText(t.title) + (continued ? " (이어짐)" : "")); y_ += 26;
        auto note = wrap(reportText(t.note), font_, width(), &writer_);
        auto noteHeight = note.layout->lineCount() * note.height;
        if (noteHeight > bottom() - y_ - 60) throw std::runtime_error("PDF 설명이 페이지를 초과합니다.");
        note.layout->draw(&p_, QPointF(margin_, y_)); y_ += noteHeight + 8;
        drawHeaders(t);
    }
    void drawHeaders(const ReportTable& t) {
        std::vector<QString> headers; for (const auto& c : t.columns) headers.push_back(reportText(c.title));
        row(t, headers, true, 0);
    }
    void row(const ReportTable& t, const std::vector<QString>& values, bool header, int index) {
        constexpr double pad = 4;
        double total = 0; for (const auto& c : t.columns) total += std::max(1, c.width);
        std::vector<double> widths; std::vector<Wrapped> layouts; double lineHeight = 1; int maxLines = 1;
        auto font = font_; font.setBold(header);
        for (std::size_t c = 0; c < values.size(); ++c) {
            widths.push_back(width() * std::max(1, t.columns[c].width) / total);
            const bool numeric = !header && (std::holds_alternative<std::int64_t>(t.rows[std::size_t(index)][c].value) || std::holds_alternative<double>(t.rows[std::size_t(index)][c].value));
            auto layout = wrap(values[c], font, widths.back() - pad * 2, &writer_, numeric); lineHeight = std::max(lineHeight, layout.height); maxLines = std::max(maxLines, layout.layout->lineCount()); layouts.push_back(std::move(layout));
        }
        for (auto& l : layouts) for (int n = 0; n < l.layout->lineCount(); ++n) l.layout->lineAt(n).setPosition(QPointF(0, n * lineHeight));
        // Keep ordinary rows together; exceptionally long rows continue by line.
        const auto height = maxLines * lineHeight + pad * 2;
        if (!header && height <= 160 && height > bottom() - y_) { page(); heading(t, true); }
        int offset = 0;
        while (offset < maxLines) {
            auto available = int(std::floor((bottom() - y_ - pad * 2) / lineHeight));
            if (available < 1) {
                if (header) throw std::runtime_error("PDF 표 머리글이 페이지를 초과합니다.");
                page(); heading(t, true); available = int(std::floor((bottom() - y_ - pad * 2) / lineHeight));
                if (available < 1) throw std::runtime_error("PDF 행을 배치할 공간이 없습니다.");
            }
            const auto count = std::min(maxLines - offset, available); const auto height = count * lineHeight + pad * 2;
            double x = margin_;
            for (std::size_t c = 0; c < layouts.size(); ++c) {
                const QRectF cell(x, y_, widths[c], height); p_.fillRect(cell, header ? QColor("#17324d") : index % 2 ? QColor("#f1f5f9") : QColor(Qt::white));
                p_.setPen(header ? Qt::white : QColor("#1c2e40"));
                p_.save(); p_.setClipRect(cell.adjusted(pad, pad, -pad, -pad));
                for (int n = offset; n < std::min(offset + count, layouts[c].layout->lineCount()); ++n) layouts[c].layout->lineAt(n).draw(&p_, QPointF(x + pad, y_ + pad - offset * lineHeight));
                p_.restore(); p_.setPen(QColor("#d8e1e9")); p_.drawLine(cell.bottomLeft(), cell.bottomRight()); x += widths[c];
            }
            y_ += height; offset += count;
            if (offset < maxLines) { if (header) throw std::runtime_error("PDF 머리글이 너무 깁니다."); page(); heading(t, true); }
        }
    }
public:
    int pages = 0;
    Pages(QPdfWriter& w, QPainter& painter, const ProjectReport& r, QFont font) : writer_(w), p_(painter), r_(r), font_(std::move(font)) {}
    void table(const ReportTable& t) {
        if (t.columns.empty()) throw std::runtime_error("PDF 보고서에 열이 없습니다.");
        if (!pages || bottom() - y_ < 160) page(); else y_ += 15;
        heading(t, false);
        if (t.rows.empty()) { p_.setFont(font_); p_.setPen(QColor("#526477")); p_.drawText(QRectF(margin_ + 4, y_ + 6, width() - 8, 22), "해당 행 없음"); y_ += 30; }
        for (std::size_t i = 0; i < t.rows.size(); ++i) {
            if (t.rows[i].size() != t.columns.size()) throw std::runtime_error("PDF 보고서 열 개수가 일치하지 않습니다.");
            std::vector<QString> values; for (const auto& c : t.rows[i]) values.push_back(displayCell(c, r_.currency)); row(t, values, false, int(i));
        }
    }
    void chart() { page(); paintCostTrend(p_, QRectF(margin_, y_, width(), bottom() - y_), r_, font_); y_ = bottom(); }
};
}
int exportPdf(const ProjectReport& r, const QString& path, ReportFamily family, const PdfOptions& options) {
    validateReportExport(r);
    if (options.fontPoints < 9 || options.fontPoints > 12) throw std::runtime_error("PDF 글자 크기는 9~12pt입니다.");
    ExportFile file(path, "pdf"); int pages = 0;
    {
        QPdfWriter writer(&file.file); writer.setResolution(72); writer.setPageSize(QPageSize(options.a3 ? QPageSize::A3 : QPageSize::A4)); writer.setPageOrientation(options.landscape ? QPageLayout::Landscape : QPageLayout::Portrait); writer.setPageMargins(QMarginsF(0, 0, 0, 0)); writer.setCreator("JOBS"); writer.setTitle(reportText(r.projectName) + " 프로젝트 보고서");
        QPainter painter(&writer); if (!painter.isActive()) throw std::runtime_error("PDF 렌더러를 시작할 수 없습니다.");
        auto font = options.font;
#ifdef Q_OS_MACOS
        // CoreText's private UI fonts can emit a broken PDF ToUnicode map
        // (the time colon becomes U+0013). Use an installed public print face;
        // Qt still supplies the native Korean fallback for missing glyphs.
        if(font.family().isEmpty()||font.family().startsWith('.')) {
            const auto families=QFontDatabase::families();bool found=false;
            for(const auto& family:QStringList{"Arial","Helvetica","Noto Sans"})if(families.contains(family)){font.setFamily(family);found=true;break;}
            if(!found)throw std::runtime_error("PDF 출력용 Arial/Helvetica/Noto Sans 글꼴을 찾지 못했습니다.");
        }
#endif
        font.setPointSize(options.fontPoints); painter.setFont(font); Pages renderer(writer, painter, r, font);
        for (const auto& t : r.tables) if (includeTable(t, family)) renderer.table(t);
        if ((family == ReportFamily::All || family == ReportFamily::Costs) && !r.costTrend.empty()) renderer.chart();
        pages = renderer.pages; if (!pages || !painter.end()) throw std::runtime_error("PDF를 완료할 수 없습니다.");
    }
    file.commit(); return pages;
}
}
