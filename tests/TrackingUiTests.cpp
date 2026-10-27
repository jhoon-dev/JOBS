#include "TaskTableTest.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "persistence/JsonProjectStore.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <QTimeZone>
#include <cstdlib>
#include <cmath>
#include <iostream>
#include <stdexcept>
namespace {
void check(bool c, const char* m) { if (!c) throw std::runtime_error(m); }
void callbackCheck(bool c, const char* m) { if (!c) { std::cerr << "FAIL: " << m << '\n'; std::exit(2); } }
void trigger(QWidget& w, const QString& label) {
    for (auto* a : w.findChildren<QAction*>()) if (a->text() == label) { check(a->isEnabled(), "tracking action disabled"); a->trigger(); return; }
    throw std::runtime_error("tracking action missing");
}
int count(jobs::GanttView* view, const QString& kind) {
    int n = 0; for (auto* i : view->scene()->items()) if (i->data(1).toString() == kind) ++n; return n;
}
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "tracking UI timeout\n"; std::exit(2); }); watchdog.start(30000);
    try {
        QTemporaryDir dir; check(dir.isValid(), "temporary directory failed");
        jobs::JsonProjectStore store; jobs::Document doc(store); doc.create("기준선·실적 검증", "2026-10-26", {"설계", "계획 승인"});
        doc.setDuration(0, 960); doc.setDuration(1, 0); const auto aid = doc.project()->tasks[0].id;
        doc.setPredecessors(1, {aid}); doc.applyCalculation(jobs::calculate(*doc.project()));
        jobs::MainWindow window(doc); window.show(); QTest::qWait(60);
        auto* table = taskTable(window); auto* gantt = dynamic_cast<jobs::GanttView*>(window.findChild<QGraphicsView*>("ganttView"));
        auto baseline = [&](const QString& name) {
            QTimer::singleShot(50, [name] {
                auto* dialog = QApplication::activeModalWidget(); if (!dialog) return;
                dialog->findChild<QLineEdit*>("baselineName")->setText(name); dialog->findChild<QLineEdit*>("baselineApprover")->setText("PM");
                dialog->findChild<QLineEdit*>("baselineReason")->setText("검토 후 승인"); dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
            });
            const auto size = doc.project()->baselines.size(); trigger(window, "기준선 저장");
            for (int i = 0; i < 300 && doc.project()->baselines.size() == size; ++i) QTest::qWait(10);
            check(doc.project()->baselines.size() == size + 1, "baseline UI/worker failed");
        };
        baseline("최초 승인"); const auto frozen = doc.project()->baselines[0];
        QTimer::singleShot(50, [] {
            auto* dialog = QApplication::activeModalWidget(); if (!dialog) return;
            dialog->findChild<QDateTimeEdit*>("statusAt")->setDateTime(QDateTime(QDate(2026, 10, 26), QTime(18, 0), QTimeZone::UTC));
            dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Ok)->click();
        });
        trigger(window, "상태일 설정"); check(doc.project()->statusAt == "2026-10-26 18:00", "status date dialog failed");
        table->selectRow(0);
        QTimer::singleShot(50, [] {
            auto* dialog = QApplication::activeModalWidget(); if (!dialog) return;
            dialog->findChild<QLineEdit*>("actualStart")->setText("2026-10-26 09:00"); dialog->findChild<QSpinBox*>("actualMinutes")->setValue(480);
            dialog->findChild<QSpinBox*>("remainingMinutes")->setValue(960); dialog->findChild<QLineEdit*>("progressReporter")->setText("현장 담당자");
            dialog->findChild<QLineEdit*>("progressReason")->setText("잔여 16시간 보고"); dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save)->click();
        });
        trigger(window, "선택 작업 실적"); check(doc.project()->progress.size() == 1 && doc.project()->baselines[0] == frozen, "progress UI changed baseline");
        auto recalculate = [&] {
            trigger(window, "일정 계산(&C)");
            for (int i = 0; i < 300; ++i) { QTest::qWait(10); if (!window.findChild<QPlainTextEdit*>("calculationDiagnostics")->toPlainText().startsWith("계산 중")) break; }
            check(doc.calculation() && doc.calculation()->status == jobs::CalculationStatus::Valid, "forecast UI failed");
        };
        recalculate();
        check(doc.calculation()->isForecast && cellText(table, 0, 3) == "2026-10-26 09:00" && cellText(table, 0, 4) == "2026-10-28 18:00" &&
              cellText(table, 0, 5) == "—" && count(gantt, "baseline") == 2, "forecast actual dates / unsupported float / baseline overlay mismatch");
        auto* choice = window.findChild<QComboBox*>("baselineChoice"); choice->setCurrentIndex(0); check(count(gantt, "baseline") == 0, "baseline overlay toggle failed");
        choice->setCurrentIndex(1); check(count(gantt, "baseline") == 2, "baseline selection overlay failed");
        QTimer comparePoll; comparePoll.setInterval(20); int stage = 0;
        QObject::connect(&comparePoll, &QTimer::timeout, [&] {
            auto* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget()); if (!dialog || dialog->objectName() != "comparisonDialog") return;
            auto* comparison = dialog->findChild<QTableWidget*>("comparisonTable"); if (comparison->rowCount() != 2) return;
            if (stage == 0) {
                callbackCheck(comparison->item(0, 1)->text() == "2026-10-27 18:00" && comparison->item(0, 2)->text() == "2026-10-27 18:00" &&
                    comparison->item(0, 3)->text() == "0.00" && comparison->item(0, 9)->text() == "2026-10-28 18:00" && comparison->item(0, 10)->text() == "+8.00" &&
                    comparison->item(0, 8)->text() == "33.3" && comparison->item(1, 8)->text() == "미정", "comparison UI confused baseline/plan/forecast/progress");
                callbackCheck(dialog->findChild<QLabel*>("comparisonSummary")->text().contains("보고 범위 1/2"), "comparison coverage missing");
                dialog->grab().save("tracking-comparison-smoke.png"); ++stage;
                dialog->findChild<QComboBox*>("comparisonBaseline")->setCurrentIndex(0); return;
            }
            callbackCheck(comparison->item(0, 10)->text() == "—" && comparison->item(0, 1)->text() == "—", "no baseline displayed fake zero variance");
            comparePoll.stop(); dialog->reject();
        });
        comparePoll.start(); trigger(window, "계획 대비 비교"); check(stage == 1, "comparison dialog did not complete");
        // A contradictory report stays in the dialog and preserves the ledger, then a reasoned correction appends.
        table->selectRow(0); const auto originalReport = doc.project()->progress[0];
        QTimer::singleShot(50, [&] {
            auto* dialog = QApplication::activeModalWidget(); if (!dialog) return;
            callbackCheck(dialog->findChild<QTableWidget*>("progressHistory")->rowCount() == 1, "report history absent");
            dialog->findChild<QLineEdit*>("actualFinish")->setText("2026-10-29 18:00"); dialog->findChild<QSpinBox*>("remainingMinutes")->setValue(0);
            dialog->findChild<QLineEdit*>("progressReason")->setText("검증 후 정정"); auto* save = dialog->findChild<QDialogButtonBox*>()->button(QDialogButtonBox::Save); save->click();
            for (int i = 0; i < 300 && !save->isEnabled(); ++i) QTest::qWait(10);
            callbackCheck(save->isEnabled() && doc.project()->progress.size() == 1 && !dialog->findChild<QLabel*>("progressError")->text().isEmpty(), "invalid report mutated state / no field error");
            dialog->findChild<QLineEdit*>("actualFinish")->clear(); dialog->findChild<QSpinBox*>("remainingMinutes")->setValue(1200); save->click();
        });
        trigger(window, "선택 작업 실적");
        check(doc.project()->progress.size() == 2 && doc.project()->progress[0] == originalReport && doc.project()->progress[1].previousEntryId == originalReport.id &&
              doc.project()->baselines[0] == frozen, "correction lost immutable snapshot/history");
        baseline("재승인"); check(doc.project()->baselines[0] == frozen && doc.project()->baselines[1].version == 2, "new baseline replaced old");
        recalculate(); window.grab().save("tracking-main-smoke.png");
        const auto path = dir.filePath("tracking-ui.jobs");
        QTimer::singleShot(50, [path] {
            auto* dialog = qobject_cast<QFileDialog*>(QApplication::activeModalWidget()); if (!dialog) return;
            dialog->findChild<QLineEdit*>("fileNameEdit")->setText(path); QMetaObject::invokeMethod(dialog, "accept");
        });
        trigger(window, "저장(&S)"); check(!doc.dirty() && store.load(path.toUtf8().toStdString()).project == *doc.project(), "UI save lost baseline/progress history");
        jobs::Document reopened(store); reopened.open(path.toUtf8().toStdString(), true);
        check(reopened.project()->baselines[0] == frozen && reopened.project()->progress.size() == 2 && jobs::compareToBaseline(*reopened.project(), frozen.id).forecastFinishVariance == 720,
              "reopened UI input comparison mismatch");
        window.close();
        jobs::Document seq(store); seq.create("F08 Gantt", "2026-10-26", {"A", "B"}); seq.setDuration(0, 960); seq.setDuration(1, 480);
        seq.setPredecessors(1, {seq.project()->tasks[0].id}); seq.setStatusAt("2026-10-27 14:00");
        for (int i = 0; i < 2; ++i) {
            jobs::ProgressEntry e; e.taskId = seq.project()->tasks[std::size_t(i)].id; e.actualStart = i ? "2026-10-26 14:00" : "2026-10-26 09:00";
            e.actualDurationMinutes = i ? 240 : 480; e.remainingMinutes = i ? 240 : 480; e.reporter = "PM"; e.reason = "F08"; e.recordedAt = "2026-10-27T05:00:00Z"; seq.recordProgress(e);
        }
        auto result = jobs::calculate(*seq.project()); jobs::GanttView retained;
        retained.setPlan(&*seq.project(), &result, {0, 1}); bool foundLink = false;
        for (auto* item : retained.scene()->items()) if (item->data(1).toString() == "dependency") {
            auto* pathItem = dynamic_cast<QGraphicsPathItem*>(item); const auto endpoint = pathItem->path().currentPosition();
            check(std::abs(endpoint.x() - (2.0 + 14.0 / 24.0) * 64.0) < 1e-6, "retained FS arrow points to historical actual start instead of remaining resume"); foundLink = true;
        }
        check(foundLink && result.tasks[1].start == "2026-10-26 14:00", "F08 Gantt lost immutable actual start");
        std::cout << "PASS: baseline/status/progress dialogs, F09 comparison +8h, missing report coverage, baseline overlay/selection, invalid report error, immutable correction/versioning, UI save/reopen\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
