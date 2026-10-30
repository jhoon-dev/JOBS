#include "formatting/QtDiagnosticText.h"
#include "ui/MainWindow.h"
#include "ui/EditorUi.h"
#include "ui/CostAnalysisDialog.h"
#include "ui/MoneyFields.h"
#include <QDialog>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QDateTime>
namespace jobs {
namespace {
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
std::string str(const QString &s) {
    return s.toUtf8().toStdString();
}
} // namespace
void MainWindow::editRates() {
    if (!document_.project())
        return;
    QDialog dialog(this);
    dialog.setObjectName("rateDialog");
    dialog.setWindowTitle("통화 · 자원 유효일 단가");
    dialog.resize(740, 600);
    auto *layout = new QVBoxLayout(&dialog);
    auto *currencyRow = new QHBoxLayout;
    layout->addLayout(currencyRow);
    auto *currency = new QComboBox(&dialog);
    currency->setObjectName("costCurrency");
    currency->addItems({"KRW", "JPY", "USD", "EUR"});
    currency->setCurrentText(qs(document_.project()->currency));
    auto *apply = new QPushButton("통화 설정", &dialog);
    apply->setObjectName("saveCurrency");
    currencyRow->addWidget(new QLabel("프로젝트 단일 통화", &dialog));
    currencyRow->addWidget(currency);
    currencyRow->addWidget(apply);
    auto *note = new QLabel("통화를 먼저 설정하세요. 단가·예산·원가 이력이 생긴 뒤 통화 재해석은 거절합니다. 환율 "
                            "환산은 지원하지 않습니다.\n단가 단위: 자원 100%의 1근무시간당 금액. 단가/등급은 생산성 "
                            "배수가 아닙니다. 유효일 이후 다음 단가 전까지 적용하며, 미정은 ?입니다.",
                            &dialog);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *choice = new QComboBox(&dialog);
    choice->setObjectName("rateResource");
    for (const auto &r : document_.project()->resources)
        choice->addItem(qs(r.name), qs(r.id));
    layout->addWidget(choice);
    auto *label = new QLabel(&dialog);
    layout->addWidget(label);
    auto *rates = new QPlainTextEdit(&dialog);
    rates->setObjectName("rateEntries");
    rates->setPlaceholderText("2026-10-26=10000\n2026-11-01=12000\n2027-01-01=?");
    layout->addWidget(rates);
    auto *error = new QLabel(&dialog);
    error->setObjectName("rateError");
    error->setWordWrap(true);
    layout->addWidget(error);
    auto *save = new QPushButton("선택 자원 단가 저장", &dialog);
    save->setObjectName("saveRates");
    layout->addWidget(save);
    const bool editable = !document_.readOnly();
    currency->setEnabled(editable);
    apply->setEnabled(editable);
    rates->setReadOnly(!editable);
    save->setEnabled(editable && choice->count() > 0);
    std::string sourceId;
    std::uint64_t sourceRevision = 0;
    auto fill = [&] {
        sourceId = document_.project()->documentId;
        sourceRevision = document_.project()->revision;
        QStringList lines;
        for (const auto &r : document_.project()->resources)
            if (qs(r.id) == choice->currentData().toString())
                for (const auto &v : r.rates)
                    lines << qs(v.effectiveDate) + "=" +
                                 moneyui::format(v.hourlyMinor, document_.project()->currency, "?");
        rates->setPlainText(lines.join('\n'));
        label->setText("한 줄에 YYYY-MM-DD=금액 또는 ? · " + qs(document_.project()->currency) +
                       "/h · 단가를 바꿔도 기준선·실제원가는 보존합니다.");
    };
    connect(choice, &QComboBox::currentIndexChanged, &dialog, fill);
    connect(apply, &QPushButton::clicked, &dialog, [&] {
        try {
            document_.setCurrency(str(currency->currentText()));
            fill();
            error->setText("통화를 설정했습니다. 파일 저장으로 보존하세요.");
        } catch (const std::exception &e) {
            currency->setCurrentText(qs(document_.project()->currency));
            showEditorError(*error, e, rates);
        }
    });
    connect(save, &QPushButton::clicked, &dialog, [&] {
        try {
            if (sourceId != document_.project()->documentId || sourceRevision != document_.project()->revision)
                throw AppError(ErrorCode::MAINWINDOWCOSTS_001, {});
            if (currency->currentText() != qs(document_.project()->currency))
                throw AppError(ErrorCode::MAINWINDOWCOSTS_002, {});
            auto id = str(choice->currentData().toString());
            for (auto r : document_.project()->resources)
                if (r.id == id) {
                    r.rates.clear();
                    for (auto line : rates->toPlainText().split('\n', Qt::SkipEmptyParts)) {
                        auto parts = line.trimmed().split('=');
                        if (parts.size() != 2 || parts[1].trimmed().isEmpty())
                            throw AppError(ErrorCode::MAINWINDOWCOSTS_003, {});
                        r.rates.push_back(
                            {str(parts[0].trimmed()), parts[1].trimmed() == "?"
                                                          ? Money{}
                                                          : moneyui::parse(parts[1], document_.project()->currency)});
                    }
                    document_.putResource(std::move(r));
                    fill();
                    error->setText("단가를 문서에 반영했습니다. 파일 저장으로 보존하세요.");
                    break;
                }
        } catch (const std::exception &e) {
            showEditorError(*error, e, rates);
        }
    });
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    layout->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    fill();
    prepareEditorDialog(dialog);
    dialog.exec();
    refresh();
}
void MainWindow::editTaskCost() {
    if (!document_.project() || document_.readOnly() || selectedTask_.empty())
        return;
    const auto snapshot = *document_.project();
    const auto id = selectedTask_;
    QString name;
    for (const auto &t : snapshot.tasks)
        if (t.id == id)
            name = qs(t.name);
    QDialog dialog(this);
    dialog.setObjectName("taskCostDialog");
    dialog.setWindowTitle("작업 예산 · " + name);
    dialog.resize(650, 340);
    auto *form = new QFormLayout(&dialog);
    auto *fixed = new QLineEdit(&dialog);
    fixed->setObjectName("fixedCost");
    fixed->setPlaceholderText("공란=미정 · 0=고정비 없음");
    auto *accrual = new QComboBox(&dialog);
    accrual->setObjectName("fixedAccrual");
    accrual->addItem("근무기간에 균등", "uniform");
    accrual->addItem("시작 시 전액", "start");
    accrual->addItem("완료 시 전액", "finish");
    auto *credit = new QComboBox(&dialog);
    credit->setObjectName("evCredit");
    credit->addItem("물리 진척 보고 × 기준선 예산", "physical");
    credit->addItem("0/100: 실제 완료 보고 시 전액", "zero-hundred");
    for (const auto &c : snapshot.taskCosts)
        if (c.taskId == id) {
            fixed->setText(moneyui::format(c.fixedMinor, snapshot.currency, ""));
            accrual->setCurrentIndex(accrual->findData(qs(c.accrual)));
            credit->setCurrentIndex(credit->findData(qs(c.credit)));
        }
    form->addRow("추가 고정비 (" + qs(snapshot.currency) + ")", fixed);
    form->addRow("고정비 계획 발생", accrual);
    form->addRow("EV 성과 측정법", credit);
    auto *note =
        new QLabel("작업 예산 = 배정 자원비 + 추가 고정비. 실제원가는 별도로 보고합니다.\n기간 진척은 물리 진척으로 "
                   "사용하지 않습니다. 측정법·단가·고정비는 새 기준선에 고정되며 기존 기준선은 바뀌지 않습니다.",
                   &dialog);
    note->setWordWrap(true);
    form->addRow(note);
    auto *error = new QLabel(&dialog);
    error->setObjectName("taskCostError");
    error->setWordWrap(true);
    form->addRow(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            if (document_.project()->documentId != snapshot.documentId ||
                document_.project()->revision != snapshot.revision)
                throw AppError(ErrorCode::MAINWINDOWCOSTS_004, {});
            document_.setTaskCost({id, moneyui::parse(fixed->text(), snapshot.currency),
                                   str(accrual->currentData().toString()), str(credit->currentData().toString())});
            dialog.accept();
        } catch (const std::exception &e) {
            showEditorError(*error, e, fixed);
        }
    });
    prepareEditorDialog(dialog);
    dialog.exec();
    refresh();
}
void MainWindow::reportCost() {
    if (!document_.project() || document_.readOnly() || !document_.project()->statusAt || selectedTask_.empty())
        return;
    const auto snapshot = *document_.project();
    const auto id = selectedTask_;
    QDialog dialog(this);
    dialog.setObjectName("costReportDialog");
    dialog.setWindowTitle("누적 실제원가 · 물리 진척 보고");
    dialog.resize(900, 640);
    auto *layout = new QVBoxLayout(&dialog);
    auto *note = new QLabel("상태일 " + qs(*snapshot.statusAt) + " · " + qs(snapshot.currency) +
                                "\n상태일까지의 누적 금액과 물리 진척을 보고하세요. 공란=미정, 0=확인된 0. 정정도 새 "
                                "이력으로 누적하며 과거 보고를 덮어쓰지 않습니다.",
                            &dialog);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *form = new QFormLayout;
    layout->addLayout(form);
    auto *actual = new QLineEdit(&dialog);
    actual->setObjectName("actualCost");
    auto *physical = new QLineEdit(&dialog);
    physical->setObjectName("physicalProgress");
    if (auto *e = costAsOf(snapshot, id, *snapshot.statusAt)) {
        actual->setText(moneyui::format(e->actualMinor, snapshot.currency, ""));
        if (e->physicalBasisPoints)
            physical->setText(moneyui::format(*e->physicalBasisPoints, "USD"));
    }
    auto *author = new QLineEdit(&dialog);
    author->setObjectName("costReporter");
    auto *reason = new QLineEdit(&dialog);
    reason->setObjectName("costReason");
    form->addRow("누적 실제원가", actual);
    form->addRow("물리 진척 (0~100%, 소수 2자리)", physical);
    form->addRow("보고자", author);
    form->addRow("보고/정정 사유", reason);
    auto *history = new QTableWidget(0, 6, &dialog);
    history->setObjectName("costHistory");
    history->setHorizontalHeaderLabels({"상태일", "누적 실제원가", "물리 진척(%)", "보고자", "사유", "기록 UTC"});
    history->setEditTriggers(QAbstractItemView::NoEditTriggers);
    history->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    layout->addWidget(history, 1);
    for (const auto &e : snapshot.costEntries)
        if (e.taskId == id) {
            auto row = history->rowCount();
            history->insertRow(row);
            QStringList cells{qs(e.statusAt),
                              moneyui::format(e.actualMinor, e.currency),
                              e.physicalBasisPoints ? moneyui::format(*e.physicalBasisPoints, "USD") : "—",
                              qs(e.reporter),
                              qs(e.reason),
                              qs(e.recordedAt)};
            for (int c = 0; c < cells.size(); ++c) {
                auto *item = new QTableWidgetItem(cells[c]);
                item->setToolTip("ID " + qs(e.id) + "\n이전 보고 " + qs(e.previousEntryId) + "\n원본 " +
                                 qs(e.sourceDocumentId) + " rev " + QString::number(e.recordedRevision));
                history->setItem(row, c, item);
            }
        }
    auto *error = new QLabel(&dialog);
    error->setObjectName("costReportError");
    error->setWordWrap(true);
    layout->addWidget(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            if (document_.project()->documentId != snapshot.documentId ||
                document_.project()->revision != snapshot.revision)
                throw AppError(ErrorCode::MAINWINDOWCOSTS_005, {});
            CostEntry e;
            e.taskId = id;
            e.actualMinor = moneyui::parse(actual->text(), snapshot.currency);
            auto value = moneyui::parse(physical->text(), "USD");
            if (value && *value > 10000)
                throw AppError(ErrorCode::MAINWINDOWCOSTS_006, {});
            if (value)
                e.physicalBasisPoints = int(*value);
            e.reporter = str(author->text().trimmed());
            e.reason = str(reason->text().trimmed());
            e.recordedAt = str(QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ssZ"));
            document_.recordCost(std::move(e));
            dialog.accept();
        } catch (const std::exception &e) {
            error->setText(qErrorText(e));
        }
    });
    prepareEditorDialog(dialog);
    dialog.exec();
    refresh();
}
void MainWindow::showCosts() {
    if (!document_.project())
        return;
    CostAnalysisDialog dialog(document_, selectedBaseline_, this);
    prepareEditorDialog(dialog);
    dialog.exec();
    refresh();
}
} // namespace jobs
