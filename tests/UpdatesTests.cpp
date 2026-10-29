#include "updates/UpdateController.h"
#include "ui/UpdateUi.h"
#include "ui/DesktopWorkspace.h"
#include "ui/MainWindow.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QDialog>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QNetworkReply>
#include <QPushButton>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <cstring>
#include <deque>
#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace jobs;
namespace {
const QByteArray installer("MZ deterministic installer fixture");
QJsonObject releaseObject(QString version = "0.2.1") {
    const QString name = "JOBS-" + version + "-Setup-win64-abcdef12.exe";
    return {
        {"draft", false},
        {"prerelease", false},
        {"tag_name", "v" + version},
        {"html_url", "https://github.com/jhoon-dev/JOBS/releases/tag/v" + version},
        {"body", "<script>plain text release notes</script>"},
        {"assets",
         QJsonArray{QJsonObject{
             {"name", name},
             {"size", installer.size()},
             {"browser_download_url", "https://github.com/jhoon-dev/JOBS/releases/download/v" + version + "/" + name},
             {"digest", "sha256:" + QString::fromLatin1(
                                        QCryptographicHash::hash(installer, QCryptographicHash::Sha256).toHex())}}}}};
}
QByteArray catalog(QString version = "0.2.1") {
    return QJsonDocument(releaseObject(version)).toJson();
}
struct Response {
    Response(QByteArray value = {}, int code = 200, QUrl target = {}, int milliseconds = 0, bool pending = false)
        : data(std::move(value)), status(code), redirect(std::move(target)), delay(milliseconds), hang(pending) {}
    QByteArray data;
    int status = 200;
    QUrl redirect;
    int delay = 0;
    bool hang = false;
};
class Reply final : public QNetworkReply {
public:
    Reply(const QNetworkRequest &request, Response response, QObject *parent)
        : QNetworkReply(parent), data_(std::move(response.data)) {
        setRequest(request);
        setUrl(request.url());
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
        setAttribute(QNetworkRequest::HttpStatusCodeAttribute, response.status);
        if (!response.redirect.isEmpty())
            setAttribute(QNetworkRequest::RedirectionTargetAttribute, response.redirect);
        if (!response.hang)
            QTimer::singleShot(response.delay, this, [this] {
                if (!isFinished()) {
                    emit readyRead();
                    setFinished(true);
                    emit finished();
                }
            });
    }
    void abort() override {
        if (!isFinished()) {
            setError(OperationCanceledError, "cancelled");
            setFinished(true);
            emit finished();
        }
    }
    qint64 bytesAvailable() const override {
        return data_.size() - position_ + QNetworkReply::bytesAvailable();
    }

protected:
    qint64 readData(char *output, qint64 maximum) override {
        const auto count = std::min(maximum, qint64(data_.size() - position_));
        if (!count)
            return -1;
        std::memcpy(output, data_.constData() + position_, size_t(count));
        position_ += count;
        return count;
    }

private:
    QByteArray data_;
    qint64 position_ = 0;
};
class Network final : public QNetworkAccessManager {
public:
    std::deque<Response> responses;
    QList<QNetworkRequest> requests;

protected:
    QNetworkReply *createRequest(Operation, const QNetworkRequest &request, QIODevice *) override {
        requests.push_back(request);
        auto response = responses.empty() ? Response{{}, 500} : responses.front();
        if (!responses.empty())
            responses.pop_front();
        return new Reply(request, std::move(response), this);
    }
};
class Store final : public StoreUpdateClient {
public:
    Completion callback;
    int calls = 0;
    void check(quintptr, Completion completion) override {
        ++calls;
        callback = std::move(completion);
    }
    void cancel() override {
        callback = {};
    }
    void result(StoreUpdateResult result) {
        auto done = std::move(callback);
        if (done)
            done(std::move(result));
    }
};
UpdateConfig config(const QTemporaryDir &dir) {
    UpdateConfig value;
    value.currentVersion = "0.2.0";
    value.installedWindows = true;
    value.stateDirectory = dir.filePath("state");
    value.settingsFile = dir.filePath("preferences.ini");
    return value;
}
void answer(QMessageBox::StandardButton button) {
    if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
        if (auto *selected = box->button(button))
            selected->click();
}
} // namespace
class UpdatesTests final : public QObject {
    Q_OBJECT
private slots:
    void detectionDoesNotRegisterInstallation() {
#ifdef Q_OS_WIN
        const auto exists = [] {
            HKEY key = nullptr;
            const auto result = RegOpenKeyExW(HKEY_CURRENT_USER,
                                              L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\ToolMoa.JOBS",
                                              0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
            if (key)
                RegCloseKey(key);
            return result == ERROR_SUCCESS;
        };
        const bool before = exists();
        const auto options = updateConfig();
        QCOMPARE(exists(), before);
        QVERIFY(!options.installedWindows);
#else
        QVERIFY(!updateConfig().installedWindows);
#endif
    }
    void versions() {
        QVERIFY(newerVersion("0.10.0", "0.2.9"));
        QVERIFY(!newerVersion("0.2.0", "0.2.0"));
        QVERIFY(!newerVersion("0.1.9", "0.2.0"));
        for (const auto &value : {"v0.2.1", "0.2.1-beta", "0.2.1.0", "01.2.1", "0.65536.0", "-1.0.0"})
            QVERIFY_EXCEPTION_THROWN(newerVersion(QString::fromLatin1(value), "0.2.0"), std::exception);
    }
    void rejectsCatalogChanges() {
        for (const auto &field : {"draft", "prerelease"}) {
            auto object = releaseObject();
            object[field] = true;
            QVERIFY_EXCEPTION_THROWN(parseRelease(QJsonDocument(object).toJson(), "0.2.0"), std::exception);
        }
        auto object = releaseObject();
        object["html_url"] = "https://github.com/other/JOBS/releases/tag/v0.2.1";
        QVERIFY_EXCEPTION_THROWN(parseRelease(QJsonDocument(object).toJson(), "0.2.0"), std::exception);
        object = releaseObject();
        auto assets = object["assets"].toArray();
        assets.push_back(assets[0]);
        object["assets"] = assets;
        QVERIFY_EXCEPTION_THROWN(parseRelease(QJsonDocument(object).toJson(), "0.2.0"), std::exception);
        QVERIFY_EXCEPTION_THROWN(parseRelease(QByteArray(MaximumCatalogBytes + 1, 'x'), "0.2.0"), std::exception);
        QVERIFY_EXCEPTION_THROWN(parseRelease("not-json", "0.2.0"), std::exception);
        QVERIFY(!downloadUrlAllowed(QUrl("http://github.com/jhoon-dev/JOBS/releases/download/a/b")));
        QVERIFY(!downloadUrlAllowed(QUrl("https://github.com.evil.test/jhoon-dev/JOBS/releases/download/a/b")));
        QVERIFY(!downloadUrlAllowed(QUrl("https://user:pass@github.com/jhoon-dev/JOBS/releases/download/a/b")));
        QVERIFY(!downloadUrlAllowed(QUrl("https://evil.test/update.exe")));
    }
    void newerAndOlder() {
        for (const QString &version : {QString("0.2.1"), QString("0.2.0"), QString("0.1.9")}) {
            QTemporaryDir dir;
            Network network;
            network.responses.push_back({catalog(version)});
            UpdateController controller(config(dir), nullptr, &network);
            controller.check();
            controller.check();
            QTRY_VERIFY(!controller.busy());
            QCOMPARE(network.requests.size(), 1);
            QCOMPARE(controller.state(), version == "0.2.1" ? UpdateState::Available : UpdateState::Current);
            QVERIFY(!controller.lastChecked().isEmpty());
            QVERIFY(!network.requests[0].hasRawHeader("Authorization"));
            QCOMPARE(network.requests[0].url(), updateCatalogUrl());
        }
    }
    void offlineUnpublishedAndRateLimit() {
        for (int code : {0, 404, 403, 429, 500}) {
            QTemporaryDir dir;
            Network network;
            network.responses.push_back({{}, code});
            UpdateController controller(config(dir), nullptr, &network);
            controller.check();
            QTRY_VERIFY(!controller.busy());
            QCOMPARE(controller.state(), code == 404 ? UpdateState::Unpublished : UpdateState::Failed);
        }
    }
    void cancelAndTimeout() {
        QTemporaryDir dir;
        Network network;
        network.responses.push_back({catalog(), 200, {}, 100});
        network.responses.push_back({{}, 200, {}, 0, true});
        auto options = config(dir);
        options.checkTimeoutMs = 30;
        UpdateController controller(options, nullptr, &network);
        controller.check();
        controller.cancel();
        QTest::qWait(120);
        QCOMPARE(controller.state(), UpdateState::Cancelled);
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Failed);
        QVERIFY(controller.status().contains("시간"));
    }
    void downloadIntegrityAndTamper() {
        QTemporaryDir dir;
        Network network;
        network.responses = {{catalog()}, {installer}};
        UpdateController controller(config(dir), nullptr, &network);
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Available);
        QVERIFY(controller.canDownload());
        controller.download();
        QTRY_COMPARE(controller.state(), UpdateState::Ready);
        QVERIFY(controller.verifyReady());
        QFile file(controller.installerPath());
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("bad");
        file.close();
        QVERIFY(!controller.verifyReady());
    }
    void rejectsBadDigestAndLength() {
        for (const QByteArray &data : {QByteArray(installer.size(), 'x'), installer + "extra", installer.left(5)}) {
            QTemporaryDir dir;
            Network network;
            network.responses = {{catalog()}, {data}};
            UpdateController controller(config(dir), nullptr, &network);
            controller.check();
            QTRY_COMPARE(controller.state(), UpdateState::Available);
            controller.download();
            QTRY_COMPARE(controller.state(), UpdateState::Failed);
            QVERIFY(controller.installerPath().isEmpty());
        }
    }
    void redirectRestrictions() {
        QTemporaryDir dir;
        Network network;
        network.responses = {
            {catalog()}, {{}, 302, QUrl("https://release-assets.githubusercontent.com/fixture")}, {installer}};
        UpdateController controller(config(dir), nullptr, &network);
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Available);
        controller.download();
        QTRY_COMPARE(controller.state(), UpdateState::Ready);
        QCOMPARE(network.requests.size(), 3);
        network.responses = {{catalog()}, {{}, 302, QUrl("https://evil.test/fixture")}};
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Available);
        controller.download();
        QTRY_COMPARE(controller.state(), UpdateState::Failed);
        QCOMPARE(network.requests.size(), 5);
    }
    void cancelsPartialDownload() {
        QTemporaryDir dir;
        Network network;
        network.responses = {{catalog()}, {installer, 200, {}, 200}};
        UpdateController controller(config(dir), nullptr, &network);
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Available);
        controller.download();
        const auto path = controller.installerPath();
        controller.cancel();
        QVERIFY(!QFile::exists(path));
        QTest::qWait(220);
        QCOMPARE(controller.state(), UpdateState::Cancelled);
    }
    void missingDigestAndDeveloperExecution() {
        auto object = releaseObject();
        auto asset = object["assets"].toArray()[0].toObject();
        asset.remove("digest");
        object["assets"] = QJsonArray{asset};
        QTemporaryDir dir;
        Network network;
        network.responses = {{QJsonDocument(object).toJson()}, {catalog()}};
        UpdateController controller(config(dir), nullptr, &network);
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Available);
        QVERIFY(!controller.canDownload());
        auto options = config(dir);
        options.installedWindows = false;
        UpdateController developer(options, nullptr, &network);
        developer.check();
        QTRY_COMPARE(developer.state(), UpdateState::Available);
        QVERIFY(!developer.canDownload());
    }
    void storeRoutingAndDevelopmentPackage() {
        QTemporaryDir dir;
        Network network;
        auto options = config(dir);
        options.channel = UpdateChannel::Store;
        auto client = std::make_unique<Store>();
        auto *store = client.get();
        UpdateController controller(options, nullptr, &network, std::move(client));
        controller.check();
        QCOMPARE(store->calls, 1);
        QCOMPARE(network.requests.size(), 0);
        store->result({true, 2, {}});
        QCOMPARE(controller.state(), UpdateState::Available);
        QVERIFY(!controller.canDownload());
        controller.check();
        store->result({true, 0, {}});
        QCOMPARE(controller.state(), UpdateState::Current);
        controller.check();
        store->result({false, 0, "unavailable"});
        QCOMPARE(controller.state(), UpdateState::Failed);
        options.channel = UpdateChannel::DevelopmentPackage;
        UpdateController development(options, nullptr, &network);
        development.check();
        QCOMPARE(development.state(), UpdateState::Unsupported);
        QCOMPARE(network.requests.size(), 0);
    }
    void settingsAndStartupSharing() {
        QTemporaryDir dir;
        Network network;
        network.responses = {{catalog()}};
        {
            UpdateController disabled(config(dir), nullptr, &network);
            disabled.setAutomatic(false);
            disabled.scheduleStartup([] { return quintptr(0); });
            QTest::qWait(3100);
            QCOMPARE(network.requests.size(), 0);
            disabled.setAutomatic(true);
        }
        UpdateController controller(config(dir), nullptr, &network);
        QMainWindow first, second;
        attachUpdateUi(first, controller);
        attachUpdateUi(second, controller);
        first.show();
        second.show();
        QTRY_COMPARE_WITH_TIMEOUT(controller.state(), UpdateState::Available, 4000);
        QCOMPARE(network.requests.size(), 1);
        QVERIFY(first.findChild<QWidget *>("updateBanner")->isVisible());
        QVERIFY(second.findChild<QWidget *>("updateBanner")->isVisible());
        first.findChild<QAction *>("applicationUpdates")->trigger();
        auto *box = first.findChild<QDialog *>("applicationUpdateDialog");
        QVERIFY(box);
        QVERIFY(box->isVisible());
        auto *toggle = box->findChild<QCheckBox *>("updateAutomatic");
        toggle->click();
        QVERIFY(!controller.automatic());
    }
    void storeButtonUsesStore() {
        QTemporaryDir dir;
        Network network;
        auto options = config(dir);
        options.channel = UpdateChannel::Store;
        UpdateController controller(options, nullptr, &network);
        QMainWindow window;
        QUrl opened;
        attachUpdateUi(window, controller, {{}, {}, {}, [&](const QUrl &url) {
                                                opened = url;
                                                return true;
                                            }});
        window.findChild<QAction *>("applicationUpdates")->trigger();
        window.findChild<QPushButton *>("installApplicationUpdate")->click();
        QCOMPARE(opened, storeUpdatesUrl());
        QCOMPARE(network.requests.size(), 0);
    }
    void installCancelAndLaunchFailure() {
        QTemporaryDir dir;
        Network network;
        network.responses = {{catalog()}, {installer}};
        UpdateController controller(config(dir), nullptr, &network);
        controller.setAutomatic(false);
        controller.check();
        QTRY_COMPARE(controller.state(), UpdateState::Available);
        controller.download();
        QTRY_COMPARE(controller.state(), UpdateState::Ready);
        bool prepare = false, succeed = false;
        int launched = 0, finished = 0;
        QMainWindow window;
        attachUpdateUi(window, controller,
                       {[&] { return prepare; },
                        [&] { ++finished; },
                        [&](const QString &) {
                            ++launched;
                            return succeed;
                        },
                        {}});
        window.show();
        window.findChild<QAction *>("applicationUpdates")->trigger();
        QTimer response;
        connect(&response, &QTimer::timeout, this, [] { answer(QMessageBox::Yes); });
        response.start(5);
        window.findChild<QPushButton *>("installApplicationUpdate")->click();
        QCOMPARE(launched, 0);
        prepare = true;
        window.findChild<QPushButton *>("installApplicationUpdate")->click();
        QCOMPARE(launched, 1);
        QCOMPARE(finished, 0);
        QVERIFY(window.isVisible());
        QVERIFY(window.findChild<QLabel *>("updateStatus")->text().contains("종료하지"));
        succeed = true;
        window.findChild<QPushButton *>("installApplicationUpdate")->click();
        QCOMPARE(launched, 2);
        QCOMPARE(finished, 1);
    }
    void documentCancelKeepsAllWindows() {
        QTemporaryDir dir;
        auto files = std::make_shared<JsonProjectStore>();
        const auto path = dir.filePath("Plan.jobs").toStdString();
        std::string id;
        {
            Document original(*files);
            original.create("update fixture", "2026-10-29", {"task"});
            original.setDuration(0, 480);
            original.saveAs(path);
            id = original.project()->documentId;
        }
        DesktopWorkspace workspace(files, {});
        auto *window = workspace.openProject(path);
        auto *document = workspace.document(id);
        document->renameTask(0, "unsaved");
        QTimer response;
        connect(&response, &QTimer::timeout, this, [] { answer(QMessageBox::Cancel); });
        response.start(5);
        QVERIFY(!workspace.prepareForUpdate());
        QVERIFY(document->dirty());
        QVERIFY(window->isVisible());
        QCOMPARE(QString::fromStdString(document->project()->tasks[0].name), QString("unsaved"));
        response.stop();
        disconnect(&response, nullptr, this, nullptr);
        connect(&response, &QTimer::timeout, this, [] { answer(QMessageBox::Ok); });
        response.start(5);
        for (auto *action : window->findChildren<QAction *>()) {
            if (action->text().startsWith("일정 계산(")) {
                action->trigger();
                break;
            }
        }
        QVERIFY(!workspace.prepareForUpdate());
        QVERIFY(window->isVisible());
    }
};
QTEST_MAIN(UpdatesTests)
#include "UpdatesTests.moc"
