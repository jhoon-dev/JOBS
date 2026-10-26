#include "ui/MainWindow.h"
#include "ui/ReportDialog.h"
#include "persistence/JsonProjectStore.h"
#include <QApplication>
#include <QAction>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QTableView>
#include <QTabWidget>
#include <QFileDialog>
#include <QLineEdit>
#include <QDateEdit>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <iostream>
#include <cstdlib>
namespace {
void check(bool b, const char* m) { if (!b) throw std::runtime_error(m); }
void callbackCheck(bool b, const char* m) { if (!b) { std::cerr << "FAIL: " << m << '\n'; std::exit(2); } }
bool ready(jobs::ReportDialog& dialog, const QString& needle = "") {
    for (int i = 0; i < 500; ++i) { QTest::qWait(10); if (dialog.findChild<QPushButton*>("exportXlsx")->isEnabled() && dialog.findChild<QLabel*>("reportMessage")->text().contains(needle)) return true; }
    return false;
}
QString cell(jobs::ReportDialog& d, const char* name, int row, int column) { auto* t = d.findChild<QTableView*>(name); check(t && t->model(), "missing table view"); return t->model()->index(row, column).data().toString(); }
}
int main(int argc, char* argv[]) {
    QApplication app(argc, argv); QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QTimer watchdog; watchdog.setSingleShot(true); QObject::connect(&watchdog, &QTimer::timeout, [] { std::cerr << "report UI timeout\n"; std::exit(2); }); watchdog.start(30000);
    try {
        QTemporaryDir dir; jobs::JsonProjectStore store; jobs::Document doc(store); doc.create("통합 프로젝트 현황", "2026-10-26", {"기본 설계", "전기 설계"}); doc.setCurrency("USD");
        auto wbs = doc.addWbs("기본 설계 단계"); doc.assignWbs(0, wbs); auto id = doc.project()->tasks[0].id;
        doc.setDuration(0, 960); doc.setDuration(1, 480); doc.setTaskCost({id, 1000, "uniform", "physical"}); doc.setTaskCost({doc.project()->tasks[1].id, 0, "finish", "physical"});
        auto resource = doc.putResource({{}, "설계 공유팀", "labor", 100, {}, {{"2026-10-26", 0}}}); doc.setAssignments(id, {{id, resource, 100}}); doc.setAssignments(doc.project()->tasks[1].id, {{doc.project()->tasks[1].id, resource, 100}});
        auto baseline = doc.captureBaseline("최초 승인", "현장 PM", "예산 승인", "2026-10-26T00:00:00Z"); doc.setStatusAt("2026-10-26 18:00");
        const std::vector<std::string> taskIds{id, doc.project()->tasks[1].id};
        for (const auto& taskId : taskIds) { jobs::CostEntry e; e.taskId = taskId; e.actualMinor = taskId == id ? 600 : 0; e.physicalBasisPoints = taskId == id ? 4000 : 0; e.reporter = "현장 PM"; e.reason = "청구/완료량 확인"; e.recordedAt = "2026-10-26T09:00:00Z"; doc.recordCost(e); }
        auto original = *doc.project(); auto dirty = doc.dirty(); jobs::MainWindow main(doc); main.show(); check(main.findChild<QAction*>("projectReports")->isEnabled(), "report menu disabled");
        jobs::ReportDialog dialog(doc, {}, baseline, &main); dialog.show(); check(ready(dialog), "report preparation failed");
        check(dialog.findChild<QLabel*>("dashboard_tasks")->text() == "2 개" && cell(dialog, "costs", 0, 2) == "10.00" && cell(dialog, "costs", 0, 4) == "5.00" && cell(dialog, "costs", 0, 5) == "4.00", "dashboard / report totals differ");
        dialog.findChild<QComboBox*>("reportFormula")->setCurrentIndex(1); check(ready(dialog) && dialog.findChild<QLabel*>("dashboard_eac")->text() == "15.00 USD", "EAC choice not recalculated");
        dialog.findChild<QComboBox*>("reportScope")->setCurrentIndex(1); check(ready(dialog) && dialog.findChild<QLabel*>("dashboard_tasks")->text() == "1 개" && cell(dialog, "resourceLoad", 0, 3) == "8.00" && cell(dialog, "resourceLoad", 0, 4) == "16.00", "scope hid shared demand");
        dialog.findChild<QDateEdit*>("reportTo")->setDate(QDate(2026, 10, 26)); dialog.findChild<QCheckBox*>("reportPeriod")->setChecked(true); check(ready(dialog) && dialog.findChild<QTableView*>("costDays")->model()->rowCount() == 1, "daily period not applied");
        QTest::qWait(30); dialog.grab().save("report-dashboard-smoke.png");
        auto* tabs = dialog.findChild<QTabWidget*>("reportTabs"); for (int i = 0; i < tabs->count(); ++i) if (tabs->tabText(i) == "기간별 자원 수요") tabs->setCurrentIndex(i); dialog.grab().save("report-resource-smoke.png");
        auto output = dir.filePath("export.xlsx");
        auto choose = [&](const QString& path, bool change, bool cancel) {
            QTimer::singleShot(30, [&, path, change, cancel] {
                auto* chooser = qobject_cast<QFileDialog*>(QApplication::activeModalWidget()); callbackCheck(chooser, "save chooser missing");
                if (change) doc.renameTask(0, "설계 변경");
                if (cancel) chooser->reject(); else { auto* name = chooser->findChild<QLineEdit*>("fileNameEdit"); callbackCheck(name, "filename field absent"); name->setText(path); QMetaObject::invokeMethod(chooser, "accept"); }
            });
        };
        choose(dir.filePath("export"), false, false); dialog.findChild<QPushButton*>("exportXlsx")->click();
        check(ready(dialog, "내보내기 완료") && QFile::exists(output), "UI XLSX export failed");
        auto pdf = dir.filePath("export.pdf"); choose(pdf, false, false); dialog.findChild<QPushButton*>("exportPdf")->click(); check(ready(dialog, "내보내기 완료") && QFile::exists(pdf), "UI PDF export failed");
        auto canceled = dir.filePath("cancel.xlsx"); choose(canceled, false, true); dialog.findChild<QPushButton*>("exportXlsx")->click(); check(!QFile::exists(canceled) && *doc.project() == original && doc.dirty() == dirty, "cancel/report/export mutated document");
        auto stale = dir.filePath("stale.xlsx"); choose(stale, true, false); dialog.findChild<QPushButton*>("exportXlsx")->click(); check(!QFile::exists(stale) && !dialog.findChild<QPushButton*>("exportXlsx")->isEnabled() && dialog.findChild<QLabel*>("reportMessage")->text().contains("변경"), "dialog change exported stale revision");
        dialog.findChild<QPushButton*>("refreshReport")->click(); check(ready(dialog), "refresh after edit failed");
        dialog.findChild<QPushButton*>("refreshReport")->click(); doc.renameTask(0, "준비 중 변경"); QTest::qWait(150); check(!dialog.findChild<QPushButton*>("exportXlsx")->isEnabled() && !dialog.findChild<QTableView*>("costs"), "stale worker result displayed"); dialog.reject();
        doc.saveAs(dir.filePath("report-source.jobs").toStdString()); jobs::Document viewer(store); viewer.open(doc.path(), true); jobs::ReportDialog readonly(viewer, wbs, baseline); readonly.show(); check(ready(readonly) && readonly.findChild<QPushButton*>("exportPdf")->isEnabled(), "readonly reports unavailable"); readonly.reject();
        doc.renameWbs(wbs, std::string("invalid-control-") + '\x01'); jobs::ReportDialog invalid(doc, wbs, baseline); invalid.show();
        for (int i = 0; i < 100 && !invalid.findChild<QLabel*>("reportMessage")->text().contains("보고서 오류"); ++i) QTest::qWait(10);
        check(invalid.findChild<QLabel*>("reportMessage")->text().contains("보고서 오류") && !invalid.findChild<QPushButton*>("exportPdf")->isEnabled(), "invalid source text escaped UI error handling"); invalid.reject();
        std::cout << "PASS: dashboard, report models, WBS/global demand, EAC/filter controls, XLSX/PDF dialogs, cancel/no mutation, stale snapshot during chooser/worker, readonly, invalid source text\n";
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
