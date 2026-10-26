#include "ui/MainWindow.h"
#include "ui/MoneyFields.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QDateTimeEdit>
#include <QTimeZone>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
namespace {
void check(bool c, const char* message) { if (!c) throw std::runtime_error(message); }
void callbackCheck(bool c, const char* m) { if (!c) { std::cerr << "FAIL: " << m << '\n'; std::exit(2); } }
void trigger(QWidget& w, const QString& name) { auto* a = w.findChild<QAction*>(name); check(a && a->isEnabled(), "cost action unavailable"); a->trigger(); }
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "cost UI timeout\n"; std::exit(2); }); watchdog.start(30000);
    try {
        check(jobs::moneyui::parse("90000000000.00", "USD") == jobs::maxMoney && jobs::moneyui::parse("0.01", "USD") == 1 && jobs::moneyui::format(-123, "USD") == "-1.23", "money fields lost minor units");
        QTemporaryDir dir; jobs::JsonProjectStore store; jobs::Document doc(store); doc.create("EVM 원가 검증", "2026-10-26", {"설계"}); doc.setDuration(0, 960);
        auto id = doc.project()->tasks[0].id; auto wbs = doc.addWbs("설계 단계"); doc.assignWbs(0, wbs);
        auto resource = doc.putResource({{}, "설계팀", "labor", 100, {}}); doc.setAssignments(id, {{id, resource, 100}});
        jobs::MainWindow window(doc); window.show(); QTest::qWait(50); auto* table = window.findChild<QTableWidget*>();
        QTimer::singleShot(30, [&] {
            auto* d = QApplication::activeModalWidget(); callbackCheck(d && d->objectName() == "rateDialog", "rate dialog missing");
            d->findChild<QComboBox*>("costCurrency")->setCurrentText("USD"); d->findChild<QPushButton*>("saveCurrency")->click(); callbackCheck(doc.project()->currency == "USD", "currency setup failed");
            auto* rates = d->findChild<QPlainTextEdit*>("rateEntries"); rates->setPlainText("2026-10-26=0.501"); d->findChild<QPushButton*>("saveRates")->click();
            callbackCheck(doc.project()->resources[0].rates.empty() && !d->findChild<QLabel*>("rateError")->text().isEmpty(), "excess monetary precision accepted");
            rates->setPlainText("2026-10-26=0.50"); d->findChild<QPushButton*>("saveRates")->click(); callbackCheck(doc.project()->resources[0].rates[0].hourlyMinor == 50, "rate value lost cents");
            d->findChild<QComboBox*>("costCurrency")->setCurrentText("KRW"); d->findChild<QPushButton*>("saveCurrency")->click(); callbackCheck(doc.project()->currency == "USD", "existing money silently reinterpreted");
            d->grab().save("cost-rates-smoke.png"); static_cast<QDialog*>(d)->reject();
        }); trigger(window, "resourceRates");
        const auto rate = doc.project()->resources[0].rates;
        QTimer::singleShot(30, [&] {
            auto* d = QApplication::activeModalWidget(); auto* list = d->findChild<QTableWidget*>("resourceList"); list->selectRow(0); d->findChild<QLineEdit*>("resourceName")->setText("설계팀 A"); d->findChild<QPushButton*>("saveResource")->click();
            callbackCheck(doc.project()->resources[0].rates == rate, "capacity editor erased resource rates"); static_cast<QDialog*>(d)->reject();
        }); trigger(window, "manageResources");
        table->selectRow(0);
        QTimer::singleShot(30, [] { auto* d = QApplication::activeModalWidget(); callbackCheck(d && d->objectName() == "taskCostDialog", "task budget dialog missing"); d->findChild<QLineEdit*>("fixedCost")->setText("2.00"); d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click(); }); trigger(window, "taskCost");
        check(doc.project()->taskCosts[0].fixedMinor == 200, "task fixed cost not saved");
        doc.captureBaseline("최초 원가 승인", "PM", "예산 승인", "2026-10-26T00:00:00Z");
        QTimer::singleShot(30, [] { auto* d = QApplication::activeModalWidget(); d->findChild<QDateTimeEdit*>("statusAt")->setDateTime(QDateTime(QDate(2026, 10, 26), QTime(18, 0), QTimeZone::UTC)); d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click(); });
        for (auto* a : window.findChildren<QAction*>()) if (a->text() == "상태일 설정") a->trigger();
        table->selectRow(0);
        auto submitReport = [&](const QString& amount, const QString& progress, bool invalid) {
            QTimer::singleShot(30, [&, amount, progress, invalid] {
                auto* d = QApplication::activeModalWidget(); callbackCheck(d && d->objectName() == "costReportDialog", "cost report dialog missing");
                d->findChild<QLineEdit*>("actualCost")->setText(amount); d->findChild<QLineEdit*>("physicalProgress")->setText(progress); d->findChild<QLineEdit*>("costReporter")->setText("현장 PM"); d->findChild<QLineEdit*>("costReason")->setText("청구·물리 완료량 확인");
                auto size = doc.project()->costEntries.size(); auto* save = d->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save);
                if (invalid) { d->findChild<QLineEdit*>("physicalProgress")->setText("101"); save->click(); callbackCheck(doc.project()->costEntries.size() == size && !d->findChild<QLabel*>("costReportError")->text().isEmpty(), "invalid physical credit accepted"); d->findChild<QLineEdit*>("physicalProgress")->setText(progress); }
                callbackCheck(d->findChild<QTableWidget*>("costHistory")->rowCount() == int(size), "cost history incomplete"); save->click();
            }); trigger(window, "costReport");
        };
        submitReport("6.00", "40.00", true); const auto first = doc.project()->costEntries[0];
        QTimer poll; poll.setInterval(20); int stage = 0;
        QObject::connect(&poll, &QTimer::timeout, [&] {
            auto* d = QApplication::activeModalWidget(); if (!d || d->objectName() != "costAnalysisDialog") return;
            auto* tasks = d->findChild<QTableWidget*>("costTasks"); auto* summary = d->findChild<QLabel*>("costSummary");
            if (stage == 0 && tasks->rowCount() == 1) {
                callbackCheck(tasks->item(0, 1)->text() == "10.00" && tasks->item(0, 2)->text() == "10.00" && tasks->item(0, 3)->text() == "5.00" && tasks->item(0, 4)->text() == "4.00" && tasks->item(0, 5)->text() == "6.00" && tasks->item(0, 6)->text() == "-2.00" && tasks->item(0, 8)->text() == "0.6667", "F10 UI values mismatch");
                callbackCheck(d->findChild<QTableWidget*>("costGroups")->rowCount() == 4 && d->findChild<QTableWidget*>("costDays")->rowCount() == 2, "WBS / period totals missing");
                d->findChild<QComboBox*>("eacFormula")->setCurrentIndex(1); stage = 1;
            } else if (stage == 1 && tasks->rowCount() == 1 && tasks->item(0, 10)->text() == "15.00") {
                d->grab().save("cost-evm-smoke.png"); d->findChild<QTabWidget*>("costTabs")->setCurrentIndex(3); d->grab().save("cost-curve-smoke.png");
                auto* days = d->findChild<QTableWidget*>("costDays"); callbackCheck(days->item(1, 5)->text() == "—", "future actual costs invented");
                d->findChild<QComboBox*>("costBaseline")->setCurrentIndex(0); stage = 2;
            } else if (stage == 2 && tasks->rowCount() == 1 && summary->text().contains("기준선 없음")) {
                callbackCheck(tasks->item(0, 2)->text() == "—" && tasks->item(0, 1)->text() == "10.00", "no baseline produced EVM or hid current costs");
                d->findChild<QComboBox*>("costBaseline")->setCurrentIndex(1); doc.renameTask(0, "설계 · 변경 이름"); stage = 3;
            } else if (stage == 3 && summary->text().contains("문서가 변경")) { callbackCheck(tasks->rowCount() == 0, "stale financial result displayed"); poll.stop(); stage = 4; static_cast<QDialog*>(d)->reject(); }
        }); poll.start(); trigger(window, "costAnalysis"); check(stage == 4, "EVM dialog workflow incomplete");
        table->selectRow(0); submitReport("6.50", "40.00", false); check(doc.project()->costEntries.size() == 2 && doc.project()->costEntries[0] == first, "cost correction mutated history");
        doc.saveAs(dir.filePath("cost-ui.jobs").toStdString()); check(store.load(doc.path()).project == *doc.project(), "cost UI persistence mismatch");
        jobs::Document reader(store); reader.open(doc.path(), true); jobs::MainWindow viewer(reader); viewer.show(); viewer.findChild<QTableWidget*>()->selectRow(0);
        check(!viewer.findChild<QAction*>("costReport")->isEnabled() && !viewer.findChild<QAction*>("taskCost")->isEnabled(), "readonly costs editable");
        QTimer::singleShot(30, [] { auto* d = QApplication::activeModalWidget(); callbackCheck(!d->findChild<QPushButton*>("saveRates")->isEnabled() && !d->findChild<QPushButton*>("saveCurrency")->isEnabled(), "readonly rates editable"); static_cast<QDialog*>(d)->reject(); }); trigger(viewer, "resourceRates");
        QTimer readonlyPoll; readonlyPoll.setInterval(20); QObject::connect(&readonlyPoll, &QTimer::timeout, [&] { auto* d = QApplication::activeModalWidget(); if (!d || d->objectName() != "costAnalysisDialog") return; auto* rows = d->findChild<QTableWidget*>("costTasks"); if (rows->rowCount() == 1) { callbackCheck(rows->item(0, 5)->text() == "6.50", "readonly analysis lost corrected actual"); readonlyPoll.stop(); static_cast<QDialog*>(d)->reject(); } }); readonlyPoll.start(); trigger(viewer, "costAnalysis");
        std::cout << "PASS: currency/rate precision, capacity edits preserve rates, task budgets, cumulative actual/physical reporting/corrections, F10 EVM/EAC, WBS/timephased/curve, no baseline/future AC/stale results, persistence and readonly\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
