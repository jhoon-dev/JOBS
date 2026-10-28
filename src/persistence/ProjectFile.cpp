#include "persistence/ProjectFile.h"
#include "persistence/ProjectJson.h"
#include <QCryptographicHash>
#include <QJsonDocument>
#include <QJsonObject>
#include <stdexcept>

namespace jobs {
namespace {
[[noreturn]] void fail(const QString& message) {
    throw std::runtime_error(message.toUtf8().toStdString());
}
}
Project decodeProjectFile(const QByteArray& bytes) {
    if (bytes.size() > projectFileLimitBytes)
        fail("초기 형식의 파일 한도(64 MiB)를 초과했습니다.");
    QJsonParseError error;
    const auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject())
        fail("손상된 JSON 프로젝트 파일입니다.");
    const auto root = json.object();
    const QStringList allowed{"format", "schemaVersion", "payload", "sha256"};
    for (const auto& key : root.keys())
        if (!allowed.contains(key)) fail("지원하지 않는 필드: " + key);
    if (root.value("format") != "JOBS") fail("JOBS 프로젝트 형식이 아닙니다.");
    const auto version = root.value("schemaVersion");
    if (!version.isDouble() || version.toDouble() != version.toInt() ||
        version.toInt() < 1 || version.toInt() > 9)
        fail("지원하지 않는 파일 버전입니다. 원본은 변경하지 않았습니다.");
    if (!root.value("payload").isObject()) fail("프로젝트 본문이 없습니다.");
    const auto body = root.value("payload").toObject();
    const auto digest = QCryptographicHash::hash(
        QJsonDocument(body).toJson(QJsonDocument::Compact), QCryptographicHash::Sha256).toHex();
    if (root.value("sha256").toString().toLatin1() != digest)
        fail("파일 무결성 검사에 실패했습니다.");
    return json::decodeProject(body, version.toInt());
}
}
