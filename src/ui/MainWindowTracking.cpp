#include "ui/MainWindow.h"
#include "ui/ComparisonDialog.h"
#include <QAction>
#include <algorithm>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSpinBox>
#include <QTableWidget>
#include <QTimeZone>
#include <QtConcurrent/QtConcurrentRun>
namespace jobs {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
std::string utcNow() { return str(QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd'T'HH:mm:ss'Z'")); }
std::optional<std::string> optional(const QString& s) { return s.trimmed().isEmpty() ? std::nullopt : std::optional<std::string>{str(s.trimmed())}; }
struct BaselinePreparation { std::optional<Baseline> baseline; std::string error; };
}
void MainWindow::refreshTracking() {
    const auto& p = document_.project(); const bool writable = p.has_value() && !document_.readOnly();
    baselineSave_->setEnabled(writable && !baselinePreparing_); statusEdit_->setEnabled(writable);
    progressEdit_->setEnabled(writable && p->statusAt.has_value() && !selectedTask_.empty()); comparison_->setEnabled(p.has_value());
    const QSignalBlocker block(baselineChoice_); baselineChoice_->clear(); baselineChoice_->addItem("간트: 기준선 없음", QString{});
    if (p) for (const auto& b : p->baselines) {
        baselineChoice_->addItem("v" + QString::number(b.version) + " · " + qs(b.name), qs(b.id));
        baselineChoice_->setItemData(baselineChoice_->count() - 1, "승인자: " + qs(b.approvedBy) + "\n사유: " + qs(b.reason), Qt::ToolTipRole);
    }
    const auto i = baselineChoice_->findData(qs(selectedBaseline_)); baselineChoice_->setCurrentIndex(std::max(0, i));
    if (i < 0) selectedBaseline_.clear();
    baselineChoice_->setEnabled(p.has_value());
}
void MainWindow::saveBaseline() {
    if (!document_.project() || document_.readOnly() || baselinePreparing_) return;
    QDialog dialog(this); dialog.setWindowTitle("기준선 저장"); dialog.setObjectName("baselineDialog"); dialog.resize(500, 270);
    auto* layout = new QFormLayout(&dialog);
    layout->addRow(new QLabel("현재 계획의 입력·달력·WBS·계산 결과를 새 버전으로 고정합니다.\n기존 기준선은 유지되며 실적은 별도 이력으로 관리합니다.", &dialog));
    auto* name = new QLineEdit(&dialog); name->setObjectName("baselineName");
    auto* author = new QLineEdit(&dialog); author->setObjectName("baselineApprover");
    auto* reason = new QLineEdit(&dialog); reason->setObjectName("baselineReason");
    layout->addRow("기준선 이름", name); layout->addRow("승인자", author); layout->addRow("승인/변경 사유", reason);
    auto* error = new QLabel(&dialog); error->setTextFormat(Qt::PlainText); layout->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().isEmpty() || author->text().trimmed().isEmpty() || reason->text().trimmed().isEmpty()) {
            error->setText("이름·승인자·사유를 모두 입력하세요."); return;
        }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted) return;
    const auto snapshot = *document_.project(); const auto n = str(name->text().trimmed()), a = str(author->text().trimmed()), r = str(reason->text().trimmed()), utc = utcNow();
    baselinePreparing_ = true; refreshTracking();
    auto* watcher = new QFutureWatcher<BaselinePreparation>(this);
    connect(watcher, &QFutureWatcher<BaselinePreparation>::finished, this, [this, watcher] {
        baselinePreparing_ = false;
        try {
            auto prepared = watcher->result();
            if (!prepared.baseline) throw std::runtime_error(prepared.error);
            selectedBaseline_ = document_.appendBaseline(std::move(*prepared.baseline));
        } catch (const std::exception& e) { report(e); }
        watcher->deleteLater(); refresh();
    });
    watcher->setFuture(QtConcurrent::run([snapshot, n, a, r, utc] {
        try { return BaselinePreparation{prepareBaseline(snapshot, n, a, r, utc), {}}; }
        catch (const std::exception& e) { return BaselinePreparation{std::nullopt, e.what()}; }
    }));
}
void MainWindow::editStatusAt() {
    if (!document_.project() || document_.readOnly()) return;
    QDialog dialog(this); dialog.setWindowTitle("상태일 설정"); dialog.setObjectName("statusDialog");
    auto* layout = new QFormLayout(&dialog); auto* date = new QDateTimeEdit(&dialog); date->setObjectName("statusAt");
    // UTC is only a neutral editor carrier: the stored value is a floating civil timestamp.
    date->setTimeZone(QTimeZone::UTC);
    date->setDateRange(QDate(1, 1, 1), QDate(9999, 12, 31));
    date->setCalendarPopup(true); date->setDisplayFormat("yyyy-MM-dd HH:mm");
    const auto s = document_.project()->statusAt.value_or(document_.project()->startDate + " 18:00");
    date->setDateTime(QDateTime(QDate::fromString(qs(s).left(10), Qt::ISODate), QTime::fromString(qs(s).mid(11), "HH:mm"), QTimeZone::UTC));
    layout->addRow("현지 상태일·시각", date);
    auto* clear = new QCheckBox("상태일 해제 · 현재 계획 모드 (보고 이력은 보존)", &dialog); clear->setObjectName("clearStatusAt"); layout->addRow(clear);
    layout->addRow(new QLabel("미완료 잔여 작업을 이 시각 이후로 재예측합니다.\n과거 상태일을 선택하면 그때까지의 보고만 사용합니다.", &dialog));
    auto* error = new QLabel(&dialog); error->setTextFormat(Qt::PlainText); layout->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try { document_.setStatusAt(clear->isChecked() ? std::nullopt : std::optional<std::string>{str(date->dateTime().toString("yyyy-MM-dd HH:mm"))}); dialog.accept(); }
        catch (const std::exception& e) { error->setText(QString::fromUtf8(e.what())); }
    });
    if (dialog.exec() == QDialog::Accepted) refresh();
}
void MainWindow::reportProgress() {
    if (!document_.project() || document_.readOnly() || !document_.project()->statusAt || selectedTask_.empty()) return;
    const auto& p = *document_.project(); const Task* task = nullptr;
    for (const auto& t : p.tasks) if (t.id == selectedTask_) task = &t;
    if (!task) return;
    const auto taskId = task->id; const auto* previous = progressAsOf(p, taskId);
    QDialog dialog(this); dialog.setObjectName("progressDialog"); dialog.setWindowTitle("작업 실적 입력 · 새 보고/정정"); dialog.resize(960, 620);
    auto* layout = new QFormLayout(&dialog);
    auto* heading = new QLabel(qs(task->name) + " · 상태일 " + qs(*p.statusAt), &dialog); heading->setTextFormat(Qt::PlainText); layout->addRow(heading);
    auto* start = new QLineEdit(previous && previous->actualStart ? qs(*previous->actualStart) : "", &dialog); start->setObjectName("actualStart");
    auto* finish = new QLineEdit(previous && previous->actualFinish ? qs(*previous->actualFinish) : "", &dialog); finish->setObjectName("actualFinish");
    start->setPlaceholderText("YYYY-MM-DD HH:mm · 미착수면 빈칸"); finish->setPlaceholderText("YYYY-MM-DD HH:mm · 미완료면 빈칸");
    auto* actual = new QSpinBox(&dialog); actual->setRange(0, 5000000); actual->setSuffix(" 분"); actual->setObjectName("actualMinutes");
    auto* remaining = new QSpinBox(&dialog); remaining->setRange(0, 5000000); remaining->setSuffix(" 분"); remaining->setObjectName("remainingMinutes");
    actual->setValue(previous ? int(previous->actualDurationMinutes) : 0); remaining->setValue(int(previous ? previous->remainingMinutes : task->durationMinutes.value_or(0)));
    auto* reporter = new QLineEdit(previous ? qs(previous->reporter) : "", &dialog); reporter->setObjectName("progressReporter");
    auto* reason = new QLineEdit(&dialog); reason->setObjectName("progressReason");
    layout->addRow("실제 시작", start); layout->addRow("실제 완료", finish); layout->addRow("실제 소요 근무분", actual); layout->addRow("잔여 근무분", remaining);
    layout->addRow("보고자", reporter); layout->addRow("보고/정정 사유", reason);
    layout->addRow(new QLabel("8시간=480분. 완료는 실제 시작·완료와 잔여 0을 함께 입력합니다.\n기간 진척은 실제 소요/(실제 소요+잔여)입니다. 기존 보고는 이력으로 보존됩니다.", &dialog));
    auto* history = new QTableWidget(0, 8, &dialog); history->setObjectName("progressHistory"); history->setMaximumHeight(180);
    history->setHorizontalHeaderLabels({"상태일", "실제 시작", "실제 완료", "실제(분)", "잔여(분)", "보고자", "사유", "기록(UTC)"});
    history->setEditTriggers(QAbstractItemView::NoEditTriggers); history->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    for (const auto& e : p.progress) if (e.taskId == taskId) {
        const auto row = history->rowCount(); history->insertRow(row);
        QStringList values{qs(e.statusAt), e.actualStart ? qs(*e.actualStart) : "—", e.actualFinish ? qs(*e.actualFinish) : "—",
            QString::number(e.actualDurationMinutes), QString::number(e.remainingMinutes), qs(e.reporter), qs(e.reason), qs(e.recordedAt)};
        for (int col = 0; col < values.size(); ++col) { auto* item = new QTableWidgetItem(values[col]); item->setToolTip("ID: " + qs(e.id) + "\n이전 보고: " + qs(e.previousEntryId) + "\n원본 문서: " + qs(e.sourceDocumentId)); history->setItem(row, col, item); }
    }
    layout->addRow("보고 이력", history);
    auto* error = new QLabel(&dialog); error->setTextFormat(Qt::PlainText); error->setWordWrap(true); error->setObjectName("progressError"); layout->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        ProgressEntry e; e.taskId = taskId; e.actualStart = optional(start->text()); e.actualFinish = optional(finish->text());
        e.actualDurationMinutes = actual->value(); e.remainingMinutes = remaining->value(); e.reporter = str(reporter->text().trimmed());
        e.reason = str(reason->text().trimmed()); e.recordedAt = utcNow();
        try { document_.recordProgress(std::move(e)); dialog.accept(); } catch (const std::exception& ex) { error->setText(QString::fromUtf8(ex.what())); }
    });
    if (dialog.exec() == QDialog::Accepted) refresh();
}
void MainWindow::showComparison() {
    if (!document_.project()) return;
    ComparisonDialog dialog(document_, selectedBaseline_, this); dialog.exec();
}
}
