#pragma once
#include "calculation/Reports.h"
#include <QFont>
#include <QString>
#include <QRectF>
class QPainter;
namespace jobs {
struct PdfOptions { bool a3 = false, landscape = true; int fontPoints = 9; QFont font; };
QString reportText(const std::string&);
void validateReportExport(const ProjectReport&);
QString displayCell(const ReportCell&, const std::string& currency);
QString numericCell(const ReportCell&, const std::string& currency);
void paintCostTrend(QPainter&, const QRectF&, const ProjectReport&, const QFont&);
void exportXlsx(const ProjectReport&, const QString& path, ReportFamily family = ReportFamily::All);
int exportPdf(const ProjectReport&, const QString& path, ReportFamily family = ReportFamily::All, const PdfOptions& = {});
}
