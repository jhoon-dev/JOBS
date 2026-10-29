#include "formatting/QtDiagnosticText.h"
#include "ui/MainWindow.h"
#include "ui/AutoSaveController.h"
#include "ui/AsyncEditController.h"
#include "ui/RecoveryDialog.h"
#include "persistence/ApplicationStorage.h"
#include <QApplication>
#include <QAction>
#include <QMenuBar>
#include <QMenu>
#include <QLabel>
#include <QStatusBar>
#include <QInputDialog>
#include <QSettings>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <algorithm>
namespace jobs {
void MainWindow::updateHistoryActions() {
    const bool pending = editController_ && editController_->busy();
    undo_->setEnabled(pending || document_.canUndo());
    redo_->setEnabled(pending || document_.canRedo());
    undo_->setText(document_.canUndo() ? "실행 취소: " + QString::fromStdString(document_.undoLabel()) : "실행 취소");
    redo_->setText(document_.canRedo() ? "다시 실행: " + QString::fromStdString(document_.redoLabel()) : "다시 실행");
    undo_->setToolTip(QString::fromStdString(document_.historyNotice()));
    redo_->setToolTip(QString::fromStdString(document_.historyNotice()));
    setWindowModified(document_.dirty());
    if (autosaveNow_)
        autosaveNow_->setEnabled(autosave_ && autosave_->enabled() && !autosave_->busy() && document_.dirty() &&
                                 !document_.readOnly());
}
void MainWindow::setupRecovery(JsonRecoveryStore *store) {
    auto *menu = menuBar()->addMenu("자동 저장·복구(&A)");
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::commitPendingCell);
    autosaveEnabled_ = menu->addAction("자동복구 저장 사용");
    autosaveEnabled_->setCheckable(true);
    autosaveEnabled_->setObjectName("autosaveEnabled");
    autosaveNow_ = menu->addAction("복구본 지금 저장");
    autosaveNow_->setObjectName("autosaveNow");
    auto *interval = menu->addAction("자동 저장 간격…");
    interval->setObjectName("autosaveInterval");
    recover_ = menu->addAction("복구본 선택…");
    recover_->setObjectName("recoverDocuments");
    autosaveStatus_ = new QLabel;
    autosaveStatus_->setObjectName("autosaveStatus");
    autosaveStatus_->setTextFormat(Qt::PlainText);
    autosaveStatus_->setMaximumWidth(500);
    statusBar()->addPermanentWidget(autosaveStatus_);
    autosaveEnabled_->setEnabled(store);
    autosaveNow_->setEnabled(false);
    interval->setEnabled(store);
    recover_->setEnabled(store);
    if (!store) {
        autosaveStatus_->setText("자동복구 저장 미연결");
        return;
    }
    recoveryStore_ = std::make_unique<JsonRecoveryStore>(*store);
    auto settings = applicationSettings();
    const auto seconds = std::clamp(settings.value("recovery/intervalSeconds", 30).toInt(), 5, 300);
    autosave_ = new AutoSaveController(document_, *store, this, seconds * 1000);
    const bool enabled = settings.value("recovery/enabled", true).toBool();
    autosaveEnabled_->setChecked(enabled);
    autosave_->setEnabled(enabled);
    autosave_->statusChanged = [this] {
        autosaveStatus_->setText(autosave_->status());
        autosaveStatus_->setToolTip(autosave_->status() + "\n" + recoveryStore_->directory());
        updateHistoryActions();
    };
    autosaveStatus_->setText(autosave_->status());
    connect(autosaveEnabled_, &QAction::toggled, this, [this](bool enabled) {
        autosave_->setEnabled(enabled);
        applicationSettings().setValue("recovery/enabled", enabled);
    });
    connect(autosaveNow_, &QAction::triggered, this, [this] { runAfterEdit([this] { autosave_->checkpointNow(); }); });
    connect(interval, &QAction::triggered, this, [this] {
        bool ok = false;
        auto value =
            QInputDialog::getInt(this, "자동 저장 간격", "변경 중 자동 저장 요청 간격(초) · 멈춘 뒤 2초에도 저장",
                                 autosave_->intervalMs() / 1000, 5, 300, 5, &ok);
        if (ok) {
            autosave_->setIntervalMs(value * 1000);
            applicationSettings().setValue("recovery/intervalSeconds", value);
        }
    });
    connect(recover_, &QAction::triggered, this, [this] { runAfterEdit([this] { showRecovery(); }); });
    QTimer::singleShot(0, this, [this] { showRecovery(true); });
}
void MainWindow::showRecovery(bool startup) {
    if (!recoveryStore_)
        return;
    if (startup) {
        struct Result {
            RecoveryScan scan;
            QString error;
        };
        auto *watcher = new QFutureWatcher<Result>(this);
        connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher] {
            auto result = watcher->result();
            watcher->deleteLater();
            if (!result.error.isEmpty()) {
                autosaveStatus_->setText("복구본 검색 실패: " + result.error);
                return;
            }
            if (result.scan.candidates.empty())
                return;
            if (QApplication::activeModalWidget() || editController_->busy()) {
                recover_->setText(QString("복구본 선택… (%1세대)").arg(result.scan.candidates.size()));
                statusBar()->showMessage("이전 복구본이 있습니다. 자동 저장·복구 메뉴에서 확인하세요.");
                return;
            }
            RecoveryDialog dialog(*recoveryStore_, this, std::move(result.scan));
            if (dialog.exec() != QDialog::Accepted)
                return;
            auto claim = dialog.takeClaim();
            if (!claim || !confirmChanges())
                return;
            try {
                document_.recover(claim->snapshot);
                autosave_->adoptRecovered(std::move(claim));
                refresh();
            } catch (const std::exception &e) {
                report(e);
            }
        });
        watcher->setFuture(QtConcurrent::run([store = *recoveryStore_] {
            Result result;
            try {
                result.scan = store.scan();
            } catch (const std::exception &e) {
                result.error = qErrorText(e);
            }
            return result;
        }));
        return;
    }
    RecoveryDialog dialog(*recoveryStore_, this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    auto claim = dialog.takeClaim();
    if (!claim || !confirmChanges())
        return;
    try {
        document_.recover(claim->snapshot);
        autosave_->adoptRecovered(std::move(claim));
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
} // namespace jobs
