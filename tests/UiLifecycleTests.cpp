#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDir>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <cstdlib>
#include <stdexcept>

namespace {
void check(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
void trigger(QWidget& w, const QString& label) {
    for (auto* a : w.findChildren<QAction*>()) if (a->text().startsWith(label)) { a->trigger(); return; }
    throw std::runtime_error("action missing");
}
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    // Ensure a broken modal test fails instead of blocking CI indefinitely.
    QTimer watchdog; watchdog.setSingleShot(true);
    QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "modal test timeout\n"; std::exit(2); });
    watchdog.start(20000);
    try {
        QTemporaryDir dir; check(dir.isValid(), "temp directory failed");
        jobs::JsonProjectStore store; jobs::Document doc(store); jobs::MainWindow window(doc);
        window.show(); QTest::qWait(50);
        QTimer::singleShot(50, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->findChild<QLineEdit*>()->setText("UI 프로젝트");
            dialog->findChild<QPlainTextEdit*>()->setPlainText("설계\n개발\n\n검증");
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        trigger(window, "새로 만들기");
        check(doc.project() && doc.project()->tasks.size() == 3 && doc.dirty(), "new project dialog failed");
        auto path = dir.filePath("MyUiPlan.jobs");
        QTimer::singleShot(50, [&] {
            auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            // Type the path as a user would; selectFile() can leave the focused, prefilled name intact on macOS.
            auto* filename = dialog->findChild<QLineEdit*>("fileNameEdit");
            if (!filename) return;
            filename->setText(path); QMetaObject::invokeMethod(dialog, "accept");
        });
        trigger(window, "저장(&");
        check(!doc.dirty() && !doc.path().empty(), "save dialog failed");
        check(QDir(dir.path()).entryList({"*.jobs"}, QDir::Files) == QStringList{"MyUiPlan.jobs"},
              "save dialog did not preserve the physical filename");
        check(QFileInfo(QString::fromUtf8(doc.path())).fileName() == "MyUiPlan.jobs",
              "save dialog did not preserve the displayed filename");
        if (doc.path() != store.normalizePath(path.toUtf8().toStdString())) {
            std::cerr << "expected path: " << path.toUtf8().toStdString() << "; selected path: " << doc.path() << '\n';
            throw std::runtime_error("file dialog selected unexpected save path");
        }
        auto* table = taskTable(window);
        setCellText(table, 0, 0, "편집된 설계");
        check(doc.dirty() && doc.project()->tasks[0].name == "편집된 설계", "table edit failed");
        setCellText(table, 0, 1, "8"); setCellText(table, 1, 1, "4"); setCellText(table, 2, 1, "1m");
        setCellText(table, 1, 2, "1");
        check(doc.project()->tasks[0].durationMinutes == 480 && doc.project()->tasks[2].durationMinutes == 1 &&
            doc.project()->tasks[1].predecessors == std::vector<std::string>{doc.project()->tasks[0].id}, "duration/FS input failed");
        auto waitResult = [&] {
            for (int i = 0; i < 200; ++i) {
                QTest::qWait(10);
                if (!window.findChild<QPlainTextEdit*>("calculationDiagnostics")->toPlainText().startsWith("계산 중")) break;
            }
            check(doc.calculation().has_value(), "calculation did not finish");
            check(!window.findChild<QPlainTextEdit*>("calculationDiagnostics")->toPlainText().startsWith("계산 중"), "calculation timed out");
        };
        trigger(window, "일정 계산"); waitResult();
        check(doc.calculation()->status == jobs::CalculationStatus::Valid && cellText(table, 0, 3) != "미계산", "calculation UI failed");
        setCellText(table, 0, 1, "16");
        check(doc.calculation()->status == jobs::CalculationStatus::Stale && cellText(table, 0, 3) == "미계산", "stale dates remained visible");
        trigger(window, "일정 계산");
        doc.setDuration(0, 960 + 60); // Edit before the worker completion reaches the event loop.
        waitResult();
        check(doc.calculation()->status == jobs::CalculationStatus::Stale, "in-flight outdated result was applied");
        QTimer::singleShot(50, [] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->findChild<QPlainTextEdit*>()->setPlainText("2026-11-02");
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        trigger(window, "휴일 편집");
        check(doc.project()->holidays == std::vector<std::string>{"2026-11-02"}, "holiday dialog failed");
        setCellText(table, 2, 1, ""); trigger(window, "일정 계산"); waitResult();
        check(doc.calculation()->status == jobs::CalculationStatus::Incomplete && cellText(table, 0, 3) == "미계산", "incomplete forecast remained visible");
        setCellText(table, 2, 1, "1m"); setCellText(table, 0, 2, "2");
        trigger(window, "일정 계산"); waitResult();
        check(doc.calculation()->status == jobs::CalculationStatus::Conflict && cellText(table, 0, 3) == "미계산", "cycle forecast remained visible");
        setCellText(table, 0, 2, ""); trigger(window, "일정 계산"); waitResult();
        check(doc.calculation()->status == jobs::CalculationStatus::Valid, "corrected cycle did not recalculate");
        window.grab().save("ui-smoke.png");
        QTimer::singleShot(50, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Cancel)->click();
        });
        check(!window.close() && window.isVisible() && doc.dirty(), "cancel close lost changes");
        QTimer::singleShot(50, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Save)->click();
        });
        check(window.close() && !doc.dirty(), "save on close failed");
        check(store.load(path.toUtf8().toStdString()).project == *doc.project(), "UI save content mismatch");
        std::cout << "PASS: new/save dialogs, task/duration/FS/holiday edits, async calculation, stale/incomplete/cycle handling, cancel/save on close\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
