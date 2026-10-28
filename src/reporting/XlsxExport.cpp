#include "reporting/ReportExport.h"
#include "reporting/ExportFile.h"
#include <QXmlStreamWriter>
#include <QDataStream>
#include <functional>
#include <algorithm>
namespace jobs {
namespace {
constexpr auto mainNs = "http://schemas.openxmlformats.org/spreadsheetml/2006/main";
constexpr auto relNs = "http://schemas.openxmlformats.org/officeDocument/2006/relationships";
QByteArray xml(const std::function<void(QXmlStreamWriter &)> &write) {
    QByteArray bytes;
    QXmlStreamWriter x(&bytes);
    x.writeStartDocument();
    write(x);
    x.writeEndDocument();
    if (x.hasError())
        throw std::runtime_error("XML 보고서를 작성할 수 없습니다.");
    return bytes;
}
std::uint32_t crc32(const QByteArray &bytes) {
    std::uint32_t crc = 0xffffffff;
    for (auto c : bytes) {
        crc ^= static_cast<unsigned char>(c);
        for (int n = 0; n < 8; ++n)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1)));
    }
    return ~crc;
}
class Zip {
    struct Entry {
        QByteArray name;
        std::uint32_t crc, size, offset;
    };
    QIODevice &device_;
    QDataStream stream_;
    std::vector<Entry> entries_;
    void raw(const QByteArray &b) {
        if (device_.write(b) != b.size())
            throw std::runtime_error("ZIP 파일 쓰기에 실패했습니다.");
    }

public:
    explicit Zip(QIODevice &d) : device_(d), stream_(&d) {
        stream_.setByteOrder(QDataStream::LittleEndian);
    }
    void add(const QByteArray &name, const QByteArray &bytes) {
        if (device_.pos() + bytes.size() > 64 * 1024 * 1024)
            throw std::runtime_error("엑셀 보고서 파일 한도(64 MiB)를 초과했습니다.");
        Entry e{name, crc32(bytes), std::uint32_t(bytes.size()), std::uint32_t(device_.pos())};
        stream_ << quint32(0x04034b50) << quint16(20) << quint16(0x800) << quint16(0) << quint16(0) << quint16(33)
                << quint32(e.crc) << quint32(e.size) << quint32(e.size) << quint16(name.size()) << quint16(0);
        raw(name);
        raw(bytes);
        entries_.push_back(std::move(e));
    }
    void finish() {
        const auto at = quint32(device_.pos());
        for (const auto &e : entries_) {
            stream_ << quint32(0x02014b50) << quint16(20) << quint16(20) << quint16(0x800) << quint16(0) << quint16(0)
                    << quint16(33) << quint32(e.crc) << quint32(e.size) << quint32(e.size) << quint16(e.name.size())
                    << quint16(0) << quint16(0) << quint16(0) << quint16(0) << quint32(0) << quint32(e.offset);
            raw(e.name);
        }
        const auto directorySize = quint32(device_.pos() - at);
        stream_ << quint32(0x06054b50) << quint16(0) << quint16(0) << quint16(entries_.size())
                << quint16(entries_.size()) << directorySize << at << quint16(0);
        if (stream_.status() != QDataStream::Ok)
            throw std::runtime_error("ZIP 디렉터리 쓰기에 실패했습니다.");
    }
};
QString column(int n) {
    QString result;
    do {
        result.prepend(QChar('A' + n % 26));
        n = n / 26 - 1;
    } while (n >= 0);
    return result;
}
void empty(QXmlStreamWriter &x, const QString &name, const std::initializer_list<std::pair<QString, QString>> &attrs) {
    x.writeEmptyElement(name);
    for (const auto &a : attrs)
        x.writeAttribute(a.first, a.second);
}
void textCell(QXmlStreamWriter &x, const QString &ref, const QString &value, int style) {
    if (value.size() > 32767)
        throw std::runtime_error("엑셀 셀 문자 한도(32,767자)를 초과했습니다. PDF 또는 더 좁은 범위를 사용하세요.");
    x.writeStartElement("c");
    x.writeAttribute("r", ref);
    x.writeAttribute("s", QString::number(style));
    x.writeAttribute("t", "inlineStr");
    x.writeStartElement("is");
    x.writeStartElement("t");
    x.writeAttribute("xml:space", "preserve");
    x.writeCharacters(value);
    x.writeEndElement();
    x.writeEndElement();
    x.writeEndElement();
}
int style(const ReportCell &c) {
    if (std::holds_alternative<std::monostate>(c.value))
        return 9;
    if (std::holds_alternative<std::string>(c.value))
        return 0;
    if (c.format == CellFormat::Hours || c.format == CellFormat::WorkHours)
        return 5;
    if (c.format == CellFormat::Money)
        return 6;
    if (c.format == CellFormat::Units || c.format == CellFormat::PhysicalPercent)
        return 7;
    if (c.format == CellFormat::Ratio)
        return 8;
    return 4;
}
double rowHeight(const std::vector<QString> &values, const std::vector<int> &widths, double minimum) {
    int lines = 1;
    for (std::size_t c = 0; c < values.size(); ++c) {
        int count = 0;
        for (auto line : values[c].split('\n')) {
            int units = 0;
            for (auto ch : line)
                units += ch.unicode() > 255 ? 2 : 1;
            count += std::max(1, (units + widths[c] - 1) / widths[c]);
        }
        lines = std::max(lines, count);
    }
    const auto h = std::max(minimum, lines * 15.0 + 8);
    if (h > 409)
        throw std::runtime_error(
            "엑셀 행 표시 한도를 초과하는 긴 내용이 있습니다. PDF 내보내기를 사용하거나 범위를 좁히세요.");
    return h;
}
QByteArray sheet(const ProjectReport &r, const ReportTable &t) {
    if (t.rows.size() > 1048570 || t.columns.size() > 16384 || t.columns.empty())
        throw std::runtime_error("엑셀 시트 행/열 한도를 초과했습니다.");
    return xml([&](auto &x) {
        x.writeStartElement("worksheet");
        x.writeDefaultNamespace(mainNs);
        x.writeStartElement("sheetPr");
        empty(x, "pageSetUpPr", {{"fitToPage", "1"}});
        x.writeEndElement();
        empty(x, "dimension",
              {{"ref", "A1:" + column(int(t.columns.size()) - 1) + QString::number(t.rows.size() + 6)}});
        x.writeStartElement("sheetViews");
        x.writeStartElement("sheetView");
        x.writeAttribute("workbookViewId", "0");
        x.writeAttribute("showGridLines", "0");
        empty(x, "pane", {{"ySplit", "6"}, {"topLeftCell", "A7"}, {"activePane", "bottomLeft"}, {"state", "frozen"}});
        x.writeEndElement();
        x.writeEndElement();
        x.writeStartElement("cols");
        int totalWidth = 0;
        for (std::size_t c = 0; c < t.columns.size(); ++c) {
            totalWidth += t.columns[c].width;
            empty(x, "col",
                  {{"min", QString::number(c + 1)},
                   {"max", QString::number(c + 1)},
                   {"width", QString::number(t.columns[c].width)},
                   {"customWidth", "1"}});
        }
        x.writeEndElement();
        x.writeStartElement("sheetData");
        const QStringList preamble{reportText(t.title), reportText(r.projectName),
                                   "문서 " + reportText(r.documentId) + " / revision " + QString::number(r.revision) +
                                       " / UTC " + reportText(r.options.generatedAt),
                                   "범위 " + reportText(r.scopeName) + " / 상태일 " +
                                       reportText(r.statusAt.value_or("미설정")) + " / 통화 " + reportText(r.currency),
                                   reportText(t.note)};
        for (int row = 1; row <= 5; ++row) {
            x.writeStartElement("row");
            x.writeAttribute("r", QString::number(row));
            x.writeAttribute("ht", QString::number(rowHeight({preamble[row - 1]}, {totalWidth}, row == 1 ? 32 : 24)));
            x.writeAttribute("customHeight", "1");
            textCell(x, "A" + QString::number(row), preamble[row - 1], row == 1 ? 1 : 2);
            x.writeEndElement();
        }
        std::vector<int> widths;
        std::vector<QString> headers;
        for (const auto &c : t.columns) {
            widths.push_back(c.width);
            headers.push_back(reportText(c.title));
        }
        x.writeStartElement("row");
        x.writeAttribute("r", "6");
        x.writeAttribute("ht", QString::number(rowHeight(headers, widths, 32)));
        x.writeAttribute("customHeight", "1");
        for (int c = 0; c < int(headers.size()); ++c)
            textCell(x, column(c) + "6", headers[std::size_t(c)], 3);
        x.writeEndElement();
        for (std::size_t i = 0; i < t.rows.size(); ++i) {
            const auto &row = t.rows[i];
            std::vector<QString> displayed;
            for (const auto &c : row)
                displayed.push_back(displayCell(c, r.currency));
            x.writeStartElement("row");
            x.writeAttribute("r", QString::number(i + 7));
            x.writeAttribute("ht", QString::number(rowHeight(displayed, widths, 25)));
            x.writeAttribute("customHeight", "1");
            for (int c = 0; c < int(row.size()); ++c) {
                auto ref = column(c) + QString::number(i + 7);
                auto n = numericCell(row[std::size_t(c)], r.currency);
                if (n.isEmpty())
                    textCell(x, ref, displayed[std::size_t(c)], style(row[std::size_t(c)]));
                else {
                    x.writeStartElement("c");
                    x.writeAttribute("r", ref);
                    x.writeAttribute("s", QString::number(style(row[std::size_t(c)])));
                    x.writeTextElement("v", n);
                    x.writeEndElement();
                }
            }
            x.writeEndElement();
        }
        x.writeEndElement();
        empty(x, "autoFilter",
              {{"ref", "A6:" + column(int(t.columns.size()) - 1) +
                           QString::number(std::max<std::size_t>(6, t.rows.size() + 6))}});
        x.writeStartElement("mergeCells");
        x.writeAttribute("count", "5");
        for (int i = 1; i <= 5; ++i)
            empty(x, "mergeCell",
                  {{"ref", "A" + QString::number(i) + ":" + column(int(t.columns.size()) - 1) + QString::number(i)}});
        x.writeEndElement();
        empty(x, "printOptions", {{"horizontalCentered", "0"}});
        empty(x, "pageMargins",
              {{"left", "0.3"},
               {"right", "0.3"},
               {"top", "0.4"},
               {"bottom", "0.4"},
               {"header", "0.2"},
               {"footer", "0.2"}});
        empty(x, "pageSetup",
              {{"paperSize", "9"}, {"orientation", "landscape"}, {"fitToWidth", "1"}, {"fitToHeight", "0"}});
        x.writeEndElement();
    });
}
QByteArray styles(const ProjectReport &r) {
    return xml([&](auto &x) {
        x.writeStartElement("styleSheet");
        x.writeDefaultNamespace(mainNs);
        x.writeStartElement("numFmts");
        x.writeAttribute("count", "4");
        empty(x, "numFmt", {{"numFmtId", "164"}, {"formatCode", "0.00"}});
        empty(
            x, "numFmt",
            {{"numFmtId", "165"},
             {"formatCode", currencyDecimals(r.currency) ? "#,##0.00;[Red](#,##0.00);0.00" : "#,##0;[Red](#,##0);0"}});
        empty(x, "numFmt", {{"numFmtId", "166"}, {"formatCode", "0.0%"}});
        empty(x, "numFmt", {{"numFmtId", "167"}, {"formatCode", "0.0000"}});
        x.writeEndElement();
        x.writeStartElement("fonts");
        x.writeAttribute("count", "4");
        for (int i = 0; i < 4; ++i) {
            x.writeStartElement("font");
            if (i == 1 || i == 3)
                x.writeEmptyElement("b");
            empty(x, "sz", {{"val", i == 1 ? "20" : i == 2 ? "10" : "11"}});
            empty(x, "color", {{"rgb", i == 3 ? "FFFFFFFF" : i == 2 ? "FF526477" : "FF17324D"}});
            empty(x, "name", {{"val", "Calibri"}});
            x.writeEndElement();
        }
        x.writeEndElement();
        x.writeStartElement("fills");
        x.writeAttribute("count", "4");
        for (const auto &color : QStringList{"none", "gray125", "17324D", "FFF1CC"}) {
            x.writeStartElement("fill");
            x.writeStartElement("patternFill");
            x.writeAttribute("patternType", color.size() == 6 ? "solid" : color);
            if (color.size() == 6)
                empty(x, "fgColor", {{"rgb", "FF" + color}});
            x.writeEndElement();
            x.writeEndElement();
        }
        x.writeEndElement();
        x.writeStartElement("borders");
        x.writeAttribute("count", "1");
        x.writeStartElement("border");
        for (const auto &b : {"left", "right", "top", "bottom", "diagonal"})
            x.writeEmptyElement(b);
        x.writeEndElement();
        x.writeEndElement();
        x.writeStartElement("cellStyleXfs");
        x.writeAttribute("count", "1");
        empty(x, "xf", {{"numFmtId", "0"}, {"fontId", "0"}, {"fillId", "0"}, {"borderId", "0"}});
        x.writeEndElement();
        x.writeStartElement("cellXfs");
        x.writeAttribute("count", "10");
        for (int i = 0; i < 10; ++i) {
            x.writeStartElement("xf");
            x.writeAttribute("fontId", QString::number(i == 1 ? 1 : i == 2 ? 2 : i == 3 ? 3 : 0));
            x.writeAttribute("fillId", i == 3 ? "2" : i == 9 ? "3" : "0");
            x.writeAttribute("borderId", "0");
            x.writeAttribute("xfId", "0");
            x.writeAttribute("numFmtId", QString::number(i == 4 ? 3 : i >= 5 && i <= 8 ? i + 159 : 0));
            x.writeAttribute("applyFont", "1");
            x.writeAttribute("applyFill", "1");
            x.writeAttribute("applyAlignment", "1");
            x.writeAttribute("applyNumberFormat", "1");
            empty(x, "alignment", {{"vertical", "top"}, {"wrapText", "1"}, {"horizontal", i >= 4 ? "right" : "left"}});
            x.writeEndElement();
        }
        x.writeEndElement();
        x.writeStartElement("cellStyles");
        x.writeAttribute("count", "1");
        empty(x, "cellStyle", {{"name", "Normal"}, {"xfId", "0"}, {"builtinId", "0"}});
        x.writeEndElement();
        x.writeEndElement();
    });
}
} // namespace
void exportXlsx(const ProjectReport &r, const QString &path, ReportFamily family) {
    validateReportExport(r);
    std::vector<const ReportTable *> tables;
    for (const auto &t : r.tables)
        if (includeTable(t, family))
            tables.push_back(&t);
    if (tables.empty() || tables.size() > 100)
        throw std::runtime_error("내보낼 보고서가 없거나 시트 수 한도 초과입니다.");
    ExportFile file(path, "xlsx");
    Zip zip(file.file);
    zip.add(
        "[Content_Types].xml", xml([&](auto &x) {
            x.writeStartElement("Types");
            x.writeDefaultNamespace("http://schemas.openxmlformats.org/package/2006/content-types");
            empty(x, "Default",
                  {{"Extension", "rels"}, {"ContentType", "application/vnd.openxmlformats-package.relationships+xml"}});
            empty(x, "Default", {{"Extension", "xml"}, {"ContentType", "application/xml"}});
            empty(x, "Override",
                  {{"PartName", "/xl/workbook.xml"},
                   {"ContentType", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet.main+xml"}});
            empty(x, "Override",
                  {{"PartName", "/xl/styles.xml"},
                   {"ContentType", "application/vnd.openxmlformats-officedocument.spreadsheetml.styles+xml"}});
            empty(x, "Override",
                  {{"PartName", "/docProps/core.xml"},
                   {"ContentType", "application/vnd.openxmlformats-package.core-properties+xml"}});
            for (std::size_t i = 0; i < tables.size(); ++i)
                empty(x, "Override",
                      {{"PartName", "/xl/worksheets/sheet" + QString::number(i + 1) + ".xml"},
                       {"ContentType", "application/vnd.openxmlformats-officedocument.spreadsheetml.worksheet+xml"}});
            x.writeEndElement();
        }));
    zip.add("_rels/.rels", xml([&](auto &x) {
                x.writeStartElement("Relationships");
                x.writeDefaultNamespace("http://schemas.openxmlformats.org/package/2006/relationships");
                empty(x, "Relationship",
                      {{"Id", "rId1"}, {"Type", QString(relNs) + "/officeDocument"}, {"Target", "xl/workbook.xml"}});
                empty(
                    x, "Relationship",
                    {{"Id", "rId2"},
                     {"Type", "http://schemas.openxmlformats.org/package/2006/relationships/metadata/core-properties"},
                     {"Target", "docProps/core.xml"}});
                x.writeEndElement();
            }));
    zip.add("docProps/core.xml", xml([&](auto &x) {
                x.writeStartElement("cp:coreProperties");
                x.writeNamespace("http://schemas.openxmlformats.org/package/2006/metadata/core-properties", "cp");
                x.writeNamespace("http://purl.org/dc/elements/1.1/", "dc");
                x.writeNamespace("http://purl.org/dc/terms/", "dcterms");
                x.writeNamespace("http://www.w3.org/2001/XMLSchema-instance", "xsi");
                x.writeTextElement("dc:title", reportText(r.projectName));
                x.writeTextElement("dc:creator", "JOBS");
                x.writeTextElement("dc:description", "보고서 " + reportText(r.options.reportId) + " / 문서 " +
                                                         reportText(r.documentId) + " / rev " +
                                                         QString::number(r.revision));
                x.writeStartElement("dcterms:created");
                x.writeAttribute("xsi:type", "dcterms:W3CDTF");
                x.writeCharacters(reportText(r.options.generatedAt));
                x.writeEndElement();
                x.writeEndElement();
            }));
    zip.add(
        "xl/workbook.xml", xml([&](auto &x) {
            x.writeStartElement("workbook");
            x.writeDefaultNamespace(mainNs);
            x.writeNamespace(relNs, "r");
            x.writeStartElement("bookViews");
            empty(x, "workbookView", {{"activeTab", "0"}});
            x.writeEndElement();
            x.writeStartElement("sheets");
            for (std::size_t i = 0; i < tables.size(); ++i) {
                auto title = reportText(tables[i]->title);
                if (title.size() > 31)
                    throw std::runtime_error("엑셀 시트 이름 한도 초과입니다.");
                empty(x, "sheet",
                      {{"name", title}, {"sheetId", QString::number(i + 1)}, {"r:id", "rId" + QString::number(i + 1)}});
            }
            x.writeEndElement();
            x.writeStartElement("definedNames");
            for (std::size_t i = 0; i < tables.size(); ++i) {
                x.writeStartElement("definedName");
                x.writeAttribute("name", "_xlnm.Print_Titles");
                x.writeAttribute("localSheetId", QString::number(i));
                x.writeCharacters("'" + reportText(tables[i]->title) + "'!$1:$6");
                x.writeEndElement();
            }
            x.writeEndElement();
            x.writeEndElement();
        }));
    zip.add("xl/_rels/workbook.xml.rels", xml([&](auto &x) {
                x.writeStartElement("Relationships");
                x.writeDefaultNamespace("http://schemas.openxmlformats.org/package/2006/relationships");
                for (std::size_t i = 0; i < tables.size(); ++i)
                    empty(x, "Relationship",
                          {{"Id", "rId" + QString::number(i + 1)},
                           {"Type", QString(relNs) + "/worksheet"},
                           {"Target", "worksheets/sheet" + QString::number(i + 1) + ".xml"}});
                empty(x, "Relationship",
                      {{"Id", "styles"}, {"Type", QString(relNs) + "/styles"}, {"Target", "styles.xml"}});
                x.writeEndElement();
            }));
    zip.add("xl/styles.xml", styles(r));
    for (std::size_t i = 0; i < tables.size(); ++i)
        zip.add(("xl/worksheets/sheet" + QString::number(i + 1) + ".xml").toUtf8(), sheet(r, *tables[i]));
    zip.finish();
    file.commit();
}
} // namespace jobs
