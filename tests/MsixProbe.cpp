#include "BuildIdentity.h"
#include "application/Document.h"
#include "calculation/SharedResources.h"
#include "formatting/DiagnosticText.h"
#include "persistence/ApplicationStorage.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include "persistence/JsonWorkspaceStore.h"
#include "ui/DesktopWorkspace.h"
#include "ui/MainWindow.h"
#include "ui/WorkspaceDialog.h"
#include <QAction>
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSettings>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <algorithm>

namespace {
void check(bool value, const char *reason) {
    if (!value)
        throw std::logic_error(reason);
}
QJsonObject read(const QString &path) {
    QFile f(path);
    check(f.open(QIODevice::ReadOnly), "read probe state");
    return QJsonDocument::fromJson(f.readAll()).object();
}
void write(const QString &path, const QJsonObject &object) {
    QSaveFile f(path);
    const auto bytes = QJsonDocument(object).toJson();
    check(f.open(QIODevice::WriteOnly) && f.write(bytes) == bytes.size() && f.commit(), "write probe state");
}
void wait(const std::function<bool()> &ready) {
    QElapsedTimer time;
    time.start();
    while (!ready() && time.elapsed() < 30000)
        QTest::qWait(10);
    check(ready(), "packaged UI operation timed out");
}
std::string str(const QString &text) {
    return text.toUtf8().toStdString();
}
void fixture(jobs::JsonProjectStore &files, const std::string &path, const std::string &title, int count) {
    jobs::Document document(files);
    std::vector<std::string> names{"기획 검토", "설계",      "인허가",    "기반 공사",
                                   "구조 공사", "설비 설치", "시험 운전", "검수"};
    names.resize(std::size_t(count));
    document.create(title, "2026-10-26", names);
    const auto wbs = document.addWbs("프로젝트 실행");
    const auto resource = document.putResource({"", "현장팀", "labor", 100, {}});
    for (std::size_t i = 0; i < names.size(); ++i) {
        document.setDuration(i, 480 * std::int64_t(i % 3 + 1));
        document.assignWbs(i, wbs);
        if (i)
            document.setPredecessors(i, {document.project()->tasks[i - 1].id});
        document.setAssignments(document.project()->tasks[i].id, {{document.project()->tasks[i].id, resource, 100}});
    }
    document.saveAs(path);
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QCoreApplication::setOrganizationName("ToolMoa");
    QCoreApplication::setApplicationName("JOBS");
    const auto args = app.arguments();
    const bool local = args.contains("--local");
    QTemporaryDir temporary;
    QString root = temporary.path(), phase = "initial", nonce = "local-contract";
    const auto argument = [&](const QString &key) {
        const auto index = args.indexOf(key);
        check(index >= 0 && index + 1 < args.size(), "missing probe argument");
        return args[index + 1];
    };
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] { std::exit(3); });
    watchdog.start(90000);
    try {
        const auto storage = jobs::applicationStorage();
        if (!local) {
            root = argument("--root");
            phase = argument("--phase");
            nonce = argument("--nonce");
            check(QFileInfo(root).fileName().startsWith("msix-smoke-") && !QFileInfo(root).isSymLink(),
                  "not an owned MSIX test directory");
            check(read(QDir(root).filePath("test-owner.json"))["nonce"] == nonce, "probe ownership nonce mismatch");
            check(storage.packaged() && storage.packageFamilyName.startsWith("ToolMoa.JOBS.MsixDevelopment_"),
                  "probe did not run under development package identity");
            check(storage.settingsFile.contains("/LocalState/") &&
                      storage.recoveryDirectory.startsWith(storage.stateDirectory + "/"),
                  "package state not isolated in LocalState");
            QFile forbidden(QCoreApplication::applicationDirPath() + "/probe-readonly.tmp");
            check(!forbidden.open(QIODevice::WriteOnly), "package install location is writable");
        }
        check(phase == "initial" || phase == "verify", "unsupported probe phase");
        const auto project = QDir(root).filePath("Project.jobs"), other = QDir(root).filePath("Team.jobs"),
                   workspaceFile = QDir(root).filePath("Projects.jobsworkspace");
        auto files = std::make_shared<jobs::JsonProjectStore>();
        auto settings =
            local ? QSettings(QDir(root).filePath("settings.ini"), QSettings::IniFormat) : jobs::applicationSettings();
        const auto recoveryPath = local ? QDir(root).filePath("recovery") : storage.recoveryDirectory;
        jobs::JsonRecoveryStore recovery(recoveryPath);
        if (phase == "initial") {
            fixture(*files, str(project), "샘플 · 연구센터 구축", 8);
            fixture(*files, str(other), "샘플 · 운영 준비", 3);
            jobs::Document document(*files);
            document.open(str(project));
            const auto name = document.project()->tasks[0].name;
            document.renameTask(0, "편집 응답 확인");
            check(document.canUndo(), "undo missing");
            document.undo();
            check(document.project()->tasks[0].name == name, "undo changed source");
            settings.setValue("msix-probe/nonce", nonce);
            settings.setValue("recovery/intervalSeconds", 15);
            settings.sync();
            check(settings.status() == QSettings::NoError, "package settings write failed");
            document.renameTask(0, "복구 확인 · 기획 검토");
            auto session = recovery.newSession();
            session->checkpoint(document.recoverySnapshot("2026-10-29T03:00:00Z"));
            session.reset();
        } else {
            check(settings.value("msix-probe/nonce").toString() == nonce &&
                      settings.value("recovery/intervalSeconds").toInt() == 15,
                  "update lost package settings");
            const auto scan = recovery.scan();
            auto found = std::find_if(scan.candidates.begin(), scan.candidates.end(),
                                      [](const auto &value) { return value.valid; });
            check(found != scan.candidates.end(), "update lost recovery checkpoint");
            auto claim = recovery.claim(*found);
            jobs::Document recovered(*files);
            recovered.recover(claim->snapshot);
            check(recovered.project()->tasks[0].name == "복구 확인 · 기획 검토", "checkpoint recovery content changed");
        }
        {
            jobs::DesktopWorkspace workspace(files, {});
            auto *window = workspace.openProject(str(project));
            workspace.openProject(str(other));
            for (auto *action : window->findChildren<QAction *>())
                if (action->text().startsWith("일정 계산(")) {
                    action->trigger();
                    break;
                }
            const auto first = workspace.snapshots()[0]->documentId;
            auto *source = workspace.document(first);
            wait([&] {
                return source->calculation() && source->calculation()->status == jobs::CalculationStatus::Valid;
            });
            auto config = workspace.configuration();
            config.resources = {{"shared", "공유 현장팀"}};
            for (const auto &snapshot : workspace.snapshots())
                config.bindings.push_back({snapshot->documentId, snapshot->resources[0].id, "shared"});
            workspace.applyConfiguration(config);
            if (phase == "initial")
                jobs::JsonWorkspaceStore(*files).save(str(workspaceFile), config, {});
            else
                check(jobs::JsonWorkspaceStore(*files).load(str(workspaceFile)).workspace.bindings == config.bindings,
                      "saved workspace changed after upgrade");
            window->resize(1440, 900);
            window->show();
            QTest::qWait(100);
            if (!local)
                check(window->grab().save(QDir(root).filePath("01-schedule.png")), "schedule screenshot failed");
            jobs::WorkspaceDialog dialog(workspace);
            dialog.resize(1440, 900);
            dialog.show();
            dialog.startAnalysis();
            wait([&] { return !dialog.busy(); });
            check(dialog.result() && !dialog.result()->days.empty() &&
                      dialog.result()->status == jobs::CalculationStatus::Conflict,
                  "packaged shared load failed");
            if (!local) {
                dialog.findChild<QTabWidget *>()->setCurrentIndex(2);
                QTest::qWait(100);
                check(dialog.grab().save(QDir(root).filePath("02-shared-resources.png")),
                      "workspace screenshot failed");
            }
        }
        QJsonObject result{{"status", "success"},
                           {"phase", phase},
                           {"packaged", storage.packaged()},
                           {"package_family", storage.packageFamilyName},
                           {"state_directory", storage.stateDirectory},
                           {"settings_file", settings.fileName()},
                           {"recovery_directory", recoveryPath},
                           {"nonce", nonce},
                           {"project", project},
                           {"workspace", workspaceFile},
                           {"source_revision", QString::fromUtf8(JOBS_PERF_REVISION)}};
        write(QDir(root).filePath("probe-" + phase + ".json"), result);
        std::cout << "PASS: " << phase.toStdString()
                  << " create/save/open, calculate, undo, workspace, settings and checkpoint under "
                  << storage.packageFamilyName.toStdString() << '\n';
    } catch (const std::exception &error) {
        const auto message = QString::fromStdString(jobs::errorText(error));
        try {
            write(QDir(root).filePath("probe-" + phase + ".json"), {{"status", "failed"}, {"error", message}});
        } catch (...) {
        }
        std::cerr << message.toStdString() << '\n';
        return 1;
    }
}
