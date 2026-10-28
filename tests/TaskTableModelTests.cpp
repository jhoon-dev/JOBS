#include "ui/TaskTableModel.h"
#include "persistence/JsonProjectStore.h"
#include "ui/TaskInput.h"
#include <QAbstractItemModelTester>
#include <QApplication>
#include <QBrush>
#include <QPersistentModelIndex>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QTemporaryDir>
#include <iostream>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
int main(int argc, char **argv) {
    QApplication app(argc, argv);
    try {
        jobs::JsonProjectStore store;
        jobs::Document doc(store);
        doc.create("model", "2026-10-26", {"Alpha", "Beta", "Gamma"});
        doc.setDuration(0, 480);
        doc.setDuration(1, 240);
        doc.setDuration(2, 0);
        const auto firstId = doc.project()->tasks[0].id;
        doc.setPredecessors(1, {firstId});
        jobs::TaskTableModel model(doc);
        QAbstractItemModelTester tester(&model, QAbstractItemModelTester::FailureReportingMode::Fatal);
        std::vector<std::size_t> rows{0, 1, 2};
        model.synchronize(rows);
        check(model.rowCount() == 3 && model.columnCount() == 14 && model.rowCount(model.index(0, 0)) == 0,
              "model dimensions/parent contract");
        check(model.data(model.index(1, 2)).toString() == "1" && model.headerData(1, Qt::Vertical).toString() == "2",
              "canonical row/dependency numbers changed");
        check((model.flags(model.index(0, 0)) & Qt::ItemIsEditable) &&
                  !(model.flags(model.index(0, 3)) & Qt::ItemIsEditable),
              "model flags changed");
        check(model.data(model.index(0, 8)).toString() == QString::fromStdString(firstId) &&
                  model.data(model.index(0, 0), Qt::UserRole).toString() == QString::fromStdString(firstId),
              "stable identity role missing");
        model.editRequested = [&](std::string id, int column, const QString &value) {
            std::size_t row = 0;
            while (doc.project()->tasks[row].id != id)
                ++row;
            if (column == 0)
                doc.renameTask(row, value.toStdString());
            else if (column == 1)
                doc.setDuration(row, jobs::taskDurationInput(value));
            else
                doc.setDependencies(row, jobs::taskDependencyInput(value, *doc.project(), id));
            model.synchronize(rows);
            return true;
        };
        QSignalSpy resets(&model, &QAbstractItemModel::modelReset);
        QSignalSpy updates(&model, &QAbstractItemModel::dataChanged);
        const QPersistentModelIndex persistent(model.index(1, 0));
        check(model.setData(model.index(0, 0), "Renamed") && doc.project()->tasks[0].name == "Renamed" &&
                  resets.isEmpty() && !updates.isEmpty() && persistent.isValid(),
              "ordinary edit reset the model/index or failed to commit");
        check(!model.setData(model.index(0, 3), "bad"), "readonly result column accepted edit");
        QStandardItemModel foreign(1, 1);
        check(!model.setData(foreign.index(0, 0), "foreign") && !model.data(foreign.index(0, 0)).isValid(),
              "foreign index accepted");
        doc.applyCalculation(jobs::calculate(*doc.project()));
        model.synchronize(rows);
        check(model.data(model.index(0, 3)).toString() == "2026-10-26 09:00" &&
                  model.data(model.index(1, 4)).toString() == "2026-10-27 14:00",
              "scheduled columns changed");
        doc.setDuration(0, 240);
        model.synchronize(rows);
        check(model.data(model.index(0, 3)).toString() == "미계산", "stale schedule remained visible");
        jobs::ScheduleResult conflict;
        conflict.documentId = doc.project()->documentId;
        conflict.inputRevision = doc.project()->revision;
        conflict.profile = doc.project()->calculationProfile;
        conflict.status = jobs::CalculationStatus::Conflict;
        conflict.diagnostics = {{"TEST", {"<unsafe & text>"}, {firstId}}};
        doc.applyCalculation(conflict);
        model.synchronize(rows);
        check(model.data(model.index(0, 0), Qt::BackgroundRole).value<QBrush>().color() == QColor("#ffddd7") &&
                  model.data(model.index(0, 0), Qt::ToolTipRole).toString().contains("&lt;unsafe &amp; text&gt;"),
              "conflict color/escaped tooltip changed");
        rows = {2, 1};
        model.synchronize(rows);
        check(!persistent.isValid() && resets.size() == 1 && model.headerData(0, Qt::Vertical).toString() == "3" &&
                  model.data(model.index(1, 2)).toString() == "1",
              "view reset lost original dependency identity");
        const auto displayed = model.data(model.index(0, 0));
        doc.create("replaced", "2026-10-26", {"New"});
        check(model.data(model.index(0, 0)) == displayed && !model.setData(model.index(0, 0), "wrong session"),
              "live document replacement invalidated owned display data or accepted stale edit");
        rows = {0};
        model.synchronize(rows);
        check(model.data(model.index(0, 0)).toString() == "New", "session reset did not show new document");
        QTemporaryDir directory;
        doc.saveAs(directory.filePath("Readonly.jobs").toUtf8().toStdString());
        doc.open(doc.path(), true);
        model.synchronize(rows);
        check(!(model.flags(model.index(0, 0)) & Qt::ItemIsEditable) && !model.setData(model.index(0, 0), "readonly"),
              "readonly model accepted edit");
        std::cout << "PASS: Qt model tester, dimensions/roles/flags, sparse notifications/persistent index, "
                     "dates/diagnostics, view mapping and session/read-only safety\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
