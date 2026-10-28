#include "ui/RecoveryDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QMessageBox>
#include <QFutureWatcher>
#include <QCloseEvent>
#include <QtConcurrent>
namespace jobs {
namespace {
QString q(const std::string &s) {
    return QString::fromStdString(s);
}
struct Scanned {
    RecoveryScan scan;
    QString error;
};
struct Loaded {
    std::shared_ptr<RecoveryClaim> claim;
    QString error;
};
} // namespace
RecoveryDialog::RecoveryDialog(JsonRecoveryStore store, QWidget *parent, std::optional<RecoveryScan> initial)
    : QDialog(parent), store_(std::move(store)) {
    setObjectName("recoveryDialog");
    setWindowTitle("자동 저장 복구본 선택");
    resize(1380, 720);
    auto *layout = new QVBoxLayout(this);
    auto *hint = new QLabel(
        "완료·무결성 검사를 통과한 세대를 선택해 새 사본으로 엽니다. 원본을 자동으로 덮어쓰거나 병합하지 "
        "않습니다.\n복구본은 새 파일 저장 성공 전까지 보존합니다. 실행 중인 다른 앱의 세션은 목록에서 제외합니다.");
    hint->setWordWrap(true);
    layout->addWidget(hint);
    message_ = new QLabel;
    message_->setObjectName("recoveryMessage");
    message_->setWordWrap(true);
    message_->setTextFormat(Qt::PlainText);
    message_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(message_);
    table_ = new QTableWidget(0, 8);
    table_->setObjectName("recoveryList");
    table_->setHorizontalHeaderLabels({"프로젝트", "복구 시각 UTC", "복구 revision", "명시 저장 revision", "세대",
                                       "원본 상태", "원본 경로", "문서 ID"});
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setSelectionMode(QAbstractItemView::SingleSelection);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table_->setAlternatingRowColors(true);
    table_->horizontalHeader()->setSectionResizeMode(QHeaderView::Interactive);
    for (int i = 0; i < 8; ++i)
        table_->setColumnWidth(i, i == 0 ? 190 : i == 6 ? 300 : i == 7 ? 280 : i == 5 ? 240 : 140);
    layout->addWidget(table_, 1);
    details_ = new QLabel;
    details_->setObjectName("recoveryDetails");
    details_->setTextFormat(Qt::PlainText);
    details_->setWordWrap(true);
    details_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(details_);
    auto *buttons = new QHBoxLayout;
    layout->addLayout(buttons);
    reload_ = new QPushButton("다시 검색");
    reload_->setObjectName("reloadRecovery");
    discard_ = new QPushButton("선택 세션의 복구본 모두 버리기…");
    discard_->setObjectName("discardRecovery");
    restore_ = new QPushButton("복구 사본 열기");
    restore_->setObjectName("restoreRecovery");
    auto *later = new QPushButton("나중에");
    later->setObjectName("laterRecovery");
    buttons->addWidget(reload_);
    buttons->addWidget(discard_);
    buttons->addStretch();
    buttons->addWidget(restore_);
    buttons->addWidget(later);
    connect(later, &QPushButton::clicked, this, &RecoveryDialog::reject);
    connect(reload_, &QPushButton::clicked, this, &RecoveryDialog::reload);
    connect(restore_, &QPushButton::clicked, this, &RecoveryDialog::restore);
    connect(discard_, &QPushButton::clicked, this, &RecoveryDialog::discard);
    connect(table_, &QTableWidget::itemSelectionChanged, this, &RecoveryDialog::select);
    if (initial)
        populate(std::move(*initial));
    else
        reload();
}
void RecoveryDialog::setBusy(bool busy) {
    busy_ = busy;
    table_->setEnabled(!busy);
    reload_->setEnabled(!busy);
    select();
}
void RecoveryDialog::populate(RecoveryScan scan) {
    scan_ = std::move(scan);
    table_->setRowCount(0);
    table_->setRowCount(int(scan_.candidates.size()));
    int first = -1, invalid = 0;
    for (int i = 0; i < int(scan_.candidates.size()); ++i) {
        const auto &c = scan_.candidates[std::size_t(i)];
        if (!c.valid)
            ++invalid;
        else if (first < 0)
            first = i;
        const QStringList values{c.valid ? q(c.projectName) : "손상/미지원 복구본",
                                 q(c.capturedAt),
                                 c.valid ? QString::number(c.revision) : "미확인",
                                 !c.valid              ? "미확인"
                                 : c.lastSavedRevision ? QString::number(*c.lastSavedRevision)
                                                       : "처음 저장 전",
                                 c.sequence ? QString::number(c.sequence) : "미확인",
                                 c.valid ? c.sourceState : "복구 불가 · 진단 확인",
                                 q(c.sourcePath),
                                 q(c.documentId)};
        for (int col = 0; col < values.size(); ++col) {
            auto *item = new QTableWidgetItem(values[col]);
            item->setToolTip("<qt>" + values[col].toHtmlEscaped() + "</qt>");
            if (!c.valid) {
                item->setBackground(QColor("#fff1cc"));
                item->setForeground(QColor("#4f3100"));
            }
            table_->setItem(i, col, item);
        }
    }
    message_->setText(QString("복구 세대 %1개 · 손상/미지원 %2개 · 실행 중인 세션 %3개 제외")
                          .arg(scan_.candidates.size())
                          .arg(invalid)
                          .arg(scan_.activeSessions) +
                      "\n폴더: " + store_.directory());
    if (first >= 0)
        table_->selectRow(first);
    else if (table_->rowCount())
        table_->selectRow(0);
    select();
}
void RecoveryDialog::select() {
    const auto row = table_->currentRow();
    const bool selected = row >= 0 && std::size_t(row) < scan_.candidates.size() && !table_->selectedItems().isEmpty();
    restore_->setEnabled(!busy_ && selected && scan_.candidates[std::size_t(row)].valid);
    discard_->setEnabled(!busy_ && selected);
    if (!selected) {
        details_->setText("검증된 복구본을 선택하세요.");
        return;
    }
    const auto &c = scan_.candidates[std::size_t(row)];
    details_->setText(
        "세션 " + c.sessionId +
        (c.valid ? QString("\n작업 %1개 / 기준선 %2개 / 일정 실적 %3건 / 원가 이력 %4건\n")
                           .arg(c.tasks)
                           .arg(c.baselines)
                           .arg(c.progress)
                           .arg(c.costs) +
                       c.differences +
                       "\n복구 revision은 마지막 완료 체크포인트입니다. 그 이후 입력은 포함되지 않을 수 있습니다."
                 : "\n" + c.error));
}
void RecoveryDialog::reload() {
    if (busy_)
        return;
    setBusy(true);
    message_->setText("복구본 검색·무결성 검사 중…");
    auto *watcher = new QFutureWatcher<Scanned>(this);
    connect(watcher, &QFutureWatcher<Scanned>::finished, this, [this, watcher] {
        auto result = watcher->result();
        watcher->deleteLater();
        setBusy(false);
        if (!result.error.isEmpty())
            message_->setText(result.error);
        else
            populate(std::move(result.scan));
    });
    watcher->setFuture(QtConcurrent::run([store = store_] {
        Scanned out;
        try {
            out.scan = store.scan();
        } catch (const std::exception &e) {
            out.error = QString::fromUtf8(e.what());
        }
        return out;
    }));
}
void RecoveryDialog::restore() {
    const auto row = table_->currentRow();
    if (busy_ || row < 0 || std::size_t(row) >= scan_.candidates.size() || !scan_.candidates[std::size_t(row)].valid)
        return;
    auto candidate = scan_.candidates[std::size_t(row)];
    setBusy(true);
    protectedOperation_ = true;
    message_->setText("선택한 복구본을 잠그고 다시 검증하는 중…");
    auto *watcher = new QFutureWatcher<Loaded>(this);
    connect(watcher, &QFutureWatcher<Loaded>::finished, this, [this, watcher] {
        auto result = watcher->result();
        watcher->deleteLater();
        protectedOperation_ = false;
        setBusy(false);
        if (!result.error.isEmpty()) {
            message_->setText(result.error);
            return;
        }
        claim_ = std::move(result.claim);
        accept();
    });
    watcher->setFuture(QtConcurrent::run([store = store_, candidate] {
        Loaded out;
        try {
            out.claim = store.claim(candidate);
        } catch (const std::exception &e) {
            out.error = QString::fromUtf8(e.what());
        }
        return out;
    }));
}
void RecoveryDialog::discard() {
    const auto row = table_->currentRow();
    if (busy_ || row < 0 || std::size_t(row) >= scan_.candidates.size())
        return;
    const auto id = scan_.candidates[std::size_t(row)].sessionId;
    if (QMessageBox::question(this, "복구본 버리기",
                              "세션 " + id +
                                  "의 모든 세대 복구본을 삭제합니다. 원본 .jobs 파일은 유지됩니다. 계속하시겠습니까?",
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    protectedOperation_ = true;
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher] {
        auto error = watcher->result();
        watcher->deleteLater();
        protectedOperation_ = false;
        setBusy(false);
        if (!error.isEmpty())
            message_->setText(error);
        else
            reload();
    });
    watcher->setFuture(QtConcurrent::run([store = store_, id] {
        try {
            store.discardSession(id);
            return QString{};
        } catch (const std::exception &e) {
            return QString::fromUtf8(e.what());
        }
    }));
}
void RecoveryDialog::reject() {
    if (!protectedOperation_)
        QDialog::reject();
}
void RecoveryDialog::closeEvent(QCloseEvent *e) {
    if (protectedOperation_)
        e->ignore();
    else
        QDialog::closeEvent(e);
}
} // namespace jobs
