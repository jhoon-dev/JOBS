#include "formatting/QtDiagnosticText.h"
#include "ui/CostAnalysisDialog.h"
#include "ui/MoneyFields.h"
#include <QComboBox>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QPainter>
#include <QPainterPath>
#include <QDate>
#include <algorithm>
namespace jobs {
namespace {
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
QString ratio(const std::optional<double> &v) {
    return v ? QString::number(*v, 'f', 4) : "—";
}
QTableWidget *table(QWidget *parent, const char *name, const QStringList &columns) {
    auto *t = new QTableWidget(0, columns.size(), parent);
    t->setObjectName(name);
    t->setHorizontalHeaderLabels(columns);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    return t;
}
void cells(QTableWidget *t, int row, const QStringList &values, const QString &detail) {
    for (int c = 0; c < values.size(); ++c) {
        auto *item = new QTableWidgetItem(values[c]);
        item->setToolTip(detail);
        t->setItem(row, c, item);
    }
}
struct Computed {
    CostAnalysis report;
    QString error;
};
} // namespace
class CostChart final : public QWidget {
public:
    using QWidget::QWidget;
    void setReport(const CostAnalysis &r) {
        days_ = r.days;
        currency_ = r.currency;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), palette().base());
        p.setPen(palette().text().color());
        p.drawText(QRectF(12, 8, width() - 24, 30),
                   "선택 기준선 누적 금액 · PV 계획 / EV 획득 / AC 실제 · 미정 구간은 연결하지 않습니다.");
        if (days_.empty()) {
            p.drawText(rect(), Qt::AlignCenter, "표시할 기간 데이터가 없습니다.");
            return;
        }
        const QRectF plot(115, 65, std::max(1, width() - 150), std::max(1, height() - 110));
        std::int64_t maximum = 1;
        for (const auto &d : days_)
            for (auto n : {d.pv, d.ev, d.ac})
                if (n)
                    maximum = std::max(maximum, *n);
        p.drawLine(plot.bottomLeft(), plot.topLeft());
        p.drawLine(plot.bottomLeft(), plot.bottomRight());
        p.drawText(QRectF(5, plot.top() - 10, 105, 25), Qt::AlignRight, moneyui::format(maximum, currency_));
        p.drawText(QRectF(5, plot.bottom() - 10, 105, 25), Qt::AlignRight, "0 " + qs(currency_));
        auto first = QDate::fromString(qs(days_.front().date), Qt::ISODate),
             last = QDate::fromString(qs(days_.back().date), Qt::ISODate);
        const auto range = std::max<qint64>(1, first.daysTo(last));
        const QColor colors[]{QColor("#328bd6"), QColor("#339b69"), QColor("#d58125")};
        const QString names[]{"PV", "EV", "AC"};
        for (int series = 0; series < 3; ++series) {
            p.setPen(QPen(colors[series], 2));
            p.drawText(QRectF(120 + series * 85, 37, 80, 25), names[series]);
            QPainterPath path;
            bool connected = false;
            for (const auto &d : days_) {
                auto n = series == 0 ? d.pv : series == 1 ? d.ev : d.ac;
                if (!n) {
                    connected = false;
                    continue;
                }
                QPointF point(plot.left() + double(first.daysTo(QDate::fromString(qs(d.date), Qt::ISODate))) / range *
                                                plot.width(),
                              plot.bottom() - double(*n) / double(maximum) * plot.height());
                if (connected)
                    path.lineTo(point);
                else
                    path.moveTo(point);
                connected = true;
                p.drawEllipse(point, 2, 2);
            }
            p.drawPath(path);
        }
        p.setPen(palette().text().color());
        p.drawText(QRectF(plot.left(), plot.bottom() + 8, 160, 30), qs(days_.front().date));
        p.drawText(QRectF(plot.right() - 160, plot.bottom() + 8, 160, 30), Qt::AlignRight, qs(days_.back().date));
    }

private:
    std::vector<CostDay> days_;
    std::string currency_ = "KRW";
};
CostAnalysisDialog::CostAnalysisDialog(Document &doc, const std::string &id, QWidget *parent)
    : QDialog(parent), document_(doc) {
    setObjectName("costAnalysisDialog");
    setWindowTitle("계획·실제 원가 · 획득가치(EVM) 분석");
    resize(1480, 850);
    auto *layout = new QVBoxLayout(this);
    auto *choices = new QHBoxLayout;
    layout->addLayout(choices);
    baseline_ = new QComboBox(this);
    baseline_->setObjectName("costBaseline");
    baseline_->addItem("기준선 없음 · 현재 계획/실제만", QString{});
    for (const auto &b : doc.project()->baselines)
        baseline_->addItem("v" + QString::number(b.version) + " · " + qs(b.name) + " · " + qs(b.input.currency),
                           qs(b.id));
    auto index = baseline_->findData(qs(id));
    baseline_->setCurrentIndex(index > 0 ? index : baseline_->count() - 1);
    formula_ = new QComboBox(this);
    formula_->setObjectName("eacFormula");
    formula_->addItem("ETC=BAC−EV · 잔여는 예산대로", "budget-remaining-v1");
    formula_->addItem("EAC=BAC/CPI · 원가 효율 지속", "cpi-v1");
    formula_->addItem("ETC=(BAC−EV)/(CPI×SPI) · 양 효율 지속", "cpi-spi-v1");
    choices->addWidget(new QLabel("원가 기준선", this));
    choices->addWidget(baseline_, 1);
    choices->addWidget(new QLabel("EAC/ETC 가정", this));
    choices->addWidget(formula_, 1);
    summary_ = new QLabel(this);
    summary_->setObjectName("costSummary");
    summary_->setTextFormat(Qt::PlainText);
    summary_->setWordWrap(true);
    summary_->setMinimumHeight(145);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(summary_);
    auto *tabs = new QTabWidget(this);
    tabs->setObjectName("costTabs");
    layout->addWidget(tabs, 1);
    tasks_ = table(this, "costTasks",
                   {"작업", "현재 예산", "BAC", "PV", "EV", "AC", "CV", "SV", "CPI", "SPI", "EAC", "ETC",
                    "물리 진척(%)", "기준선 EV 측정법", "범위/진단"});
    tabs->addTab(tasks_, "작업별 비용·EVM");
    groups_ =
        table(this, "costGroups",
              {"집계 범위", "현재 예산", "현재 실제", "BAC", "PV", "EV", "AC", "CV", "SV", "CPI", "SPI", "EAC", "ETC"});
    tabs->addTab(groups_, "프로젝트·WBS 집계");
    days_ =
        table(this, "costDays", {"날짜", "현재 계획 일별", "기준선 예산 일별", "기준선 PV 누적", "EV 누적", "AC 누적"});
    tabs->addTab(days_, "기간별 금액");
    chart_ = new CostChart(this);
    chart_->setObjectName("costChart");
    chart_->setMinimumHeight(220);
    tabs->addTab(chart_, "PV·EV·AC 추이");
    diagnostics_ = new QPlainTextEdit(this);
    diagnostics_->setObjectName("costDiagnostics");
    diagnostics_->setReadOnly(true);
    diagnostics_->setMaximumHeight(105);
    layout->addWidget(diagnostics_);
    auto *note = new QLabel(
        "EVM은 선택 기준선 작업 범위 · 현재 계획/실제 총계는 현재 작업 범위입니다. 보고 공란·미보고·누락 단가는 0으로 "
        "합산하지 않습니다.\nCV=EV−AC, SV=EV−PV(금액 편차), CPI=EV/AC, SPI=EV/PV. 0분모는 —. EAC=AC+ETC. 일정 금액 "
        "편차를 날짜 지연으로 해석하지 않습니다.\n추이는 근무일·보고일의 일말 값이며 상태일 당일은 상태 "
        "시각까지입니다. 이후 EV/AC는 예측하지 않습니다. AC는 누적 보고값으로 지출 발생일별 내역을 추정하지 않습니다.",
        this);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close, this);
    layout->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(baseline_, &QComboBox::currentIndexChanged, this, [this] { recalculate(); });
    connect(formula_, &QComboBox::currentIndexChanged, this, [this] { recalculate(); });
    recalculate();
}
void CostAnalysisDialog::recalculate() {
    const auto snapshot = *document_.project();
    const auto generation = ++generation_;
    auto baseline = baseline_->currentData().toString().toStdString(),
         formula = formula_->currentData().toString().toStdString();
    summary_->setText("원가·EVM 계산 중…");
    tasks_->setRowCount(0);
    groups_->setRowCount(0);
    days_->setRowCount(0);
    diagnostics_->clear();
    chart_->setReport(CostAnalysis{});
    auto *watcher = new QFutureWatcher<Computed>(this);
    connect(watcher, &QFutureWatcher<Computed>::finished, this, [this, watcher, generation] {
        auto result = watcher->result();
        watcher->deleteLater();
        if (generation != generation_)
            return;
        if (!result.error.isEmpty()) {
            summary_->setText(result.error);
            return;
        }
        display(result.report);
    });
    watcher->setFuture(QtConcurrent::run([snapshot, baseline, formula] {
        Computed c;
        try {
            c.report = analyzeCosts(snapshot, baseline, formula);
        } catch (const std::exception &e) {
            c.error = qErrorText(e);
        }
        return c;
    }));
}
void CostAnalysisDialog::display(const CostAnalysis &r) {
    if (!document_.project() || document_.project()->documentId != r.documentId ||
        document_.project()->revision != r.revision) {
        summary_->setText("계산 중 문서가 변경되었습니다. 창을 다시 열어 최신 입력으로 계산하세요.");
        return;
    }
    auto m = [&](Money n) { return moneyui::format(n, r.currency); };
    QString provenance = "기준선 없음";
    for (const auto &b : document_.project()->baselines)
        if (b.id == r.baselineId)
            provenance = "기준선 v" + QString::number(b.version) + " " + qs(b.name) + " · 원본 rev " +
                         QString::number(b.inputRevision) + " · 승인 " + qs(b.approvedBy) + " · " + qs(b.createdAt);
    summary_->setText("통화 " + qs(r.currency) + " · 현재 rev " + QString::number(r.revision) + " · 상태일 " +
                      (r.statusAt ? qs(*r.statusAt) : "미설정") + "\n" + provenance + "\n현재 작업 총계: 계획 예산 " +
                      m(r.plan) + " · 실제원가 " + m(r.actual) + " · AC 보고 " + QString::number(r.actualCoverage) +
                      "/" + QString::number(document_.project()->tasks.size()) + "\n기준선 작업 총계: BAC " +
                      m(r.evm.bac) + " · PV " + m(r.evm.pv) + " · EV " + m(r.evm.ev) + " · AC " + m(r.evm.ac) +
                      " · EV 근거 " + QString::number(r.evCoverage) + "/" + QString::number(r.baselineTasks) + "\nCV " +
                      m(r.evm.cv) + " · SV " + m(r.evm.sv) + " · CPI " + ratio(r.evm.cpi) + " · SPI " +
                      ratio(r.evm.spi) + " · EAC " + m(r.evm.eac) + " · ETC " + m(r.evm.etc) + " · " + qs(r.formula));
    tasks_->setRowCount(int(r.tasks.size()));
    for (int i = 0; i < tasks_->rowCount(); ++i) {
        const auto &row = r.tasks[std::size_t(i)];
        const auto &e = row.evm;
        QStringList issues;
        for (const auto &text : row.issues)
            issues << qs(diagnosticMessage(text));
        auto detail = "작업 ID " + qs(row.taskId);
        if (r.statusAt)
            if (auto *report = costAsOf(*document_.project(), row.taskId, *r.statusAt))
                detail += "\n원가 보고 ID " + qs(report->id) + " · 상태일 " + qs(report->statusAt) + " · " +
                          qs(report->reporter) + " · " + qs(report->reason) + "\n출처 " + qs(report->sourceDocumentId) +
                          " rev " + QString::number(report->recordedRevision);
        cells(tasks_, i,
              {qs(row.name), m(row.plan), m(e.bac), m(e.pv), m(e.ev), m(e.ac), m(e.cv), m(e.sv), ratio(e.cpi),
               ratio(e.spi), m(e.eac), m(e.etc),
               row.physicalBasisPoints ? moneyui::format(*row.physicalBasisPoints, "USD") : "—",
               row.credit.empty()         ? "—"
               : row.credit == "physical" ? "물리 진척"
                                          : "완료 0/100",
               (row.baselineMember ? "기준선 작업" : "기준선 밖") + (issues.empty() ? "" : " · " + issues.join(" · "))},
              detail);
        tasks_->item(i, 0)->setData(Qt::UserRole, qs(row.taskId));
    }
    groups_->setRowCount(1 + int(r.currentWbs.size()) + (r.baselineId.empty() ? 0 : 1 + int(r.baselineWbs.size())));
    int row = 0;
    auto group = [&](const QString &label, const std::string &id, Money plan, Money actual, const Evm &e) {
        cells(groups_, row++,
              {label, m(plan), m(actual), m(e.bac), m(e.pv), m(e.ev), m(e.ac), m(e.cv), m(e.sv), ratio(e.cpi),
               ratio(e.spi), m(e.eac), m(e.etc)},
              "ID: " + qs(id) + "\n하위 작업을 고유 ID로 한 번씩 합산합니다. 상위/하위 WBS 행을 다시 더하지 마세요.");
    };
    group("현재 프로젝트", r.documentId, r.plan, r.actual, {});
    for (const auto &g : r.currentWbs)
        group("현재 WBS · " + qs(g.name), g.id, g.plan, g.actual, {});
    if (!r.baselineId.empty()) {
        group("기준선 프로젝트", r.baselineId, {}, {}, r.evm);
        for (const auto &g : r.baselineWbs)
            group("기준선 WBS · " + qs(g.name), g.id, {}, {}, g.evm);
    }
    days_->setRowCount(int(r.days.size()));
    for (int i = 0; i < days_->rowCount(); ++i) {
        const auto &d = r.days[std::size_t(i)];
        cells(days_, i, {qs(d.date), m(d.plan), m(d.budget), m(d.pv), m(d.ev), m(d.ac)},
              "일말 · 상태일 당일은 상태 시각 · 이후 EV/AC 미정");
    }
    QStringList messages;
    for (const auto &d : r.diagnostics) {
        QStringList ids;
        for (const auto &id : d.taskIds)
            ids << qs(id);
        messages << "[" + qs(d.code) + "] " + qs(diagnosticMessage(d)) + (ids.empty() ? "" : " · " + ids.join(", "));
    }
    diagnostics_->setPlainText(messages.join('\n'));
    chart_->setReport(r);
}
} // namespace jobs
