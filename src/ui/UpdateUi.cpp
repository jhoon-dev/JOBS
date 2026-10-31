#include "ui/UpdateUi.h"
#include "updates/UpdateController.h"
#include "updates/UpdateInstaller.h"
#include <QApplication>
#include <QAction>
#include <QCheckBox>
#include <QCloseEvent>
#include <QDateTime>
#include <QDesktopServices>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QVBoxLayout>

namespace jobs {
namespace {
class InstallBarrier final : public QDialog {
public:
    explicit InstallBarrier(QWidget *parent) : QDialog(parent) {
        setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
        setWindowTitle("업데이트 설치 준비");
        setWindowModality(Qt::ApplicationModal);
        auto *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("문서 저장과 Windows 설치 실행을 확인하고 있습니다…", this));
        show();
    }
    void reject() override {}

protected:
    void closeEvent(QCloseEvent *event) override {
        event->ignore();
    }
};
} // namespace
void attachUpdateUi(QMainWindow &window, UpdateController &controller, UpdateUiHooks hooks) {
    if (!hooks.openUrl)
        hooks.openUrl = [](const QUrl &url) { return QDesktopServices::openUrl(url); };
    if (!hooks.launchInstaller)
        hooks.launchInstaller = launchUpdateInstaller;
    auto *menu = window.menuBar()->addMenu("설정(&S)");
    auto *action = menu->addAction("업데이트…");
    action->setObjectName("applicationUpdates");
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
        dialog->setWindowTitle("JOBS 업데이트");
        dialog->resize(650, 440);
        auto *layout = new QVBoxLayout(dialog);
        auto *version = new QLabel("현재 앱 버전: " + controller.config().currentVersion, dialog);
        const auto channel = controller.config().channel;
        auto *source =
            new QLabel(channel == UpdateChannel::Store         ? "업데이트 경로: Microsoft Store"
                       : channel == UpdateChannel::MacAppStore ? "업데이트 경로: Mac App Store"
                       : channel == UpdateChannel::DevelopmentPackage ? "업데이트 경로: 개발용 MSIX · Store 미연결"
                                                                      : "업데이트 경로: GitHub 정식 릴리스",
                       dialog);
        auto *automatic = new QCheckBox("앱 시작 시 새 버전 확인", dialog);
        automatic->setObjectName("updateAutomatic");
        auto *privacy =
            new QLabel("확인 시 배포 서버에 연결합니다. 프로젝트 내용이나 파일 경로는 전송하지 않습니다.", dialog);
        privacy->setWordWrap(true);
        if (channel == UpdateChannel::MacAppStore)
            privacy->setText("업데이트는 App Store에서 관리합니다. 앱 자체의 외부 서버 버전 확인은 사용하지 않습니다.");
        auto *status = new QLabel(dialog);
        status->setObjectName("updateStatus");
        status->setWordWrap(true);
        status->setTextFormat(Qt::PlainText);
        auto *last = new QLabel(dialog);
        auto *notes = new QPlainTextEdit(dialog);
        notes->setReadOnly(true);
        notes->setObjectName("updateNotes");
        auto *progress = new QProgressBar(dialog);
        progress->setRange(0, 100);
        auto *buttons = new QHBoxLayout;
        auto *check = new QPushButton("지금 업데이트 확인", dialog);
        check->setObjectName("checkApplicationUpdate");
        auto *cancel = new QPushButton("취소", dialog);
        cancel->setObjectName("cancelApplicationUpdate");
        auto *install = new QPushButton(dialog);
        install->setObjectName("installApplicationUpdate");
        auto *close = new QPushButton("닫기", dialog);
        auto *page = new QPushButton("릴리스 보기", dialog);
        page->setObjectName("applicationReleasePage");
        for (auto *widget :
             std::initializer_list<QWidget *>{version, source, automatic, privacy, status, last, notes, progress})
            layout->addWidget(widget);
        for (auto *button : {check, cancel, install, page, close})
            buttons->addWidget(button);
        layout->addLayout(buttons);
        auto refresh = [&controller, automatic, status, last, notes, progress, check, cancel, install, page, channel] {
            QSignalBlocker blocker(automatic);
            automatic->setChecked(controller.automatic());
            automatic->setEnabled(channel != UpdateChannel::DevelopmentPackage &&
                                  channel != UpdateChannel::MacAppStore);
            status->setText(controller.status());
            auto time = QDateTime::fromString(controller.lastChecked(), Qt::ISODate);
            last->setText("마지막 확인: " +
                          (time.isValid() ? time.toLocalTime().toString("yyyy-MM-dd HH:mm:ss") : "없음"));
            notes->setPlainText(controller.release().notes);
            check->setEnabled(!controller.busy());
            cancel->setEnabled(controller.busy());
            progress->setVisible(controller.state() == UpdateState::Downloading);
            install->setEnabled(channel == UpdateChannel::Store || channel == UpdateChannel::MacAppStore
                                    ? !controller.busy()
                                    : controller.available());
            page->setVisible(channel == UpdateChannel::Release && controller.config().installedWindows);
            page->setEnabled(releasePageAllowed(controller.release().page));
            install->setText(channel == UpdateChannel::Store            ? "Microsoft Store에서 업데이트"
                             : channel == UpdateChannel::MacAppStore    ? "App Store에서 업데이트"
                             : controller.state() == UpdateState::Ready ? "저장 확인 후 설치…"
                             : controller.canDownload()                 ? "업데이트 다운로드"
                                                                        : "릴리스 페이지 열기");
        };
        QObject::connect(&controller, &UpdateController::changed, dialog, refresh);
        QObject::connect(&controller, &UpdateController::progress, dialog, [progress](qint64 received, qint64 total) {
            progress->setValue(total ? int(received * 100 / total) : 0);
        });
        QObject::connect(automatic, &QCheckBox::toggled, dialog,
                         [&controller](bool enabled) { controller.setAutomatic(enabled); });
        QObject::connect(check, &QPushButton::clicked, dialog,
                         [&controller, &window] { controller.check(quintptr(window.winId())); });
        QObject::connect(cancel, &QPushButton::clicked, &controller, &UpdateController::cancel);
        QObject::connect(close, &QPushButton::clicked, dialog, &QDialog::close);
        QObject::connect(page, &QPushButton::clicked, dialog, [&controller, status, hooks] {
            if (releasePageAllowed(controller.release().page) && !hooks.openUrl(controller.release().page))
                status->setText("릴리스 페이지를 열지 못했습니다.");
        });
        QObject::connect(install, &QPushButton::clicked, dialog, [&controller, dialog, status, hooks, channel] {
            if (channel == UpdateChannel::MacAppStore) {
                if (!hooks.openUrl(macAppStoreUrl()))
                    status->setText("App Store를 열지 못했습니다. macOS의 App Store를 직접 열어 주세요.");
            } else if (channel == UpdateChannel::Store) {
                if (!hooks.openUrl(storeUpdatesUrl()))
                    status->setText("Microsoft Store를 열지 못했습니다. 시작 메뉴에서 Store를 열어 주세요.");
            } else if (controller.state() == UpdateState::Ready) {
                if (!hooks.prepareInstall || !hooks.finishInstall) {
                    status->setText("설치된 JOBS의 프로젝트 창에서 업데이트를 진행해 주세요.");
                    return;
                }
                InstallBarrier barrier(dialog);
                if (QMessageBox::question(&barrier, "업데이트 설치",
                                          "열린 문서의 저장 여부를 확인한 뒤 JOBS를 종료하고 설치기를 실행합니다.",
                                          QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
                    return;
                if (hooks.prepareInstall && !hooks.prepareInstall())
                    return;
                if (!controller.verifyReady()) {
                    status->setText("설치 파일이 변경됐습니다. 다시 확인하고 다운로드해 주세요.");
                    return;
                }
                if (!hooks.launchInstaller(controller.installerPath())) {
                    status->setText("설치기를 실행하지 못했습니다. 앱은 종료하지 않았습니다.");
                    return;
                }
                controller.preserveInstaller();
                if (hooks.finishInstall)
                    hooks.finishInstall();
            } else if (controller.canDownload())
                controller.download();
            else if (releasePageAllowed(controller.release().page) && !hooks.openUrl(controller.release().page))
                status->setText("릴리스 페이지를 열지 못했습니다.");
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
