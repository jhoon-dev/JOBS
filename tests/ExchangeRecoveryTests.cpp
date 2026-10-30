#include "application/Document.h"
#include "formatting/DiagnosticText.h"
#include "persistence/ExchangePackage.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include "persistence/PrimaveraXer.h"
#include "persistence/ProjectXml.h"
#include "ui/ExchangeOperation.h"
#include "ui/MainWindow.h"
#include "ui/RecoveryDialog.h"
#include <QAction>
#include <QApplication>
#include <QDialogButtonBox>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>
#include <QTimer>
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
QByteArray read(const QString &path) {
    QFile file(path);
    check(file.open(QIODevice::ReadOnly), "test read failed");
    return file.readAll();
}
void write(const QString &path, const QByteArray &bytes) {
    QFile file(path);
    check(file.open(QIODevice::WriteOnly) && file.write(bytes) == bytes.size(), "test write failed");
}
void onGui(QWidget &parent, const std::function<void()> &function) {
    check(QThread::currentThread() != qApp->thread(), "exchange work ran on GUI thread");
    QMetaObject::invokeMethod(&parent, function, Qt::BlockingQueuedConnection);
}
void cancelDialog() {
    auto *dialog = QApplication::activeModalWidget();
    if (!dialog || dialog->objectName() != "exchangeOperation")
        std::_Exit(2);
    dialog->findChild<QPushButton *>("exchangeCancel")->click();
}
void publication(const QTemporaryDir &directory, jobs::JsonProjectStore &store) {
    jobs::Document doc(store);
    doc.create("Publication", "2026-10-26", {"Task"});
    doc.setDuration(0, 60);
    doc.saveAs(directory.filePath("Source.jobs").toStdString());
    auto exchange = jobs::exportProjectXml(*doc.project());
    const auto source = read(QString::fromStdString(doc.path()));
    const auto original = *doc.project();
    QWidget parent;
    const auto publish = [&](const QString &path, const std::function<void()> &boundary) {
        return jobs::runExchangeOperation(
            &parent, doc, "Publication boundary test",
            [&] {
                jobs::publishPlanExchange(exchange.project, exchange.xml, exchange.report, false, "xml", path,
                                          [&] { onGui(parent, boundary); });
            },
            true);
    };
    const auto cancelled = directory.filePath("cancelled");
    check(!publish(cancelled, cancelDialog) && !QFileInfo::exists(cancelled) && *doc.project() == original &&
              read(QString::fromStdString(doc.path())) == source,
          "pre-publication cancellation changed source or published a partial directory");
    bool stale = false;
    const auto changed = directory.filePath("changed");
    try {
        publish(changed, [&] { doc.renameTask(0, "Concurrent edit"); });
    } catch (const jobs::AppError &error) {
        stale = error.code() == "EXCHANGE_SOURCE_CHANGED";
    }
    check(stale && !QFileInfo::exists(changed) && doc.project()->tasks[0].name == "Concurrent edit",
          "source edit was overwritten or published");
    doc.open(doc.path());
    stale = false;
    const auto reloaded = directory.filePath("reloaded");
    try {
        publish(reloaded, [&] { doc.open(doc.path()); });
    } catch (const jobs::AppError &error) {
        stale = error.code() == "EXCHANGE_SOURCE_CHANGED";
    }
    check(stale && !QFileInfo::exists(reloaded), "same ID/revision reload accepted old publication");
    check(publish(directory.filePath("normal-save"), [&] { doc.save(); }),
          "normal save incorrectly cancelled exchange");
    const auto completed = directory.filePath("completed");
    check(jobs::runExchangeOperation(
              &parent, doc, "Late cancellation",
              [&] {
                  jobs::publishProjectXml(exchange, completed);
                  onGui(parent, cancelDialog);
              },
              true) &&
              QFileInfo::exists(QDir(completed).filePath("source.jobs")),
          "late cancellation discarded a fully published directory");
    const auto existing = directory.filePath("existing");
    check(QDir().mkdir(existing), "test destination creation failed");
    write(QDir(existing).filePath("sentinel"), "preserved");
    bool rejected = false;
    try {
        publish(existing, [] {});
    } catch (const jobs::AppError &) {
        rejected = true;
    }
    check(rejected && read(QDir(existing).filePath("sentinel")) == "preserved", "existing destination overwritten");
    check(QDir(directory.path()).entryList({".jobs-exchange-*"}, QDir::Dirs | QDir::Hidden).isEmpty(),
          "cancelled/failed exchange leaked staged files");
}
void recoveryErrors(const QTemporaryDir &directory, jobs::JsonProjectStore &files) {
    jobs::Document doc(files);
    doc.create("Recovery", "2026-10-26", {"Task"});
    jobs::JsonRecoveryStore recovery(directory.filePath("recovery"));
    for (int kind = 0; kind < 3; ++kind) {
        auto session = recovery.newSession();
        const auto id = session->id();
        session->checkpoint(doc.recoverySnapshot("2026-10-30T00:00:00Z"));
        session.reset();
        const auto path = QDir(recovery.directory()).filePath(id + "/checkpoint-000000001.jobrecovery");
        auto root = QJsonDocument::fromJson(read(path)).object();
        if (kind == 1)
            root["schemaVersion"] = 99;
        else
            root["sha256"] = "bad";
        const auto damaged = kind == 0 ? QByteArray("broken") : QJsonDocument(root).toJson();
        write(path, damaged);
        const auto scan = recovery.scan();
        const auto found = std::find_if(scan.candidates.begin(), scan.candidates.end(),
                                        [&](const auto &item) { return item.sessionId == id; });
        check(found != scan.candidates.end() && !found->valid && found->failure &&
                  QFileInfo(QString::fromStdString(found->failure->path)).canonicalFilePath() ==
                      QFileInfo(path).canonicalFilePath() &&
                  found->error.contains("복구"),
              "recovery diagnostic lost code, explanation or path");
        const auto expected = kind == 0   ? "FILE_JSONRECOVERYSTORE_015"
                              : kind == 1 ? "FILE_JSONRECOVERYSTORE_016"
                                          : "FILE_JSONRECOVERYSTORE_017";
        check(found->failure->code == expected &&
                  jobs::formatDiagnostic(*found->failure, jobs::DiagnosticLanguage::English) !=
                      found->error.toStdString(),
              "recovery diagnostic lost its stable code or language rendering");
        jobs::RecoveryScan one;
        one.candidates.push_back(*found);
        jobs::RecoveryDialog dialog(recovery, nullptr, one);
        dialog.show();
        dialog.findChild<QTableWidget *>("recoveryList")->selectRow(0);
        const auto text = dialog.findChild<QLabel *>("recoveryDetails")->text();
        check(text.contains(expected) && text.contains("복구") && text.contains("확인") &&
                  text.contains(QString::fromStdString(found->failure->path)) &&
                  !dialog.findChild<QPushButton *>("restoreRecovery")->isEnabled() && read(path) == damaged,
              "damaged recovery UI lacked explanation/guidance or modified the generation");
    }
    auto session = recovery.newSession();
    const auto id = session->id();
    session.reset();
    // Keep the directory alive after releasing its session lock.
    QDir().mkpath(QDir(recovery.directory()).filePath(id));
    for (int i = 0; i < 1001; ++i)
        write(QDir(recovery.directory()).filePath(id + "/extra-" + QString::number(i)), {});
    const auto scan = recovery.scan();
    const auto found = std::find_if(scan.candidates.begin(), scan.candidates.end(),
                                    [&](const auto &item) { return item.sessionId == id; });
    check(found != scan.candidates.end() && found->failure && found->failure->code == "FILE_JSONRECOVERYSTORE_037" &&
              found->error.contains("복구") && !found->failure->path.empty(),
          "session-level recovery failure lost its diagnostic");
}
jobs::Project largeProject() {
    jobs::Project project;
    project.documentId = "exchange-performance";
    project.name = "Exchange performance";
    project.startDate = "2026-10-26";
    project.calculationProfile = jobs::pdmProfile;
    for (int i = 0; i < 10000; ++i) {
        jobs::Task task{"T" + std::to_string(i), "Task " + std::to_string(i), 60, {}};
        if (i >= 100)
            task.predecessors = {"T" + std::to_string(i - 100)};
        project.tasks.push_back(std::move(task));
    }
    return project;
}
void exportUi(const QTemporaryDir &directory, jobs::JsonProjectStore &store, jobs::Project project, bool xer,
              bool cancelPublication) {
    auto baseline = jobs::prepareBaseline(project, "Approved", "PM", "Reviewed", "2026-10-30T00:00:00Z");
    baseline.id = store.newId();
    baseline.version = 1;
    project.baselines.push_back(std::move(baseline));
    ++project.revision;
    const auto name = QString(xer ? "xer" : "xml") + (cancelPublication ? "-cancel" : "-success");
    const auto path = directory.filePath(name + ".jobs");
    store.save(path.toStdString(), project, {});
    jobs::Document doc(store);
    doc.open(path.toStdString());
    const auto before = *doc.project();
    const auto source = read(path);
    jobs::MainWindow window(doc);
    window.show();
    QTest::qWait(20);
    const auto destination = directory.filePath(name + "-output");
    QTimer driver, heartbeat;
    bool preview = false, progress = false, cancelled = false;
    int beats = 0;
    qint64 maxBeat = 0;
    QElapsedTimer gap, elapsed;
    QObject::connect(&driver, &QTimer::timeout, [&] {
        auto *modal = QApplication::activeModalWidget();
        if (!modal)
            return;
        if (auto *error = qobject_cast<QMessageBox *>(modal)) {
            std::cerr << error->text().toStdString() << '\n';
            std::_Exit(2);
        }
        if (modal->objectName() == (xer ? "primaveraXerPreview" : "projectXmlPreview") && !preview) {
            preview = true;
            modal->findChild<QLineEdit *>(xer ? "xerDestination" : "xmlDestination")->setText(destination);
            modal->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        } else if (modal->objectName() == "exchangeOperation") {
            progress = true;
            if (preview && cancelPublication && !cancelled) {
                cancelled = true;
                cancelDialog();
            }
        }
    });
    QObject::connect(&heartbeat, &QTimer::timeout, [&] {
        ++beats;
        maxBeat = std::max(maxBeat, gap.restart());
    });
    driver.start(2);
    gap.start();
    elapsed.start();
    heartbeat.start(5);
    auto mainControl = std::make_shared<jobs::CalculationControl>();
    {
        jobs::EvaluationScope scope(mainControl);
        window.findChild<QAction *>(xer ? "exportPrimaveraXer" : "exportProjectXml")->trigger();
    }
    heartbeat.stop();
    driver.stop();
    const auto operationMs = elapsed.elapsed();
    check(preview && progress && beats > 1 && mainControl->checkpoints.load() == 0 && *doc.project() == before &&
              !doc.dirty() && read(path) == source,
          "exchange UI blocked on domain work or changed the source");
    check(QFileInfo::exists(destination) != cancelPublication, "cancel/success publication boundary failed");
    if (!cancelPublication)
        check(store.load(QDir(destination).filePath("source.jobs").toStdString()).project == before,
              "published source lost baseline input");
    std::cout << name.toStdString() << " tasks=" << project.tasks.size() << " total_ms=" << operationMs
              << " heartbeat_max_ms=" << maxBeat << " heartbeat_count=" << beats << " gui_checkpoints=0\n";
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    try {
        QTemporaryDir directory;
        check(directory.isValid(), "temporary directory failed");
        jobs::JsonProjectStore store;
        publication(directory, store);
        recoveryErrors(directory, store);
        auto project = largeProject();
        exportUi(directory, store, project, false, false);
        exportUi(directory, store, project, true, true);
        const auto option = app.arguments().indexOf("--xer");
        if (option >= 0 && option + 1 < app.arguments().size())
            project =
                jobs::importPrimaveraXer(jobs::readPrimaveraXer(app.arguments()[option + 1]), "review.xer").project;
        exportUi(directory, store, std::move(project), true, false);
        std::cout << "PASS: asynchronous XML/XER, cancellation/cleanup, stale source/session, late commit, source "
                     "preservation, 10k heartbeat and localized recovery diagnostics\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
