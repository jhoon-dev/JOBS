#include "ui/MainWindow.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
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
        auto path = dir.filePath("ui.jobs");
        QTimer::singleShot(50, [&] {
            auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget());
            if (!dialog) return;
            dialog->selectFile(path); QMetaObject::invokeMethod(dialog, "accept");
        });
        trigger(window, "저장(&");
        check(!doc.dirty() && !doc.path().empty(), "save dialog failed");
        auto* table = window.findChild<QTableWidget*>();
        table->item(0, 0)->setText("편집된 설계");
        check(doc.dirty() && doc.project()->tasks[0].name == "편집된 설계", "table edit failed");
        QTimer::singleShot(50, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Cancel)->click();
        });
        check(!window.close() && window.isVisible() && doc.dirty(), "cancel close lost changes");
        QTimer::singleShot(50, [] {
            if (auto* box = qobject_cast<QMessageBox*>(QApplication::activeModalWidget())) box->button(QMessageBox::Save)->click();
        });
        check(window.close() && !doc.dirty(), "save on close failed");
        check(store.load(path.toUtf8().toStdString()).project == *doc.project(), "UI save content mismatch");
        std::cout << "PASS: new dialog, task paste/edit, save dialog, cancel close, save on close\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
