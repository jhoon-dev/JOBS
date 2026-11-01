#include "ui/UpdateUi.h"
#include "application/ProductIdentity.h"
#include "updates/UpdateController.h"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVBoxLayout>

namespace jobs {
void attachUpdateUi(QMainWindow &window, UpdateController &controller, UpdateUiHooks hooks) {
    if (!hooks.openUrl)
        hooks.openUrl = [](const QUrl &url) { return QDesktopServices::openUrl(url); };
    auto *menu = window.menuBar()->addMenu("설정(&S)");
    auto *action = menu->addAction("업데이트…");
    action->setObjectName("applicationUpdates");
    auto *about = menu->addAction("Project Planning Manager 정보…");
    about->setObjectName("applicationAbout");
    QObject::connect(about, &QAction::triggered, &window, [&window, &controller] {
        QMessageBox::about(&window, productDisplayName,
                           QString("<h3>%1</h3><p>버전 %2</p><p>개발자: %3<br>브랜드: %4</p>")
                               .arg(productDisplayName, controller.config().currentVersion.toHtmlEscaped(),
                                    developerName, productBrand));
    });
    auto *banner = new QWidget(&window);
    banner->setObjectName("updateBanner");
    auto *row = new QHBoxLayout(banner);
    row->setContentsMargins(4, 0, 4, 0);
    auto *text = new QLabel(banner);
    text->setTextFormat(Qt::PlainText);
    text->setMaximumWidth(450);
    text->setWordWrap(true);
    auto *show = new QPushButton("업데이트…", banner);
    row->addWidget(text);
    row->addWidget(show);
    window.statusBar()->addPermanentWidget(banner);
    auto refreshBanner = [banner, text, &controller] {
        banner->setVisible(controller.available());
        text->setText(controller.status());
    };
    QObject::connect(&controller, &UpdateController::changed, banner, refreshBanner);
    refreshBanner();
    QObject::connect(show, &QPushButton::clicked, action, &QAction::trigger);
    QObject::connect(action, &QAction::triggered, &window, [&window, &controller, hooks] {
        if (auto *existing = window.findChild<QDialog *>("applicationUpdateDialog")) {
            existing->show();
            existing->raise();
            return;
        }
        auto *dialog = new QDialog(&window);
        dialog->setObjectName("applicationUpdateDialog");
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->setWindowTitle("PPM 업데이트");
        dialog->resize(650, 270);
        auto *layout = new QVBoxLayout(dialog);
        auto *version = new QLabel("현재 앱 버전: " + controller.config().currentVersion, dialog);
        const auto channel = controller.config().channel;
        const bool windowsStore = channel == UpdateChannel::Store;
        const bool macStore = channel == UpdateChannel::MacAppStore;
        auto *source = new QLabel(windowsStore ? "업데이트 경로: Microsoft Store"
                                  : macStore   ? "업데이트 경로: Mac App Store"
                                               : "개발 실행본 · 스토어 업데이트 미지원",
                                  dialog);
        auto *automatic = new QCheckBox("앱 시작 시 새 버전 확인", dialog);
        automatic->setObjectName("updateAutomatic");
        auto *privacy = new QLabel(
            "업데이트 다운로드와 설치는 스토어에서 관리합니다. 프로젝트 내용이나 파일 경로는 전송하지 않습니다.",
            dialog);
        privacy->setWordWrap(true);
        auto *status = new QLabel(dialog);
        status->setObjectName("updateStatus");
        status->setWordWrap(true);
        status->setTextFormat(Qt::PlainText);
        auto *last = new QLabel(dialog);
        auto *buttons = new QHBoxLayout;
        auto *check = new QPushButton("지금 업데이트 확인", dialog);
        check->setObjectName("checkApplicationUpdate");
        auto *cancel = new QPushButton("취소", dialog);
        cancel->setObjectName("cancelApplicationUpdate");
        auto *install = new QPushButton(macStore ? "App Store 열기" : "Microsoft Store 열기", dialog);
        install->setObjectName("installApplicationUpdate");
        install->setVisible(windowsStore || macStore);
        auto *close = new QPushButton("닫기", dialog);
        for (auto *widget : std::initializer_list<QWidget *>{version, source, automatic, privacy, status, last})
            layout->addWidget(widget);
        for (auto *button : {check, cancel, install, close})
            buttons->addWidget(button);
        layout->addLayout(buttons);
        auto refresh = [&controller, automatic, status, last, check, cancel, install, windowsStore] {
            QSignalBlocker blocker(automatic);
            automatic->setChecked(controller.automatic());
            automatic->setEnabled(windowsStore);
            status->setText(controller.status());
            auto time = QDateTime::fromString(controller.lastChecked(), Qt::ISODate);
            last->setText("마지막 확인: " +
                          (time.isValid() ? time.toLocalTime().toString("yyyy-MM-dd HH:mm:ss") : "없음"));
            check->setEnabled(!controller.busy());
            cancel->setEnabled(controller.busy());
            install->setEnabled(!controller.busy());
        };
        QObject::connect(&controller, &UpdateController::changed, dialog, refresh);
        QObject::connect(automatic, &QCheckBox::toggled, dialog,
                         [&controller](bool enabled) { controller.setAutomatic(enabled); });
        QObject::connect(check, &QPushButton::clicked, dialog,
                         [&controller, &window] { controller.check(quintptr(window.winId())); });
        QObject::connect(cancel, &QPushButton::clicked, &controller, &UpdateController::cancel);
        QObject::connect(close, &QPushButton::clicked, dialog, &QDialog::close);
        QObject::connect(install, &QPushButton::clicked, dialog, [status, hooks, macStore, windowsStore] {
            if ((windowsStore || macStore) && !hooks.openUrl(macStore ? macAppStoreUrl() : storeUpdatesUrl()))
                status->setText("스토어를 열지 못했습니다. 운영체제의 스토어 앱을 직접 열어 주세요.");
        });
        refresh();
        dialog->show();
    });
    controller.scheduleStartup([owner = QPointer<QMainWindow>(&window)] {
        if (auto *active = qobject_cast<QMainWindow *>(QApplication::activeWindow()))
            return quintptr(active->winId());
        if (owner && owner->isVisible())
            return quintptr(owner->winId());
        for (auto *widget : QApplication::topLevelWidgets())
            if (auto *active = qobject_cast<QMainWindow *>(widget); active && active->isVisible())
                return quintptr(active->winId());
        return quintptr(0);
    });
}
} // namespace jobs
