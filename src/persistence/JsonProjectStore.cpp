#include "formatting/QtDiagnosticText.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/ProjectJson.h"
#include "persistence/ProjectFile.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSaveFile>
#include <QUuid>
#include <cmath>
#include <stdexcept>
#include <filesystem>

namespace jobs {
namespace {
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
std::string str(const QString &s) {
    return s.toUtf8().toStdString();
}

QByteArray hash(const QByteArray &bytes) {
    return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex();
}
QByteArray read(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw AppError(ErrorCode::FILE_OPEN_FAILED, {diagnosticArgument(f.errorString())},
                       ErrorContext{.path = str(path)});
    if (f.size() > projectFileLimitBytes)
        throw AppError(ErrorCode::FILE_SIZE_LIMIT, {});
    auto bytes = f.readAll();
    if (f.error() != QFileDevice::NoError)
        throw AppError(ErrorCode::FILE_READ_FAILED, {diagnosticArgument(f.errorString())},
                       ErrorContext{.path = str(path)});
    return bytes;
}
class Lock final : public WriteLease {
public:
    QLockFile file;
    explicit Lock(const QString &path) : file(path + ".lock") {
        file.setStaleLockTime(0);
        if (!file.tryLock(0))
            throw AppError(ErrorCode::FILE_LOCKED, {}, ErrorContext{.path = str(path)});
    }
};
} // namespace
std::string JsonProjectStore::newId() const {
    return str(QUuid::createUuid().toString(QUuid::WithoutBraces));
}
std::string JsonProjectStore::normalizePath(const std::string &path) const {
    QFileInfo info(qs(path));
    auto canonical = info.canonicalFilePath();
    if (canonical.isEmpty()) {
        auto parent = info.dir().canonicalPath();
        if (parent.isEmpty())
            throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_005, {});
        canonical = QDir(parent).filePath(info.fileName());
    }
    return str(canonical);
}
bool JsonProjectStore::sameFile(const std::string &left, const std::string &right) const {
    if (left.empty() || right.empty())
        return false;
    if (left == right)
        return true;
    std::error_code error;
    // Ask the OS about existing files, including case-sensitive NTFS directories.
    const bool equivalent =
        std::filesystem::equivalent(std::filesystem::path(std::u8string(left.begin(), left.end())),
                                    std::filesystem::path(std::u8string(right.begin(), right.end())), error);
    return !error && equivalent;
}
std::unique_ptr<WriteLease> JsonProjectStore::lock(const std::string &path) {
    return std::make_unique<Lock>(qs(path));
}
StoredProject JsonProjectStore::load(const std::string &path) const {
    auto bytes = read(qs(path));
    auto project = decodeProjectFile(bytes);
    const bool legacy = QJsonDocument::fromJson(bytes).object().value("schemaVersion") != QJsonValue(9);
    return {std::move(project), hash(bytes).toStdString(), legacy};
}
std::string JsonProjectStore::save(const std::string &path, const Project &project, const std::string &expected) {
    validate(project);
    auto body = json::encodeProject(project);
    auto bytes =
        QJsonDocument(
            QJsonObject{{"format", "JOBS"},
                        {"schemaVersion", 9},
                        {"payload", body},
                        {"sha256", QString::fromLatin1(hash(QJsonDocument(body).toJson(QJsonDocument::Compact)))}})
            .toJson();
    if (decodeProjectFile(bytes) != project)
        throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_006, {});
    const auto filename = qs(path);
    const auto unchanged = [&] {
        if (expected.empty()) {
            if (QFileInfo::exists(filename))
                throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_007, {});
        } else if (!QFileInfo::exists(filename) || hash(read(filename)).toStdString() != expected) {
            throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_008, {});
        }
    };
    unchanged();
    QSaveFile f(filename);
    f.setDirectWriteFallback(false);
    if (!f.open(QIODevice::WriteOnly))
        throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_009, {diagnosticArgument(f.errorString())});
    if (f.write(bytes) != bytes.size() || !f.flush())
        throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_010, {diagnosticArgument(f.errorString())});
    unchanged();
    if (!f.commit())
        throw AppError(ErrorCode::FILE_JSONPROJECTSTORE_011, {diagnosticArgument(f.errorString())});
    return hash(bytes).toStdString();
}
} // namespace jobs
