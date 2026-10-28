#include "ui/DesktopWorkspace.h"
#include "ui/MainWindow.h"
#include "ui/WorkspaceDialog.h"
#include "persistence/JsonRecoveryStore.h"
#include "formatting/DiagnosticText.h"
#include <QApplication>
#include <QComboBox>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPointer>
#include <QPushButton>
#include <QTableView>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>

namespace {
void check(bool value, const char *reason) {
    if (!value)
        throw std::logic_error(reason);
}
void wait(const std::function<bool()> &ready, int timeout = 15000) {
    QElapsedTimer timer;
    timer.start();
    while (!ready() && timer.elapsed() < timeout)
        QTest::qWait(10);
    check(ready(), "asynchronous UI timeout");
}
void flush() {
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QTest::qWait(20);
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    app.setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer watchdog;
    watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] {
        std::cerr << "workspace UI watchdog\n";
        std::exit(2);
    });
    watchdog.start(90000);
    try {
        QTemporaryDir dir;
        auto files = std::make_shared<jobs::JsonProjectStore>();
        std::vector<std::string> paths, ids;
        for (int i = 0; i < 2; ++i) {
            jobs::Document doc(*files);
            doc.create("Project " + std::to_string(i), "2026-10-26", {"Task"});
            doc.setDuration(0, 60);
            auto resource = doc.putResource({"", "Shared", "labor", 100, {}});
            doc.setAssignments(doc.project()->tasks[0].id, {{doc.project()->tasks[0].id, resource, 100}});
            auto path = dir.filePath(QString("Project%1.jobs").arg(i)).toStdString();
            doc.saveAs(path);
            paths.push_back(path);
            ids.push_back(doc.project()->documentId);
        }
        const auto group = dir.filePath("Workspace.jobsworkspace");
        {
            jobs::DesktopWorkspace workspace(files, {});
            auto *first = workspace.openProject(paths[0]);
            auto *second = workspace.openProject(paths[1]);
            check(first != second && workspace.snapshots().size() == 2, "projects did not get independent windows");
            check(workspace.openProject(paths[0]) == first && workspace.snapshots().size() == 2,
                  "same file duplicated");
            const auto duplicate = dir.filePath("Duplicate.jobs");
            check(QFile::copy(QString::fromStdString(paths[0]), duplicate), "copy fixture");
            bool rejected = false;
            try {
                workspace.openProject(duplicate.toStdString(), true);
            } catch (const jobs::AppError &error) {
                rejected = error.code() == "WORKSPACE_DUPLICATE_PROJECT";
            }
            check(rejected, "duplicate document ID accepted");
            auto *a = workspace.document(ids[0]);
            auto *b = workspace.document(ids[1]);
            const auto bRoot = b->snapshot();
            const auto before = workspace.generation();
            a->renameTask(0, "Independent change");
            check(a->dirty() && !b->dirty() && b->snapshot() == bRoot && workspace.generation() > before,
                  "document dirty/snapshot leaked across windows");
            a->undo();
            check(!a->dirty() && !b->canUndo(), "undo leaked across windows");
            auto config = workspace.configuration();
            config.resources = {{"S", "Shared"}};
            for (const auto &snapshot : workspace.snapshots())
                config.bindings.push_back({snapshot->documentId, snapshot->resources[0].id, "S"});
            workspace.applyConfiguration(config);
            QTimer::singleShot(30, [&] {
                auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget());
                if (dialog) {
                    dialog->findChild<QLineEdit *>("fileNameEdit")->setText(group);
                    QMetaObject::invokeMethod(dialog, "accept");
                }
            });
            check(workspace.saveWorkspace(first) && !workspace.dirty(), "workspace save UI failed");
            jobs::WorkspaceDialog dialog(workspace);
            dialog.show();
            auto *analyze = dialog.findChild<QPushButton *>("workspaceAnalyze");
            auto *cancel = dialog.findChild<QPushButton *>("workspaceCancel");
            auto *load = dialog.findChild<QTableView *>("workspaceLoad");
            auto *status = dialog.findChild<QLabel *>("workspaceStatus");
            check(analyze && cancel && load && status, "workspace controls missing");
            analyze->click();
            cancel->click();
            wait([&] { return analyze->isEnabled(); });
            check(load->model()->rowCount() == 0 && status->text().contains("WORKSPACE_CANCELLED"),
                  "cancel applied partial result");
            analyze->click();
            a->renameTask(0, "Stale source");
            wait([&] { return analyze->isEnabled(); });
            check(load->model()->rowCount() == 0 && status->text().contains("WORKSPACE_SOURCE_CHANGED"),
                  "stale worker result applied");
            a->undo();
            analyze->click();
            for (auto *timer : dialog.findChildren<QTimer *>())
                QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection);
            wait([&] { return analyze->isEnabled(); });
            check(load->model()->rowCount() == 1 && load->model()->index(0, 3).data().toInt() == 200,
                  "aggregate UI did not show concurrent demand");
            auto *tasks = dialog.findChild<QComboBox *>("workspaceRelatedTask");
            check(tasks->count() == 2, "navigation lost one project's task");
            if (app.arguments().contains("--capture-workspace")) {
                dialog.findChild<QTabWidget *>()->setCurrentIndex(2);
                QTest::qWait(100);
                check(dialog.grab().save("workspace-ui.png"), "workspace capture failed");
            }
            dialog.findChild<QComboBox *>("workspaceMode")->setCurrentIndex(1);
            check(!dialog.findChild<QPushButton *>("workspaceNavigate")->isEnabled(),
                  "mode change preserved active old navigation");
            dialog.hide();
            QPointer<jobs::MainWindow> closed = second;
            check(second->close(), "clean secondary window did not close");
            flush();
            check(!closed && workspace.snapshots().size() == 1 && workspace.configuration().projects.size() == 2,
                  "closing window lost saved project reference");
            second = workspace.openProject(paths[1]);
            check(second && workspace.snapshots().size() == 2, "closed reference could not reopen");
            // Reopen the file in a separate workspace after all original file leases are released.
            check(second->close(), "secondary close");
            flush();
            check(first->close(), "primary close");
            flush();
        }
        jobs::DesktopWorkspace restored(files, {});
        restored.openWorkspace(group.toStdString());
        check(restored.snapshots().size() == 2 && restored.configuration().bindings.size() == 2 && !restored.dirty(),
              "transactional workspace reopen failed");
        const auto frozen = restored.configuration();
        const auto roots = restored.snapshots();
        auto broken = frozen;
        broken.projects[0].path = dir.filePath("Missing.jobs").toStdString();
        const auto badPath = dir.filePath("Broken.jobsworkspace").toStdString();
        jobs::JsonWorkspaceStore(*files).save(badPath, broken, {});
        QTimer dismiss;
        QObject::connect(&dismiss, &QTimer::timeout, [] {
            if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget()))
                box->accept();
        });
        dismiss.start(10);
        restored.openWorkspace(badPath);
        dismiss.stop();
        check(restored.configuration() == frozen && restored.snapshots() == roots,
              "failed group open partially replaced documents/configuration");
        std::cout << "PASS: independent windows, leases/duplicate IDs, dirty/undo, workspace save/reopen/rollback, "
                     "cancellation and stale-result UI\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << jobs::errorText(error) << '\n';
        return 1;
    }
}
