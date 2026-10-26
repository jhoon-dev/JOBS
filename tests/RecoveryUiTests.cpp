#include "ui/MainWindow.h"
#include "ui/AutoSaveController.h"
#include "ui/RecoveryDialog.h"
#include "ui/ReportDialog.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QLabel>
#include <QTableWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QSettings>
#include <QTemporaryDir>
#include <QSemaphore>
#include <QThreadPool>
#include <QTimer>
#include <QElapsedTimer>
#include <QTest>
#include <QDir>
#include <QFile>
#include <atomic>
#include <iostream>
#include <cstdlib>
namespace {
void check(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
void callbackCheck(bool b, const char* m) { if (!b) { std::cerr << "FAIL: " << m << '\n'; std::_Exit(2); } }
bool wait(const std::function<bool()>& predicate, int ms = 5000) { QElapsedTimer timer; timer.start(); while (!predicate() && timer.elapsed() < ms) QTest::qWait(10); return predicate(); }
QByteArray read(const QString& path) { QFile f(path); check(f.open(QIODevice::ReadOnly), "read"); return f.readAll(); }
void trigger(QWidget& w, const char* name) { auto* a = w.findChild<QAction*>(name); check(a && a->isEnabled(), "action disabled"); a->trigger(); }
struct Gate { QSemaphore entered, proceed; std::atomic<bool> armed{true}; };
struct Release { std::shared_ptr<Gate> gate; ~Release() { gate->proceed.release(10); } };
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs); app.setQuitOnLastWindowClosed(false);
    QCoreApplication::setOrganizationName("JOBS-Recovery-Tests"); QCoreApplication::setApplicationName("RecoveryUi");
    QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "recovery UI timeout\n"; std::_Exit(2); }); watchdog.start(45000);
    try {
        QTemporaryDir dir; QSettings::setDefaultFormat(QSettings::IniFormat); QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, dir.filePath("settings")); QSettings().setValue("recovery/enabled", true);
        jobs::JsonProjectStore store; jobs::Document doc(store); doc.create("백그라운드 복구", "2026-10-26", {"task"}); doc.setDuration(0, 480); auto source = dir.filePath("source.jobs"); doc.saveAs(source.toStdString()); const auto bytes = read(source);
        // Deterministically pause a flushed checkpoint while save/new edits retire it.
        auto gate = std::make_shared<Gate>(); Release release{gate};
        jobs::JsonRecoveryStore raceStore(dir.filePath("race"), [gate] { if (gate->armed.exchange(false)) { gate->entered.release(); gate->proceed.acquire(); } });
        auto controller = std::make_unique<jobs::AutoSaveController>(doc, raceStore, nullptr, 100, 10);
        doc.renameTask(0, "old checkpoint"); controller->checkpointNow(); check(wait([&] { return gate->entered.available() > 0; }), "checkpoint did not reach flush gate");
        doc.save(); doc.renameTask(0, "new after save"); gate->proceed.release();
        check(wait([&] { return !controller->busy() && controller->status().contains("저장됨") && controller->status().contains("r" + QString::number(doc.project()->revision)); }), "new edit after retired write not checkpointed");
        check(doc.dirty() && read(source) != bytes && store.load(source.toStdString()).project.tasks[0].name == "old checkpoint", "auto-save wrote current edits into explicit save");
        controller.reset(); check(QThreadPool::globalInstance()->waitForDone(10000), "background retirement timeout"); QTest::qWait(20);
        auto raceCandidates = raceStore.scan(); check(raceCandidates.candidates.size() == 1, "retired checkpoint resurrected after save"); auto raced = raceStore.claim(raceCandidates.candidates.front()); check(raced->snapshot.project.tasks[0].name == "new after save", "stale write replaced new checkpoint"); raced.reset();
        // No writes when paused/readonly; continuous edits are checkpointed by deadline.
        std::atomic<int> writes{0}; jobs::JsonRecoveryStore deadlineStore(dir.filePath("deadline"), [&] { ++writes; });
        {
            jobs::AutoSaveController periodic(doc, deadlineStore, nullptr, 40, 200); periodic.setEnabled(false); QTest::qWait(60); check(writes == 0, "paused autosave wrote data"); periodic.setEnabled(true);
            for (int i = 0; i < 24; ++i) { doc.renameTask(0, "continuous " + std::to_string(i)); QTest::qWait(10); }
            check(writes > 0 && doc.dirty(), "continuous edits prevented maximum-interval checkpoint"); check(wait([&] { return !periodic.busy(); }), "deadline writer stuck"); periodic.finishDocument();
        }
        check(wait([&] { auto s = deadlineStore.scan(); return s.candidates.empty() && s.activeSessions == 0; }), "normal finish failed to clear checkpoint");
        jobs::Document readonly(store); readonly.open(source.toStdString(), true); jobs::JsonRecoveryStore readonlyStore(dir.filePath("readonly")); { jobs::AutoSaveController view(readonly, readonlyStore, nullptr, 10, 5); view.checkpointNow(); QTest::qWait(30); check(view.status().contains("읽기 전용") && !QFile::exists(readonlyStore.directory()), "readonly generated checkpoint"); }
        bool fault = false; jobs::JsonRecoveryStore failureStore(dir.filePath("failure"), [&] { if (fault) throw std::runtime_error("write failure"); });
        {
            jobs::AutoSaveController failing(doc, failureStore, nullptr, 1000, 5); failing.checkpointNow(); check(wait([&] { return !failing.busy() && failing.status().contains("저장됨"); }), "initial checkpoint failed"); fault = true; doc.renameTask(0, "failed checkpoint"); failing.checkpointNow(); check(wait([&] { return !failing.busy() && failing.status().contains("실패"); }), "write failure not visible"); check(doc.dirty(), "failed auto-save marked document saved"); failing.setEnabled(false);
        }
        check(QThreadPool::globalInstance()->waitForDone(10000), "failure thread timeout"); QTest::qWait(20); check(failureStore.scan().candidates.size() == 1, "failed write destroyed completed checkpoint");
        // Startup selection restores an editable detached copy and preserves source bytes.
        auto uiRoot = dir.filePath("ui"); jobs::JsonRecoveryStore recovery(uiRoot); auto seed = recovery.newSession(); seed->checkpoint(doc.recoverySnapshot("2026-10-26T09:00:00Z")); seed.reset(); const auto originalBytes = read(source); const auto recoveredName = doc.project()->tasks[0].name;
        jobs::Document restored(store); jobs::MainWindow window(restored, &recovery); window.show(); QTimer startupPoll; startupPoll.setInterval(10); bool selected = false;
        QObject::connect(&startupPoll, &QTimer::timeout, [&] { auto* dialog = QApplication::activeModalWidget(); if (!dialog || dialog->objectName() != "recoveryDialog") return; auto* restore = dialog->findChild<QPushButton*>("restoreRecovery"); if (!restore->isEnabled()) return; dialog->grab().save("recovery-selection-smoke.png"); selected = true; startupPoll.stop(); restore->click(); }); startupPoll.start();
        check(wait([&] { return restored.recoveryOrigin().has_value(); }), "startup recovery selection failed"); check(selected && restored.dirty() && restored.requiresSaveAs() && restored.path().empty() && restored.project()->tasks[0].name == recoveredName && read(source) == originalBytes, "startup recovery modified original or lost snapshot");
        auto* table = window.findChild<QTableWidget*>(); const auto priorName = restored.project()->tasks[0].name; table->item(0, 0)->setText("UI commit"); check(restored.project()->tasks[0].name == "UI commit", "table edit not committed");
        table->editItem(table->item(0, 0)); QTest::qWait(20); auto* editor = table->findChild<QLineEdit*>(); check(editor, "cell editor missing"); editor->setText("local"); editor->insert("x"); QTest::keySequence(editor, QKeySequence::Undo); check(editor->text() == "local" && restored.project()->tasks[0].name == "UI commit", "text undo triggered document undo"); QTest::keyClick(editor, Qt::Key_Escape); table->setFocus(); window.activateWindow(); QTest::qWait(20);
        QTest::keySequence(table, QKeySequence::Undo); check(restored.project()->tasks[0].name == priorName && window.findChild<QAction*>("redoEdit")->isEnabled(), "document undo shortcut failed"); QTest::keySequence(table, QKeySequence::Redo); check(restored.project()->tasks[0].name == "UI commit", "redo shortcut failed");
        table->editItem(table->item(0, 1)); QTest::qWait(20); editor = table->findChild<QLineEdit*>(); check(editor, "duration editor missing"); editor->setText("16");
        bool calculated = false; for (auto* action : window.findChildren<QAction*>()) if (action->shortcut() == QKeySequence(Qt::Key_F9)) { action->trigger(); calculated = true; break; }
        check(calculated && wait([&] { return restored.calculation() && restored.calculation()->status == jobs::CalculationStatus::Valid; }) && restored.project()->tasks[0].durationMinutes == 960, "F9 lost pending cell input");
        restored.captureBaseline("새 승인", "PM", "복구 후 승인", "2026-10-26T09:00:00Z"); check(!window.findChild<QAction*>("undoEdit")->isEnabled() && window.findChild<QAction*>("undoEdit")->toolTip().contains("정정 보고"), "immutable history boundary not explained");
        // Save uses a fresh file and clears the claimed source only after commit.
        const auto copyPath = dir.filePath("recovered.jobs"); QTimer::singleShot(30, [&] { auto* file = qobject_cast<QFileDialog*>(QApplication::activeModalWidget()); callbackCheck(file, "save chooser absent"); file->findChild<QLineEdit*>("fileNameEdit")->setText(copyPath); QMetaObject::invokeMethod(file, "accept"); });
        bool saved = false; for (auto* action : window.findChildren<QAction*>()) if (action->shortcut() == QKeySequence::Save) { action->trigger(); saved = true; break; }
        check(saved && !restored.dirty() && QFile::exists(copyPath) && read(source) == originalBytes && store.load(copyPath.toStdString()).project == *restored.project(), "recovery save damaged source/copy");
        check(wait([&] { auto s = recovery.scan(); return s.candidates.empty() && s.activeSessions == 0; }), "saved recovery source not cleaned");
        restored.renameTask(0, "temporary edit"); trigger(window, "undoEdit"); check(!restored.dirty() && !restored.revisionSaved(), "undo clean inputs incorrectly claimed saved revision");
        jobs::ReportDialog report(restored, {}, {}); report.show(); check(wait([&] { return report.findChild<QPushButton*>("exportXlsx")->isEnabled(); }) && report.findChild<QLabel*>("reportMessage")->text().contains("미저장 메모리"), "report provenance claimed a new undo revision was already saved"); report.reject();
        table->editItem(table->item(0, 0)); QTest::qWait(20); editor = table->findChild<QLineEdit*>(); check(editor, "pending close editor missing"); editor->setText("pending close");
        QTimer::singleShot(20, [] { auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); callbackCheck(box, "uncommitted edit close confirmation absent"); box->button(QMessageBox::Cancel)->click(); });
        const auto closed = window.close(); if (closed || !restored.dirty() || restored.project()->tasks[0].name != "pending close") std::cerr << "close=" << closed << " dirty=" << restored.dirty() << " name=" << restored.project()->tasks[0].name << '\n';
        check(!closed && restored.dirty() && restored.project()->tasks[0].name == "pending close", "close lost pending editor text or cancel state");
        trigger(window, "autosaveNow"); auto* autosave = dynamic_cast<jobs::AutoSaveController*>(window.findChild<QObject*>("autoSaveController")); check(autosave && wait([&] { return !autosave->busy() && autosave->status().contains("저장됨"); }), "manual recovery checkpoint unavailable");
        QTest::qWait(20); window.grab().save("recovery-main-smoke.png"); check(recovery.scan().activeSessions == 1, "close cancel retired recovery session");
        QTimer::singleShot(20, [] { auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); callbackCheck(box, "discard close confirmation absent"); box->button(QMessageBox::Discard)->click(); }); check(window.close(), "discard close refused"); check(wait([&] { auto s = recovery.scan(); return s.candidates.empty() && s.activeSessions == 0; }), "explicit discard close retained current checkpoint");
        // Later leaves data intact; explicit session discard is confirmed and source-safe.
        auto session = recovery.newSession(); session->checkpoint(doc.recoverySnapshot("2026-10-26T09:00:00Z")); session.reset(); auto candidates = recovery.scan(); jobs::RecoveryDialog later(recovery, nullptr, candidates); later.show(); later.reject(); check(recovery.scan().candidates.size() == 1, "later deleted recoverable data");
        jobs::RecoveryDialog manage(recovery, nullptr, candidates); manage.show(); QTimer::singleShot(20, [] { auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); callbackCheck(box, "discard confirmation missing"); box->button(QMessageBox::No)->click(); }); manage.findChild<QPushButton*>("discardRecovery")->click(); check(recovery.scan().candidates.size() == 1, "discard cancel deleted checkpoint");
        QTimer::singleShot(20, [] { auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget()); callbackCheck(box, "confirmed discard missing"); box->button(QMessageBox::Yes)->click(); }); manage.findChild<QPushButton*>("discardRecovery")->click(); check(wait([&] { return manage.findChild<QTableWidget*>("recoveryList")->rowCount() == 0; }) && read(source) == originalBytes, "session discard lost original or failed reload"); manage.reject();
        std::cout << "PASS: background save retirement race, continuous deadline/pause/readonly/failure, startup selection, detached save, editor/document undo shortcuts, immutable notice, close cancel/discard, later and explicit discard\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
