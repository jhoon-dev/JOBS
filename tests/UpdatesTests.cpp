#include "updates/UpdateController.h"
#include "ui/UpdateUi.h"
#include <QAction>
#include <QCheckBox>
#include <QDialog>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QTemporaryDir>
#include <QTest>

using namespace jobs;
namespace {
class Store final : public StoreUpdateClient {
public:
    Completion callback;
    int calls = 0, cancellations = 0;
    quintptr owner = 0;
    void check(quintptr window, Completion completion) override {
        ++calls;
        owner = window;
        callback = std::move(completion);
    }
    void cancel() override {
        ++cancellations;
        callback = {};
    }
    void result(StoreUpdateResult result) {
        auto done = std::move(callback);
        if (done)
            done(std::move(result));
    }
};
UpdateConfig config(const QTemporaryDir &dir, UpdateChannel channel = UpdateChannel::Store) {
    UpdateConfig value;
    value.currentVersion = "0.2.0";
    value.channel = channel;
    value.stateDirectory = dir.filePath("state");
    value.settingsFile = dir.filePath("preferences.ini");
    return value;
}
} // namespace
class UpdatesTests final : public QObject {
    Q_OBJECT
private slots:
    void unpackagedBuildDoesNotUseReleaseUpdates() {
        const auto options = updateConfig();
#ifdef JOBS_MAC_APP_STORE
        QCOMPARE(options.channel, UpdateChannel::MacAppStore);
#else
        QCOMPARE(options.channel, UpdateChannel::Unpackaged);
#endif
        QCOMPARE(options.currentVersion, appVersion());
    }
    void onlyStorePackagesQueryTheStore() {
        QTemporaryDir dir;
        for (auto channel :
             {UpdateChannel::Unpackaged, UpdateChannel::DevelopmentPackage, UpdateChannel::MacAppStore}) {
            auto client = std::make_unique<Store>();
            auto *store = client.get();
            UpdateController controller(config(dir, channel), nullptr, std::move(client));
            controller.setAutomatic(true);
            QVERIFY(!controller.automatic());
            bool requestedWindow = false;
            controller.scheduleStartup([&] {
                requestedWindow = true;
                return quintptr(0);
            });
            controller.check();
            QCOMPARE(controller.state(), UpdateState::Unsupported);
            QVERIFY(!controller.available());
            QCOMPARE(store->calls, 0);
            QVERIFY(!requestedWindow);
            QVERIFY(controller.lastChecked().isEmpty());
        }
    }
    void storeResultsAndManualRetry() {
        QTemporaryDir dir;
        auto client = std::make_unique<Store>();
        auto *store = client.get();
        UpdateController controller(config(dir), nullptr, std::move(client));
        controller.setAutomatic(false);
        controller.check(123);
        QCOMPARE(store->owner, quintptr(123));
        QCOMPARE(controller.state(), UpdateState::Checking);
        controller.check(456);
        QCOMPARE(store->calls, 1);
        store->result({true, 2, {}});
        QCOMPARE(controller.state(), UpdateState::Available);
        QVERIFY(controller.available());
        QVERIFY(!controller.lastChecked().isEmpty());
        controller.check();
        store->result({true, 0, {}});
        QCOMPARE(controller.state(), UpdateState::Current);
        controller.check();
        store->result({false, 0, "not published"});
        QCOMPARE(controller.state(), UpdateState::Failed);
        QVERIFY(!controller.available());
        QVERIFY(controller.status().contains("not published"));
        controller.check();
        store->result({true, 1, {}});
        QCOMPARE(controller.state(), UpdateState::Available);
    }
    void cancelAndTimeoutIgnoreLateResults() {
        QTemporaryDir dir;
        auto client = std::make_unique<Store>();
        auto *store = client.get();
        auto options = config(dir);
        options.checkTimeoutMs = 10;
        UpdateController controller(options, nullptr, std::move(client));
        controller.setAutomatic(false);
        controller.check();
        auto cancelledResult = store->callback;
        controller.cancel();
        QCOMPARE(controller.state(), UpdateState::Cancelled);
        cancelledResult({true, 4, {}});
        QCOMPARE(controller.state(), UpdateState::Cancelled);
        QVERIFY(controller.lastChecked().isEmpty());
        controller.check();
        auto timedOutResult = store->callback;
        QTRY_COMPARE(controller.state(), UpdateState::Failed);
        QVERIFY(controller.status().contains("초과"));
        controller.check();
        timedOutResult({true, 9, {}});
        QCOMPARE(controller.state(), UpdateState::Checking);
        store->result({true, 0, {}});
        QCOMPARE(controller.state(), UpdateState::Current);
        QVERIFY(store->cancellations >= 2);
    }
    void sharedStartupAndPersistentPreference() {
        QTemporaryDir dir;
        {
            UpdateController disabled(config(dir));
            disabled.setAutomatic(false);
        }
        auto client = std::make_unique<Store>();
        auto *store = client.get();
        UpdateController controller(config(dir), nullptr, std::move(client));
        QVERIFY(!controller.automatic());
        controller.setAutomatic(true);
        QMainWindow first, second;
        attachUpdateUi(first, controller);
        attachUpdateUi(second, controller);
        first.show();
        second.show();
        QTRY_COMPARE_WITH_TIMEOUT(store->calls, 1, 4000);
        store->result({true, 1, {}});
        QVERIFY(first.findChild<QWidget *>("updateBanner")->isVisible());
        QVERIFY(second.findChild<QWidget *>("updateBanner")->isVisible());
        first.findChild<QAction *>("applicationUpdates")->trigger();
        auto *dialog = first.findChild<QDialog *>("applicationUpdateDialog");
        QVERIFY(dialog);
        auto *toggle = dialog->findChild<QCheckBox *>("updateAutomatic");
        toggle->click();
        QVERIFY(!controller.automatic());
        QCOMPARE(QSettings(config(dir).settingsFile, QSettings::IniFormat).value("updates/checkAtStartup").toBool(),
                 false);
    }
    void storeButtonsOpenOnlyStoreUris() {
        QTemporaryDir dir;
        for (auto channel : {UpdateChannel::Store, UpdateChannel::MacAppStore}) {
            auto client = std::make_unique<Store>();
            auto *store = client.get();
            UpdateController controller(config(dir, channel), nullptr, std::move(client));
            controller.setAutomatic(false);
            QMainWindow window;
            QUrl opened;
            attachUpdateUi(window, controller, {[&](const QUrl &url) {
                               opened = url;
                               return true;
                           }});
            window.findChild<QAction *>("applicationUpdates")->trigger();
            auto *automatic = window.findChild<QCheckBox *>("updateAutomatic");
            QCOMPARE(automatic->isEnabled(), channel == UpdateChannel::Store);
            window.findChild<QPushButton *>("installApplicationUpdate")->click();
            QCOMPARE(opened, channel == UpdateChannel::Store ? storeUpdatesUrl() : macAppStoreUrl());
            QVERIFY(!window.findChild<QPushButton *>("applicationReleasePage"));
            if (channel == UpdateChannel::MacAppStore) {
                window.findChild<QPushButton *>("checkApplicationUpdate")->click();
                QCOMPARE(store->calls, 0);
            }
        }
    }
    void developmentUiHasNoDownloadOrInstallHandoff() {
        QTemporaryDir dir;
        for (auto channel : {UpdateChannel::Unpackaged, UpdateChannel::DevelopmentPackage}) {
            UpdateController controller(config(dir, channel));
            QMainWindow window;
            bool opened = false;
            attachUpdateUi(window, controller, {[&](const QUrl &) {
                               opened = true;
                               return true;
                           }});
            window.show();
            window.findChild<QAction *>("applicationUpdates")->trigger();
            QVERIFY(!window.findChild<QCheckBox *>("updateAutomatic")->isEnabled());
            QVERIFY(!window.findChild<QPushButton *>("installApplicationUpdate")->isVisible());
            QVERIFY(!window.findChild<QPushButton *>("applicationReleasePage"));
            window.findChild<QPushButton *>("checkApplicationUpdate")->click();
            QCOMPARE(controller.state(), UpdateState::Unsupported);
            QVERIFY(!opened);
            QVERIFY(!controller.available());
        }
    }
};
QTEST_MAIN(UpdatesTests)
#include "UpdatesTests.moc"
