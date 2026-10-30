#include "formatting/QtDiagnosticText.h"
#include "persistence/ExchangePackage.h"
#include "persistence/JsonProjectStore.h"
#include "domain/CalculationControl.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTemporaryDir>
#include <stdexcept>
namespace jobs {
namespace {} // namespace
QString publishPlanExchange(const Project &project, const QByteArray &payload, QJsonObject report, bool importing,
                            const QString &extension, const QString &destination, std::function<void()> beforePublish) {
    calculationCheckpoint();
    validate(project);
    if (extension != "xml" && extension != "xer")
        throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_001, {});
    auto target = QFileInfo(destination);
    auto parent = target.dir().canonicalPath();
    if (parent.isEmpty() || target.fileName().isEmpty() || target.fileName() == "." || target.fileName() == ".." ||
        target.exists() || target.isSymLink())
        throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_002, {});
    QDir directory(parent);
    auto finalPath = directory.filePath(target.fileName());
    QTemporaryDir stage(directory.filePath(".jobs-exchange-XXXXXX"));
    if (!stage.isValid())
        throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_003, {});
    auto write = [&](const QString &name, const QByteArray &bytes) {
        calculationCheckpoint();
        QSaveFile f(stage.filePath(name));
        f.setDirectWriteFallback(false);
        if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit())
            throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_004, {diagnosticArgument(name)});
        QFile check(stage.filePath(name));
        if (!check.open(QIODevice::ReadOnly) || check.readAll() != bytes)
            throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_005, {diagnosticArgument(name)});
    };
    JsonProjectStore store;
    auto jobsName = importing ? "project.jobs" : "source.jobs";
    store.save(stage.filePath(jobsName).toUtf8().toStdString(), project, {});
    calculationCheckpoint();
    if (store.load(stage.filePath(jobsName).toUtf8().toStdString()).project != project)
        throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_006, {});
    write((importing ? "original." : "project.") + extension, payload);
    if (!importing) {
        QFile f(stage.filePath(jobsName));
        if (!f.open(QIODevice::ReadOnly))
            throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_007, {});
        auto source = report["source"].toObject();
        source["sha256"] =
            QString::fromLatin1(QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256).toHex());
        report["source"] = source;
    }
    write("conversion.json", QJsonDocument(report).toJson());
    if (beforePublish)
        beforePublish();
    // This is the final cancellation boundary. A completed rename is a published result.
    calculationCheckpoint();
    if (QFileInfo::exists(finalPath) || !directory.rename(QFileInfo(stage.path()).fileName(), target.fileName()))
        throw AppError(ErrorCode::FILE_EXCHANGEPACKAGE_008, {});
    stage.setAutoRemove(false);
    return finalPath;
}
} // namespace jobs
