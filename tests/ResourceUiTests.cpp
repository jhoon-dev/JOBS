#include "ui/MainWindow.h"
#include "persistence/JsonProjectStore.h"
#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QComboBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool c, const char* m) { if (!c) throw std::runtime_error(m); }
void callbackCheck(bool c, const char* m) { if (!c) { std::cerr << "FAIL: " << m << '\n'; std::exit(2); } }
void trigger(QWidget& w, const QString& object) { auto* a = w.findChild<QAction*>(object); check(a && a->isEnabled(), "resource action unavailable"); a->trigger(); }
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "resource UI timeout\n"; std::exit(2); }); watchdog.start(30000);
    try {
        QTemporaryDir dir; jobs::JsonProjectStore store; jobs::Document doc(store); doc.create("자원 평준화 검증", "2026-10-26", {"설계", "개발"}); doc.setDuration(0, 480); doc.setDuration(1, 480);
        jobs::MainWindow window(doc); window.show(); QTest::qWait(60); auto* tasks = window.findChild<QTableWidget*>();
        QTimer::singleShot(30, [&] {
            auto* dialog = QApplication::activeModalWidget(); callbackCheck(dialog && dialog->objectName() == "resourceDialog", "resource dialog missing");
            dialog->findChild<QLineEdit*>("resourceName")->setText("설계·개발팀");
            auto* exceptions = dialog->findChild<QPlainTextEdit*>("capacityExceptions"); exceptions->setPlainText("2026-10-27=0\n2026-10-27=100");
            dialog->findChild<QPushButton*>("saveResource")->click(); callbackCheck(doc.project()->resources.empty() && !dialog->findChild<QLabel*>("resourceError")->text().isEmpty(), "invalid exceptions silently accepted");
            exceptions->setPlainText("2026-10-28=200\n2026-10-29=?"); dialog->findChild<QPushButton*>("saveResource")->click();
            callbackCheck(doc.project()->resources.size() == 1 && doc.project()->resources[0].exceptions.size() == 2, "resource registration failed");
            dialog->findChild<QPushButton*>("newResource")->click(); dialog->findChild<QLineEdit*>("resourceName")->setText("검증 장비"); dialog->findChild<QComboBox*>("resourceType")->setCurrentIndex(1);
            dialog->findChild<QPushButton*>("saveResource")->click(); callbackCheck(doc.project()->resources.size() == 2 && doc.project()->resources[1].type == "equipment", "new resource edited previous stable ID");
            dialog->grab().save("resource-registration-smoke.png"); static_cast<QDialog*>(dialog)->reject();
        });
        trigger(window, "manageResources");
        for (int row = 0; row < 2; ++row) {
            tasks->selectRow(row);
            QTimer::singleShot(30, [] {
                auto* dialog = QApplication::activeModalWidget(); callbackCheck(dialog && dialog->objectName() == "assignmentDialog", "assignment dialog missing");
                dialog->findChild<QSpinBox*>("assignmentUnits_0")->setValue(100);
                dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
            });
            trigger(window, "assignResources");
        }
        callbackCheck(doc.project()->assignments.size() == 2 && tasks->item(0, 7)->text().contains("100%"), "assignment cell missing");
        const auto original = static_cast<const jobs::PlanInput&>(*doc.project());
        QTimer poll; poll.setInterval(20); int stage = 0;
        QObject::connect(&poll, &QTimer::timeout, [&] {
            auto* d = QApplication::activeModalWidget(); if (!d || d->objectName() != "resourceComparisonDialog") return;
            auto* summary = d->findChild<QLabel*>("levelingSummary"); auto* save = d->findChild<QPushButton*>("saveScenario");
            if (stage == 0 && save->isEnabled()) {
                auto* before = d->findChild<QTableWidget*>("loadBefore"); auto* after = d->findChild<QTableWidget*>("loadAfter");
                callbackCheck(before->rowCount() == 1 && before->item(0, 4)->text() == "200%" && before->item(0, 6)->text() == "8.00", "peak/time overload UI mismatch");
                callbackCheck(before->item(0, 0)->foreground().color().lightness() < 100, "overload text lacks contrast in dark theme");
                callbackCheck(after->rowCount() == 2 && after->item(0, 6)->text() == "0.00" && summary->text().contains("2026-10-27 18:00"), "leveled UI mismatch");
                d->findChild<QComboBox*>("levelingPriority")->setCurrentIndex(1);
                callbackCheck(!save->isEnabled() && d->findChild<QTableWidget*>("levelingTasks")->rowCount() == 0, "old result survived option change");
                d->findChild<QPushButton*>("runLeveling")->click(); stage = 1;
            } else if (stage == 1 && save->isEnabled()) {
                d->findChild<QLineEdit*>("scenarioName")->setText("짧은 작업 우선"); save->click(); stage = 2;
            } else if (stage == 2 && !summary->text().contains("계산 중") && doc.project()->scenarios.size() == 1 && d->findChild<QTableWidget*>("levelingTasks")->rowCount() == 2) {
                auto* source = d->findChild<QComboBox*>("levelingSource"); source->setCurrentIndex(0); stage = 3;
            } else if (stage == 3 && save->isEnabled()) {
                auto* horizon = d->findChild<QSpinBox*>("levelingHorizon"); horizon->setValue(1); d->findChild<QPushButton*>("runLeveling")->click(); stage = 4;
            } else if (stage == 4 && save->isEnabled()) {
                callbackCheck(d->findChild<QTableWidget*>("loadAfter")->rowCount() == 0 && summary->text().contains("배치 불가"), "horizon failure shown as success");
                d->findChild<QLineEdit*>("scenarioName")->setText("1일 범위 · 미해결"); save->click(); stage = 5;
            } else if (stage == 5 && doc.project()->scenarios.size() == 2 && !summary->text().contains("계산 중") && d->findChild<QTableWidget*>("levelingTasks")->rowCount() == 2) {
                d->findChild<QComboBox*>("levelingOther")->setCurrentIndex(1); stage = 6;
            } else if (stage == 6 && d->findChild<QTableWidget*>("loadOther")->rowCount() == 2) {
                callbackCheck(summary->text().contains("B 원본 rev") && !d->findChild<QTableWidget*>("levelingTasks")->item(0, 7)->text().contains("—"), "saved A/B comparison missing");
                d->grab().save("resource-scenarios-smoke.png");
                d->findChild<QComboBox*>("levelingSource")->setCurrentIndex(1); stage = 7;
            } else if (stage == 7 && d->findChild<QTableWidget*>("loadAfter")->rowCount() == 2) {
                d->findChild<QTabWidget*>("resourceTabs")->setCurrentIndex(1); d->grab().save("resource-overload-smoke.png");
                poll.stop(); static_cast<QDialog*>(d)->reject(); stage = 8;
            }
        });
        poll.start(); trigger(window, "resourceComparison"); check(stage == 8 && doc.project()->scenarios.size() == 2, "scenario UI did not complete");
        check(static_cast<const jobs::PlanInput&>(*doc.project()) == original, "scenario comparison overwrote original input");
        auto path = dir.filePath("resource-ui.jobs").toStdString(); doc.saveAs(path); check(store.load(path).project == *doc.project(), "UI resource/scenario save-open roundtrip failed");
        for (auto* action : window.findChildren<QAction*>()) if (action->text() == "일정 계산(&C)") action->trigger();
        for (int i = 0; i < 300 && (!doc.calculation() || doc.calculation()->status != jobs::CalculationStatus::Valid); ++i) QTest::qWait(10);
        check(doc.calculation() && doc.calculation()->status == jobs::CalculationStatus::Valid, "main plan calculation unavailable after scenario review");
        jobs::Document readOnly(store); readOnly.open(path, true); jobs::MainWindow viewer(readOnly); viewer.show(); QTest::qWait(30);
        check(!viewer.findChild<QAction*>("assignResources")->isEnabled() && viewer.findChild<QAction*>("resourceComparison")->isEnabled(), "readonly resource actions incorrect");
        window.grab().save("resources-main-smoke.png");
        std::cout << "PASS: resource registration/exception validation, stable IDs, task assignments, peak/daily load, options invalidate results, saved A/B scenarios, unresolved horizon, source preservation and persistence, readonly\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
