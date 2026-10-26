#include "ui/ReportDialog.h"
#include <QApplication>
#include <QAbstractTableModel>
#include <QTableView>
#include <QHeaderView>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QScrollArea>
#include <QFrame>
#include <QFileDialog>
#include <QFileInfo>
#include <QStandardPaths>
#include <QDateTime>
#include <QUuid>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <QPainter>
#include <QCloseEvent>
#include <QTimer>
#include <QRegularExpression>
namespace jobs {
namespace {
class ReportModel final : public QAbstractTableModel {
    std::shared_ptr<const ProjectReport> report_; std::size_t table_;
public:
    ReportModel(std::shared_ptr<const ProjectReport> r, std::size_t t, QObject* p) : QAbstractTableModel(p), report_(std::move(r)), table_(t) {}
    int rowCount(const QModelIndex& p = {}) const override { return p.isValid() ? 0 : int(report_->tables[table_].rows.size()); }
    int columnCount(const QModelIndex& p = {}) const override { return p.isValid() ? 0 : int(report_->tables[table_].columns.size()); }
    QVariant data(const QModelIndex& i, int role) const override {
        if (!i.isValid() || i.row() >= rowCount() || i.column() >= columnCount()) return {};
        const auto& c = report_->tables[table_].rows[std::size_t(i.row())][std::size_t(i.column())];
        if (role == Qt::DisplayRole) return displayCell(c, report_->currency);
        if (role == Qt::ToolTipRole) return "<qt>" + displayCell(c, report_->currency).toHtmlEscaped().replace('\n', "<br>") + "</qt>";
        if (role == Qt::TextAlignmentRole) return int(Qt::AlignVCenter | ((std::holds_alternative<std::int64_t>(c.value) || std::holds_alternative<double>(c.value)) ? Qt::AlignRight : Qt::AlignLeft));
        if (role == Qt::BackgroundRole && std::holds_alternative<std::monostate>(c.value)) return QColor("#fff1cc");
        return {};
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (role != Qt::DisplayRole) return {};
        return orientation == Qt::Horizontal ? QVariant(reportText(report_->tables[table_].columns[std::size_t(section)].title)) : QVariant(section + 1);
    }
};
class Trend final : public QWidget {
    std::shared_ptr<const ProjectReport> report_;
public:
    Trend(std::shared_ptr<const ProjectReport> r, QWidget* p) : QWidget(p), report_(std::move(r)) { setMinimumSize(600, 280); }
    void paintEvent(QPaintEvent*) override { QPainter painter(this); paintCostTrend(painter, rect(), *report_, font()); }
};
struct Prepared { std::shared_ptr<const ProjectReport> report; QString error; };
}
ReportDialog::ReportDialog(Document& d, const std::string& wbs, const std::string& baseline, QWidget* parent) : QDialog(parent), document_(d) {
    setObjectName("reportDialog"); setWindowTitle("프로젝트 대시보드 · 보고서 · 내보내기"); resize(1480, 900);
    auto* layout = new QVBoxLayout(this); auto* filters = new QGridLayout; layout->addLayout(filters);
    scope_ = new QComboBox; scope_->setObjectName("reportScope"); scope_->addItem("프로젝트 전체", "");
    baseline_ = new QComboBox; baseline_->setObjectName("reportBaseline"); baseline_->addItem("기준선 없음", "");
    if (d.project()) {
        for (const auto& n : d.project()->wbsNodes) scope_->addItem(QString::fromStdString(n.name), QString::fromStdString(n.id));
        for (const auto& b : d.project()->baselines) baseline_->addItem(QString("v%1 %2").arg(b.version).arg(QString::fromStdString(b.name)), QString::fromStdString(b.id));
    }
    scope_->setCurrentIndex(std::max(0, scope_->findData(QString::fromStdString(wbs))));
    baseline_->setCurrentIndex(baseline.empty() ? baseline_->count() - 1 : std::max(0, baseline_->findData(QString::fromStdString(baseline))));
    formula_ = new QComboBox; formula_->setObjectName("reportFormula"); formula_->addItem("AC + (BAC − EV)", "budget-remaining-v1"); formula_->addItem("BAC / CPI", "cpi-v1"); formula_->addItem("AC + (BAC − EV) / (CPI × SPI)", "cpi-spi-v1");
    filters->addWidget(new QLabel("WBS 범위"), 0, 0); filters->addWidget(scope_, 0, 1); filters->addWidget(new QLabel("승인 기준선"), 0, 2); filters->addWidget(baseline_, 0, 3); filters->addWidget(new QLabel("EAC 가정"), 0, 4); filters->addWidget(formula_, 0, 5);
    period_ = new QCheckBox("일별 명세 기간"); period_->setObjectName("reportPeriod"); from_ = new QDateEdit; to_ = new QDateEdit;
    from_->setObjectName("reportFrom"); to_->setObjectName("reportTo"); from_->setCalendarPopup(true); to_->setCalendarPopup(true); from_->setDisplayFormat("yyyy-MM-dd"); to_->setDisplayFormat("yyyy-MM-dd");
    auto date = d.project() ? QDate::fromString(reportText(d.project()->startDate), Qt::ISODate) : QDate::currentDate(); from_->setDate(date); to_->setDate(date.addMonths(1)); from_->setEnabled(false); to_->setEnabled(false);
    filters->addWidget(period_, 1, 0); filters->addWidget(from_, 1, 1); filters->addWidget(new QLabel("부터 / 종료일 포함"), 1, 2); filters->addWidget(to_, 1, 3);
    refresh_ = new QPushButton("재계산"); refresh_->setObjectName("refreshReport"); filters->addWidget(refresh_, 1, 5);
    auto* hint = new QLabel("작업 수·누적 원가·완료일은 전체 계획/상태일 기준. 기간 필터는 일별 명세·추이와 과부하 지표에 적용됩니다."); hint->setWordWrap(true); layout->addWidget(hint);
    message_ = new QLabel; message_->setObjectName("reportMessage"); message_->setWordWrap(true); message_->setTextFormat(Qt::PlainText); message_->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(message_);
    tabs_ = new QTabWidget; tabs_->setObjectName("reportTabs"); layout->addWidget(tabs_, 1);
    auto* outputs = new QHBoxLayout; layout->addLayout(outputs); family_ = new QComboBox; family_->setObjectName("reportFamily"); family_->addItems({"전체 보고서", "일정 보고서", "자원 보고서", "원가 보고서"}); outputs->addWidget(family_);
    paper_ = new QComboBox; paper_->setObjectName("reportPaper"); paper_->addItems({"PDF A4", "PDF A3"}); orientation_ = new QComboBox; orientation_->addItems({"가로", "세로"}); orientation_->setObjectName("reportOrientation"); fontSize_ = new QComboBox; fontSize_->addItems({"9 pt", "10 pt", "11 pt", "12 pt"});
    outputs->addWidget(paper_); outputs->addWidget(orientation_); outputs->addWidget(fontSize_); outputs->addStretch();
    xlsx_ = new QPushButton("엑셀 내보내기…"); xlsx_->setObjectName("exportXlsx"); pdf_ = new QPushButton("PDF 내보내기…"); pdf_->setObjectName("exportPdf"); auto* close = new QPushButton("닫기");
    outputs->addWidget(xlsx_); outputs->addWidget(pdf_); outputs->addWidget(close);
    connect(close, &QPushButton::clicked, this, &ReportDialog::reject); connect(refresh_, &QPushButton::clicked, this, &ReportDialog::prepare);
    for (auto* c : {scope_, baseline_, formula_}) connect(c, &QComboBox::currentIndexChanged, this, [this] { prepare(); });
    connect(period_, &QCheckBox::toggled, this, [this](bool on) { from_->setEnabled(on); to_->setEnabled(on); prepare(); });
    for (auto* dateEdit : {from_, to_}) connect(dateEdit, &QDateEdit::dateChanged, this, [this] { if (period_->isChecked()) prepare(); });
    connect(xlsx_, &QPushButton::clicked, this, [this] { exportFile(false); }); connect(pdf_, &QPushButton::clicked, this, [this] { exportFile(true); });
    auto* timer = new QTimer(this); timer->setInterval(250); connect(timer, &QTimer::timeout, this, [this] { if (report_ && !current()) { clear(); message_->setText("문서가 변경되었습니다. 재계산 후 내보내세요."); } }); timer->start();
    prepare();
}
bool ReportDialog::current() const { return report_ && document_.project() && report_->documentId == document_.project()->documentId && report_->revision == document_.project()->revision && report_->options.unsaved == document_.dirty(); }
void ReportDialog::exportEnabled() { auto enabled = current() && !exporting_; xlsx_->setEnabled(enabled); pdf_->setEnabled(enabled); }
void ReportDialog::clear() { report_.reset(); while (tabs_->count()) { auto* w = tabs_->widget(0); tabs_->removeTab(0); delete w; } exportEnabled(); }
void ReportDialog::prepare() {
    if (exporting_) return;
    const auto generation = ++generation_; clear(); if (!document_.project()) return;
    auto project = *document_.project(); ReportOptions options; options.wbsId = scope_->currentData().toString().toStdString(); options.baselineId = baseline_->currentData().toString().toStdString(); options.formula = formula_->currentData().toString().toStdString();
    options.reportId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString(); options.generatedAt = QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd'T'HH:mm:ss'Z'").toStdString(); options.unsaved = document_.dirty();
    if (period_->isChecked()) { options.fromDate = from_->date().toString(Qt::ISODate).toStdString(); options.toDate = to_->date().toString(Qt::ISODate).toStdString(); }
    message_->setText("보고서 스냅샷 계산 중…");
    auto* watcher = new QFutureWatcher<Prepared>(this);
    connect(watcher, &QFutureWatcher<Prepared>::finished, this, [this, watcher, generation, id = project.documentId, revision = project.revision, dirty = options.unsaved] {
        const auto result = watcher->result(); watcher->deleteLater(); if (generation != generation_) return;
        if (!document_.project() || document_.project()->documentId != id || document_.project()->revision != revision || document_.dirty() != dirty) { message_->setText("문서가 변경되었습니다. 재계산 후 내보내세요."); return; }
        if (!result.error.isEmpty()) { message_->setText("보고서 오류: " + result.error); return; }
        report_ = result.report; showReport(); exportEnabled();
    });
    watcher->setFuture(QtConcurrent::run([project = std::move(project), options] { Prepared out; try { auto report = std::make_shared<ProjectReport>(buildReport(project, options)); validateReportExport(*report); out.report = std::move(report); } catch (const std::exception& e) { out.error = QString::fromUtf8(e.what()); } return out; }));
}
void ReportDialog::showReport() {
    message_->setText(QString("%1 / revision %2 / %3 / 상태일 %4 / %5\n보고서 %6 / 생성 UTC %7")
        .arg(reportText(report_->projectName)).arg(report_->revision).arg(report_->options.unsaved ? "미저장 메모리" : "저장된 원본").arg(reportText(report_->statusAt.value_or("미설정"))).arg(reportText(report_->scopeName)).arg(reportText(report_->options.reportId)).arg(reportText(report_->options.generatedAt)));
    auto* scroll = new QScrollArea; scroll->setWidgetResizable(true); auto* overview = new QWidget; auto* layout = new QVBoxLayout(overview); auto* grid = new QGridLayout; layout->addLayout(grid);
    int n = 0; for (const auto& k : report_->kpis) {
        auto* card = new QFrame; card->setStyleSheet("QFrame { background: #edf3f8; border-radius: 7px; } QLabel { border: none; color: #17324d; }"); auto* body = new QVBoxLayout(card);
        auto* title = new QLabel(reportText(k.title)); auto* value = new QLabel(displayCell(k.value, report_->currency) + " " + reportText(k.unit)); value->setObjectName("dashboard_" + reportText(k.id)); auto f = value->font(); f.setPointSize(17); f.setBold(true); value->setFont(f); value->setTextFormat(Qt::PlainText); value->setWordWrap(true);
        auto* note = new QLabel(reportText(k.note)); note->setWordWrap(true); note->setTextFormat(Qt::PlainText); title->setTextFormat(Qt::PlainText); body->addWidget(title); body->addWidget(value); body->addWidget(note); grid->addWidget(card, n / 4, n % 4); ++n;
    }
    layout->addWidget(new Trend(report_, overview)); scroll->setWidget(overview); tabs_->addTab(scroll, "대시보드");
    for (std::size_t i = 0; i < report_->tables.size(); ++i) {
        const auto& t = report_->tables[i]; auto* page = new QWidget; auto* body = new QVBoxLayout(page); auto* note = new QLabel(reportText(t.note)); note->setTextFormat(Qt::PlainText); note->setWordWrap(true); body->addWidget(note);
        auto* view = new QTableView; view->setObjectName(reportText(t.id)); view->setModel(new ReportModel(report_, i, view)); view->setAlternatingRowColors(true); view->setWordWrap(false); view->setSelectionBehavior(QAbstractItemView::SelectRows); view->setSortingEnabled(false);
        for (int c = 0; c < int(t.columns.size()); ++c) view->setColumnWidth(c, t.columns[std::size_t(c)].width * 8);
        body->addWidget(view); tabs_->addTab(page, t.id == "dashboard" ? "지표 명세" : reportText(t.title));
    }
}
void ReportDialog::exportFile(bool pdf) {
    if (!current()) { clear(); message_->setText("문서가 변경되었습니다. 재계산 후 내보내세요."); return; }
    const auto snapshot = report_; const auto family = static_cast<ReportFamily>(family_->currentIndex());
    auto name = reportText(snapshot->projectName); name.replace(QRegularExpression("[<>:\"/\\\\|?*\\x00-\\x1f]"), "_"); name = name.left(60);
    const auto extension = pdf ? ".pdf" : ".xlsx"; const auto suggested = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/JOBS_" + name + "_" + QDateTime::currentDateTimeUtc().toString("yyyyMMdd_HHmmss_zzz") + extension;
    auto path = QFileDialog::getSaveFileName(this, "보고서 저장 · 새 파일명을 선택하세요", suggested, pdf ? "PDF (*.pdf)" : "Excel (*.xlsx)", nullptr, QFileDialog::DontConfirmOverwrite);
    if (path.isEmpty()) return;
    if (QFileInfo(path).suffix().isEmpty()) path += extension;
    if (!current() || report_ != snapshot) { clear(); message_->setText("문서가 변경되어 출력을 취소했습니다. 재계산하세요."); return; }
    PdfOptions options; options.a3 = paper_->currentIndex() == 1; options.landscape = orientation_->currentIndex() == 0; options.fontPoints = 9 + fontSize_->currentIndex(); options.font = QApplication::font();
    exporting_ = true; exportEnabled(); refresh_->setEnabled(false); for (auto* c : {scope_, baseline_, formula_}) c->setEnabled(false); period_->setEnabled(false); from_->setEnabled(false); to_->setEnabled(false); message_->setText("보고서 출력 중…");
    auto* watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher, path, snapshot] {
        const auto result = watcher->result(); watcher->deleteLater(); exporting_ = false; refresh_->setEnabled(true); for (auto* c : {scope_, baseline_, formula_}) c->setEnabled(true); period_->setEnabled(true); from_->setEnabled(period_->isChecked()); to_->setEnabled(period_->isChecked());
        message_->setText(result.isEmpty() ? "내보내기 완료: " + path + "\n보고서 " + reportText(snapshot->options.reportId) : "내보내기 실패: " + result);
        if (!current()) { clear(); message_->setText(message_->text() + "\n원본이 변경되었습니다. 다음 출력 전에 재계산하세요."); } exportEnabled();
    });
    watcher->setFuture(QtConcurrent::run([snapshot, path, family, options, pdf] { try { if (pdf) exportPdf(*snapshot, path, family, options); else exportXlsx(*snapshot, path, family); return QString{}; } catch (const std::exception& e) { return QString::fromUtf8(e.what()); } }));
}
void ReportDialog::closeEvent(QCloseEvent* e) { if (exporting_) e->ignore(); else QDialog::closeEvent(e); }
void ReportDialog::reject() { if (!exporting_) QDialog::reject(); }
}
