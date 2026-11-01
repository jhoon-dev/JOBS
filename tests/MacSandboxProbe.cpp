#include "ui/SandboxAccess.h"
#include "application/Document.h"
#include "persistence/ApplicationStorage.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonRecoveryStore.h"
#include "persistence/JsonWorkspaceStore.h"
#include "persistence/ProjectXml.h"
#include "persistence/PrimaveraXer.h"
#include "reporting/ReportExport.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <iostream>

namespace {
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCoreApplication::setOrganizationName("ToolMoa");
    QCoreApplication::setApplicationName("PPM");
    try {
        check(jobs::sandboxActive(), "sandbox entitlement is not active in the running process");
        check(QDir::homePath().contains("/Library/Containers/com.toolmoa.ppmsandboxvalidation/Data"),
              "test process is not using its isolated sandbox container");
        check(argc == 3, "expected write/read phase and external denied file");
        QFile denied(QString::fromUtf8(argv[2]));
        check(!denied.open(QIODevice::WriteOnly | QIODevice::Append), "sandbox allowed an unselected external write");
        const auto root = jobs::applicationStorage().stateDirectory;
        check(root.startsWith(QDir::homePath() + '/'), "test storage escapes its sandbox container");
        check(QDir().mkpath(root), "cannot create container state");
        jobs::restoreSandboxFolders();
        const auto project = QDir(root).filePath("Project.ppm");
        const auto workspace = QDir(root).filePath("Projects.ppmworkspace");
        check(jobs::authorizeSandboxFile(nullptr, project), "container access incorrectly requires a folder prompt");
        jobs::JsonProjectStore files;
        jobs::JsonWorkspaceStore spaces(files);
        jobs::JsonRecoveryStore recovery(jobs::JsonRecoveryStore::defaultDirectory());
        auto settings = jobs::applicationSettings();
        if (QString::fromUtf8(argv[1]) == "write") {
            check(!QFileInfo::exists(project), "test state already exists; preserve it");
            jobs::Document document(files);
            document.create("Sandbox validation", "2026-11-02", {"Initial task"});
            document.saveAs(project.toStdString());
            document.renameTask(0, "Saved edit");
            document.save();
            jobs::Workspace config;
            config.id = "sandbox-workspace";
            config.projects = {{document.project()->documentId, project.toStdString(), "Sandbox validation"}};
            const auto lease = files.lock(workspace.toStdString());
            spaces.save(workspace.toStdString(), config, {});
            jobs::ReportOptions options;
            options.reportId = "sandbox-report";
            options.generatedAt = "2026-11-01T00:00:00Z";
            const auto report = jobs::buildReport(*document.project(), options);
            jobs::exportXlsx(report, QDir(root).filePath("Report.xlsx"));
            jobs::exportPdf(report, QDir(root).filePath("Report.pdf"));
            jobs::publishProjectXml(jobs::exportProjectXml(*document.project()), QDir(root).filePath("XML"));
            jobs::publishPrimaveraXer(jobs::exportPrimaveraXer(*document.project()), QDir(root).filePath("XER"));
            document.renameTask(0, "Unsaved recovered edit");
            auto session = recovery.newSession();
            session->checkpoint(document.recoverySnapshot("2026-11-01T00:00:00Z"));
            settings.setValue("sandboxValidation/sentinel", "persisted");
            settings.sync();
            check(settings.status() == QSettings::NoError, "sandbox settings did not persist");
        } else {
            check(QString::fromUtf8(argv[1]) == "read", "unknown phase");
            jobs::Document document(files);
            document.open(project.toStdString());
            check(document.project()->tasks[0].name == "Saved edit", "project atomic save did not survive restart");
            const auto config = spaces.load(workspace.toStdString());
            check(config.workspace.projects.size() == 1 &&
                      config.workspace.projects[0].documentId == document.project()->documentId,
                  "workspace did not survive restart");
            check(settings.value("sandboxValidation/sentinel").toString() == "persisted",
                  "settings did not survive restart");
            const auto scan = recovery.scan();
            check(scan.candidates.size() == 1 && scan.candidates[0].valid, "recovery did not survive restart");
            const auto claim = recovery.claim(scan.candidates[0]);
            check(claim && claim->snapshot.project.tasks[0].name == "Unsaved recovered edit",
                  "recovery content changed");
            for (const auto *name : {"Report.xlsx", "Report.pdf", "XML/source.ppm", "XER/source.ppm"})
                check(QFileInfo(QDir(root).filePath(name)).size() > 0, "report or exchange output missing");
        }
        std::cout
            << "PASS: active sandbox, denied external write, project/lock/workspace/recovery/settings/reports/exchange "
            << argv[1] << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
