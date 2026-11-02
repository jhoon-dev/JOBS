#include "persistence/JsonProjectStore.h"
#include "ui/ComparisonDialog.h"
#include "ui/CostAnalysisDialog.h"
#include "ui/DesktopWorkspace.h"
#include "ui/EditorUi.h"
#include "ui/RecoveryDialog.h"
#include "ui/ReportDialog.h"
#include "ui/ResourceComparisonDialog.h"
#include "ui/WorkspaceDialog.h"
#include "ui/UpdateUi.h"
#include "updates/UpdateController.h"
#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QScopeGuard>
#include <QScrollArea>
#include <QScrollBar>
#include <QTemporaryDir>
#include <QTabWidget>
#include <QTableView>
#include <QTest>
#include <algorithm>

class DialogLayoutTests : public QObject {
    Q_OBJECT
private slots:
    void longChoiceNamesDoNotForceWindowWidth() {
        QDialog dialog;
        auto *form = new QFormLayout(&dialog);
        auto *choice = new QComboBox;
        const auto name = QString("긴 프로젝트 / 기준선 이름 ").repeated(30);
        choice->addItem(name);
        form->addRow("기준선", choice);
        form->addRow(new QPushButton("닫기"));
        jobs::prepareEditorDialog(dialog);
        dialog.resize(640, 480);
        dialog.show();
        QTest::qWait(20);
        QCOMPARE(dialog.width(), 640);
        QVERIFY(choice->width() > 80 && choice->width() < dialog.width());
        QCOMPARE(choice->toolTip(), name);
        choice->addItem("다른 기준선");
        choice->setCurrentIndex(1);
        QCOMPARE(choice->toolTip(), QString("다른 기준선"));
    }
    void analysisDialogs_data() {
        QTest::addColumn<QString>("kind");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<bool>("dark");
        for (const auto &kind : {"report", "comparison", "cost", "resource", "recovery", "workspace", "update"})
            for (const auto &size : {QSize(640, 480), QSize(1024, 650), QSize(1440, 900)})
                for (bool dark : {false, true}) {
                    const auto name = QString("%1-%2-%3").arg(kind).arg(size.width()).arg(dark ? "dark" : "light");
                    QTest::newRow(qPrintable(name)) << QString(kind) << size << dark;
                }
    }
    void analysisDialogs() {
        QFETCH(QString, kind);
        QFETCH(QSize, size);
        QFETCH(bool, dark);
        const auto original = QApplication::palette();
        const auto restore = qScopeGuard([&] { QApplication::setPalette(original); });
        auto palette = original;
        palette.setColor(QPalette::Window, QColor(dark ? "#242a33" : "#f4f5f7"));
        palette.setColor(QPalette::Base, QColor(dark ? "#19202a" : "#ffffff"));
        palette.setColor(QPalette::AlternateBase, QColor(dark ? "#303947" : "#e7eaf0"));
        palette.setColor(QPalette::Button, palette.color(QPalette::AlternateBase));
        for (auto role : {QPalette::WindowText, QPalette::Text, QPalette::ButtonText})
            palette.setColor(role, dark ? Qt::white : Qt::black);
        QApplication::setPalette(palette);
        QTemporaryDir directory;
        auto store = std::make_shared<jobs::JsonProjectStore>();
        jobs::Document document(*store);
        document.create("화면 배치 점검", "2026-11-02", {"설계 검토", "현장 시공"});
        document.setDuration(0, 480);
        document.setDuration(1, 960);
        const auto baseline = document.captureBaseline(QString("긴 승인 기준선 이름 ").repeated(20).toStdString(), "PM",
                                                       "검증", "2026-11-02T00:00:00Z");
        document.setStatusAt("2026-11-02 18:00");
        jobs::DesktopWorkspace workspace(store, std::make_shared<jobs::JsonRecoveryStore>(directory.path()));
        auto updateOptions = jobs::updateConfig();
        updateOptions.channel = jobs::UpdateChannel::Unpackaged;
        updateOptions.stateDirectory = directory.filePath("updates");
        updateOptions.settingsFile = directory.filePath("settings.ini");
        jobs::UpdateController updates(updateOptions);
        QMainWindow updateWindow;
        std::unique_ptr<QDialog> dialog;
        if (kind == "report")
            dialog = std::make_unique<jobs::ReportDialog>(document, "", baseline);
        else if (kind == "comparison")
            dialog = std::make_unique<jobs::ComparisonDialog>(document, baseline);
        else if (kind == "cost")
            dialog = std::make_unique<jobs::CostAnalysisDialog>(document, baseline);
        else if (kind == "resource")
            dialog = std::make_unique<jobs::ResourceComparisonDialog>(document);
        else if (kind == "recovery")
            dialog = std::make_unique<jobs::RecoveryDialog>(jobs::JsonRecoveryStore(directory.path()), nullptr,
                                                            jobs::RecoveryScan{});
        else if (kind == "workspace")
            dialog = std::make_unique<jobs::WorkspaceDialog>(workspace);
        else {
            jobs::attachUpdateUi(updateWindow, updates, {[](const QUrl &) { return false; }});
            updateWindow.findChild<QAction *>("applicationUpdates")->trigger();
            dialog.reset(updateWindow.findChild<QDialog *>("applicationUpdateDialog"));
        }
        // These analysis screens are prepared by their MainWindow entry points.
        if (kind == "comparison" || kind == "cost" || kind == "resource")
            jobs::prepareEditorDialog(*dialog);
        dialog->resize(size);
        dialog->show();
        QTest::qWait(80);
        auto *dashboard = dialog->findChild<QScrollArea *>("reportDashboardScroll");
        if (kind == "report")
            QTRY_VERIFY((dashboard = dialog->findChild<QScrollArea *>("reportDashboardScroll")));
        const auto capture = qEnvironmentVariable("JOBS_LAYOUT_AUDIT_DIR");
        if (!capture.isEmpty()) {
            QVERIFY(QDir().mkpath(capture));
            QVERIFY(dialog->grab().save(QDir(capture).filePath(QString::fromLatin1(QTest::currentDataTag()) + ".png")));
        }
        QCOMPARE(dialog->size(), size);
        auto *scroll = dialog->findChild<QScrollArea *>("editorScrollArea");
        QVERIFY(scroll);
        for (auto *button : dialog->findChildren<QPushButton *>())
            if (button->isVisible())
                QVERIFY(button->height() >= 32);
        if (dashboard) {
            QTRY_COMPARE(dashboard->horizontalScrollBar()->maximum(), 0);
            auto *grid = dashboard->widget()->findChild<QGridLayout *>();
            QVERIFY(grid);
            if (size.width() == 640)
                QVERIFY(grid->columnCount() <= 4 && grid->rowCount() >= 6);
            dashboard->verticalScrollBar()->setValue(dashboard->verticalScrollBar()->maximum());
            auto *trend = dashboard->widget()->findChild<QWidget *>("reportCostTrend");
            QVERIFY(trend);
            QVERIFY(trend->mapTo(dashboard->viewport(), trend->rect().bottomRight()).y() <=
                    dashboard->viewport()->height());
            for (const auto &next : {QSize(1024, 650), QSize(640, 480), size}) {
                dialog->resize(next);
                QTest::qWait(10);
                QTRY_COMPARE(dashboard->horizontalScrollBar()->maximum(), 0);
                QCOMPARE(dashboard->widget()->findChildren<QFrame *>("reportKpiCard").size(), 12);
            }
            // Missing values remain readable on the highlighted cells in both palettes.
            auto *costs = dialog->findChild<QTableView *>("costs");
            QVERIFY(costs && costs->model());
            auto *model = costs->model();
            bool missing = false;
            for (int column = 0; column < model->columnCount(); ++column) {
                const auto index = model->index(0, column);
                if (model->data(index, Qt::BackgroundRole).isValid()) {
                    missing = true;
                    const auto ink = model->data(index, Qt::ForegroundRole).value<QColor>();
                    QVERIFY(ink.isValid() && ink.lightness() < 100);
                }
            }
            QVERIFY(missing);
        }
        // Primary actions stay visible even when the table/form needs scrolling.
        const auto buttons = dialog->findChildren<QPushButton *>();
        QVERIFY(std::any_of(buttons.begin(), buttons.end(), [&](auto *button) {
            return button->isVisible() && !scroll->isAncestorOf(button) &&
                   dialog->rect().contains(button->mapTo(dialog.get(), button->rect().center()));
        }));
        if (kind == "cost" && !capture.isEmpty()) {
            dialog->findChild<QTabWidget *>("costTabs")->setCurrentIndex(3);
            auto *chart = dialog->findChild<QWidget *>("costChart");
            scroll->ensureWidgetVisible(chart);
            QTest::qWait(20);
            QVERIFY(dialog->grab().save(
                QDir(capture).filePath(QString::fromLatin1(QTest::currentDataTag()) + "-chart.png")));
        }
    }
};
QTEST_MAIN(DialogLayoutTests)
#include "DialogLayoutTests.moc"
