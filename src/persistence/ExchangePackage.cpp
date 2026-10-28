#include "persistence/ExchangePackage.h"
#include "persistence/JsonProjectStore.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QSaveFile>
#include <QTemporaryDir>
#include <stdexcept>
namespace jobs {
namespace {
[[noreturn]] void fail(const QString &text) {
    throw std::runtime_error(text.toUtf8().toStdString());
}
} // namespace
QString publishPlanExchange(const Project &project, const QByteArray &payload, QJsonObject report, bool importing,
                            const QString &extension, const QString &destination) {
    validate(project);
    if (extension != "xml" && extension != "xer")
        fail("지원하지 않는 교환 형식");
    auto target = QFileInfo(destination);
    auto parent = target.dir().canonicalPath();
    if (parent.isEmpty() || target.fileName().isEmpty() || target.fileName() == "." || target.fileName() == ".." ||
        target.exists() || target.isSymLink())
        fail("존재하지 않는 새 교환 폴더를 지정하세요.");
    QDir directory(parent);
    auto finalPath = directory.filePath(target.fileName());
    QTemporaryDir stage(directory.filePath(".jobs-exchange-XXXXXX"));
    if (!stage.isValid())
        fail("교환 임시 폴더 생성 실패");
    auto write = [&](const QString &name, const QByteArray &bytes) {
        QSaveFile f(stage.filePath(name));
        f.setDirectWriteFallback(false);
        if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit())
            fail("교환 파일 쓰기 실패: " + name);
        QFile check(stage.filePath(name));
        if (!check.open(QIODevice::ReadOnly) || check.readAll() != bytes)
            fail("교환 파일 재열기 검증 실패: " + name);
    };
    JsonProjectStore store;
    auto jobsName = importing ? "project.jobs" : "source.jobs";
    store.save(stage.filePath(jobsName).toUtf8().toStdString(), project, {});
    if (store.load(stage.filePath(jobsName).toUtf8().toStdString()).project != project)
        fail("JOBS 사본 검증 실패");
    write((importing ? "original." : "project.") + extension, payload);
    if (!importing) {
        QFile f(stage.filePath(jobsName));
        if (!f.open(QIODevice::ReadOnly))
            fail("원본 사본 읽기 실패");
        auto source = report["source"].toObject();
        source["sha256"] =
            QString::fromLatin1(QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256).toHex());
        report["source"] = source;
    }
    write("conversion.json", QJsonDocument(report).toJson());
    if (QFileInfo::exists(finalPath) || !directory.rename(QFileInfo(stage.path()).fileName(), target.fileName()))
        fail("새 교환 폴더 게시 실패 · 기존 대상은 보존했습니다.");
    stage.setAutoRemove(false);
    return finalPath;
}
} // namespace jobs
