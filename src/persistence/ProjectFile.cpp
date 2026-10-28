#include "formatting/QtDiagnosticText.h"
#include "persistence/ProjectFile.h"
#include "persistence/ProjectJson.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

namespace jobs {
Project decodeProjectFile(const QByteArray &bytes) {
    if (bytes.size() > projectFileLimitBytes)
        throw AppError(ErrorCode::FILE_SIZE_LIMIT, {});
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject())
        throw AppError(ErrorCode::FILE_JSON_INVALID, {});
    const auto root = json.object();
    const QStringList allowed{"format", "schemaVersion", "payload", "sha256"};
    for (const auto &key : root.keys())
        if (!allowed.contains(key))
            throw AppError(ErrorCode::FILE_UNKNOWN_FIELD, {diagnosticArgument(key)});
    if (root.value("format") != "JOBS")
        throw AppError(ErrorCode::FILE_PROJECTFILE_004, {});
    const auto version = root.value("schemaVersion");
    if (!version.isDouble() || version.toDouble() != version.toInt() || version.toInt() < 1 || version.toInt() > 9)
        throw AppError(ErrorCode::FILE_VERSION_UNSUPPORTED, {});
    if (!root.value("payload").isObject())
        throw AppError(ErrorCode::FILE_PROJECTFILE_006, {});
    const auto body = root.value("payload").toObject();
    const auto digest =
        QCryptographicHash::hash(QJsonDocument(body).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256)
            .toHex();
    if (root.value("sha256").toString().toLatin1() != digest)
        throw AppError(ErrorCode::FILE_INTEGRITY_FAILED, {});
    return json::decodeProject(body, version.toInt());
}
} // namespace jobs
