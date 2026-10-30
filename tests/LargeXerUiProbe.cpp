#include "persistence/PrimaveraXer.h"
#include "persistence/JsonProjectStore.h"
#include "ui/MainWindow.h"
#include "ui/GanttView.h"
#include "TaskTableTest.h"
#include <QAction>
#include <QApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScrollBar>
#include <QSysInfo>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <iostream>

namespace {
void require(bool value, const char *reason) {
    if (!value)
        throw std::runtime_error(reason);
}
double milliseconds(const QElapsedTimer &timer) {
    return double(timer.nsecsElapsed()) / 1e6;
}
QJsonObject distribution(const std::vector<double> &samples) {
    auto sorted = samples;
    std::sort(sorted.begin(), sorted.end());
    QJsonArray raw;
    for (auto value : samples)
        raw.append(value);
    return {{"samples_ms", raw},
            {"p50_ms", sorted[(sorted.size() - 1) / 2]},
            {"p95_ms", sorted[std::size_t(std::ceil(sorted.size() * .95)) - 1]},
            {"max_ms", sorted.back()}};
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("JOBS native XER measurement");
    const auto args = app.arguments();
    if (args.size() != 3) {
        std::cerr << "usage: jobs_large_xer_ui_probe input.xer result.json\n";
        return 2;
    }
    QJsonObject report{
        {"scope", "same importer/Document/MainWindow; native local measurement, not external UI tool latency"},
        {"qt", QT_VERSION_STR},
        {"os", QSysInfo::prettyProductName()},
        {"platform", QApplication::platformName()}};
    int code = 0;
    try {
        QElapsedTimer clock;
        clock.start();
        auto bytes = jobs::readPrimaveraXer(args[1]);
        auto exchange = jobs::importPrimaveraXer(bytes, QFileInfo(args[1]).fileName());
        report["read_import_ms"] = milliseconds(clock);
        report["source_sha256"] =
            QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        report["tasks"] = qint64(exchange.project.tasks.size());
        require(exchange.project.tasks.size() == 1000 || exchange.project.tasks.size() == 10000,
                "unexpected fixture size");
        QTemporaryDir dir;
        require(dir.isValid(), "temporary directory unavailable");
        clock.restart();
        auto published = jobs::publishPrimaveraXer(exchange, dir.filePath("exchange"));
        report["publish_exchange_ms"] = milliseconds(clock);
        jobs::JsonProjectStore files;
        jobs::Document document(files);
        clock.restart();
        document.open((published + "/project.jobs").toUtf8().toStdString());
        report["open_document_ms"] = milliseconds(clock);
        clock.restart();
        jobs::MainWindow window(document);
        window.resize(1440, 900);
        report["construct_window_ms"] = milliseconds(clock);
        auto *table = taskTable(window);
        auto *gantt = dynamic_cast<jobs::GanttView *>(window.findChild<QGraphicsView *>("ganttView"));
        require(table && gantt, "working panes missing");
        clock.restart();
        window.show();
        require(QTest::qWaitForWindowExposed(&window, 3000), "native measurement window not exposed");
        const auto paint = [&] {
            QCoreApplication::sendPostedEvents(nullptr, QEvent::LayoutRequest);
            table->viewport()->repaint();
            gantt->viewport()->repaint();
        };
        paint();
        report["show_initial_paint_ms"] = milliseconds(clock);
        report["device_pixel_ratio"] = window.devicePixelRatioF();
        QAction *calculate = nullptr;
        for (auto *action : window.findChildren<QAction *>())
            if (action->shortcut() == QKeySequence(Qt::Key_F9))
                calculate = action;
        require(calculate && calculate->isEnabled(), "calculate action unavailable");
        QElapsedTimer heartbeatClock;
        heartbeatClock.start();
        qint64 previousBeat = 0;
        std::vector<double> heartbeat;
        QTimer pulse;
        pulse.setInterval(10);
        QObject::connect(&pulse, &QTimer::timeout, [&] {
            auto now = heartbeatClock.elapsed();
            heartbeat.push_back(double(now - previousBeat));
            previousBeat = now;
        });
        pulse.start();
        clock.restart();
        calculate->trigger();
        while (!calculate->isEnabled() && clock.elapsed() < 180000)
            QTest::qWait(2);
        report["f9_calculation_and_commit_ms"] = milliseconds(clock);
        report["preparation_ms"] = window.property("schedulePreparationMs").toDouble();
        report["worker_ms"] = window.property("scheduleWorkerMs").toDouble();
        report["gui_commit_ms"] = window.property("scheduleCommitMs").toDouble();
        require(calculate->isEnabled(), "180 second calculation timeout");
        require(document.calculation() && document.calculation()->status == jobs::CalculationStatus::Valid,
                "calculation incomplete or conflicting");
        clock.restart();
        paint();
        report["calculated_view_repaint_ms"] = milliseconds(clock);
        heartbeat.push_back(double(heartbeatClock.elapsed() - previousBeat));
        pulse.stop();
        report["finish"] = QString::fromStdString(*document.calculation()->projectFinish);
        report["calculation_and_first_paint_heartbeat"] = distribution(heartbeat);
        const int rows = table->rowCount();
        std::vector<double> selection, scrolling;
        int maximumItems = 0;
        for (int sample = 0; sample < 32; ++sample) {
            const int row = sample == 0 ? 0 : sample == 31 ? rows - 1 : (sample * 7919) % rows;
            clock.restart();
            table->selectRow(row);
            table->scrollTo(table->model()->index(row, 0), QAbstractItemView::PositionAtCenter);
            paint();
            selection.push_back(milliseconds(clock));
            require(table->verticalScrollBar()->value() == gantt->verticalScrollBar()->value(),
                    "selection scroll mismatch");
            const auto id = QString::fromStdString(document.project()->tasks[std::size_t(row)].id);
            bool highlighted = false;
            auto items = gantt->scene()->items();
            maximumItems = std::max(maximumItems, int(items.size()));
            for (auto *item : items)
                if (item->data(0).toString() == id && (item->data(1) == "task" || item->data(1) == "milestone"))
                    if (auto *shape = dynamic_cast<QAbstractGraphicsShapeItem *>(item))
                        highlighted = highlighted || shape->pen().widthF() >= 3;
            require(highlighted, "selected ID missing or not highlighted in Gantt");
            const int maximum = table->verticalScrollBar()->maximum();
            const int target = sample == 31 ? maximum : int((qint64(sample) * 7919) % std::max(1, maximum));
            clock.restart();
            table->verticalScrollBar()->setValue(target);
            paint();
            scrolling.push_back(milliseconds(clock));
            require(table->verticalScrollBar()->value() == gantt->verticalScrollBar()->value(),
                    "scroll panes diverged");
        }
        report["row_selection_and_paint"] = distribution(selection);
        report["vertical_scroll_and_paint"] = distribution(scrolling);
        report["max_scene_items_observed"] = maximumItems;
        report["last_row_checked"] = rows;
        report["status"] = "passed";
        window.hide();
    } catch (const std::exception &error) {
        report["status"] = "failed";
        report["error"] = QString::fromUtf8(error.what());
        std::cerr << error.what() << '\n';
        code = 1;
    }
    QFile output(args[2]);
    if (!output.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return 2;
    auto json = QJsonDocument(report).toJson();
    require(output.write(json) == json.size(), "report write failed");
    std::cout << json.toStdString();
    return code;
}
