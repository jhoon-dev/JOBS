#include "persistence/JsonProjectStore.h"
#include "persistence/ProjectJson.h"
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

namespace jobs {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
[[noreturn]] void fail(const QString& text) { throw std::runtime_error(str(text)); }
QByteArray hash(const QByteArray& bytes) { return QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex(); }
QByteArray read(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) fail("파일을 읽을 수 없습니다: " + f.errorString());
    if (f.size() > 64 * 1024 * 1024) fail("초기 형식의 파일 한도(64 MiB)를 초과했습니다.");
    auto bytes = f.readAll();
    if (f.error() != QFileDevice::NoError) fail("파일 읽기 실패: " + f.errorString());
    return bytes;
}
void keys(const QJsonObject& o, const QStringList& allowed) {
    for (const auto& key : o.keys()) if (!allowed.contains(key)) fail("지원하지 않는 필드: " + key);
}
Project decode(const QByteArray& bytes) {
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject()) fail("손상된 JSON 프로젝트 파일입니다.");
    auto root = json.object(); keys(root, {"format", "schemaVersion", "payload", "sha256"});
    if (root.value("format") != "JOBS") fail("JOBS 프로젝트 형식이 아닙니다.");
    auto v = root.value("schemaVersion");
    if (!v.isDouble() || v.toDouble() != v.toInt() || v.toInt() < 1 || v.toInt() > 8)
        fail("지원하지 않는 파일 버전입니다. 원본은 변경하지 않았습니다.");
    if (!root.value("payload").isObject()) fail("프로젝트 본문이 없습니다.");
    auto body = root.value("payload").toObject();
    if (root.value("sha256").toString().toLatin1() != hash(QJsonDocument(body).toJson(QJsonDocument::Compact)))
        fail("파일 무결성 검사에 실패했습니다.");
    return json::decodeProject(body, v.toInt());
}
class Lock final : public WriteLease {
public:
    QLockFile file;
    explicit Lock(const QString& path) : file(path + ".lock") {
        file.setStaleLockTime(0);
        if (!file.tryLock(0)) fail("파일 잠금을 얻지 못했습니다. 다른 앱 사용 여부와 폴더 권한을 확인하거나 읽기 전용으로 여세요.");
    }
};
}
std::string JsonProjectStore::newId() const { return str(QUuid::createUuid().toString(QUuid::WithoutBraces)); }
std::string JsonProjectStore::normalizePath(const std::string& path) const {
    QFileInfo info(qs(path));
    auto canonical = info.canonicalFilePath();
    if (canonical.isEmpty()) {
        auto parent = info.dir().canonicalPath();
        if (parent.isEmpty()) fail("저장할 폴더가 없습니다.");
        canonical = QDir(parent).filePath(info.fileName());
    }
#ifdef Q_OS_WIN
    // Windows paths are normally case-insensitive; use one lock identity.
    canonical = canonical.toLower();
#endif
    return str(canonical);
}
std::unique_ptr<WriteLease> JsonProjectStore::lock(const std::string& path) { return std::make_unique<Lock>(qs(path)); }
StoredProject JsonProjectStore::load(const std::string& path) const {
    auto bytes = read(qs(path));
    auto project = decode(bytes);
    const bool legacy = QJsonDocument::fromJson(bytes).object().value("schemaVersion") != QJsonValue(8);
    return {std::move(project), hash(bytes).toStdString(), legacy};
}
std::string JsonProjectStore::save(const std::string& path, const Project& project, const std::string& expected) {
    validate(project);
    auto body = json::encodeProject(project);
    auto bytes = QJsonDocument(QJsonObject{{"format", "JOBS"}, {"schemaVersion", 8}, {"payload", body},
        {"sha256", QString::fromLatin1(hash(QJsonDocument(body).toJson(QJsonDocument::Compact)))}}).toJson();
    if (bytes.size() > 64 * 1024 * 1024) fail("초기 형식의 파일 한도(64 MiB)를 초과했습니다.");
    if (decode(bytes) != project) fail("저장 전 검증에 실패했습니다.");
    const auto filename = qs(path);
    const auto unchanged = [&] {
        if (expected.empty()) {
            if (QFileInfo::exists(filename)) fail("대상 파일이 이미 있습니다. 기존 정상본을 보존하려면 새 파일명을 선택하세요.");
        } else if (!QFileInfo::exists(filename) || hash(read(filename)).toStdString() != expected) {
            fail("파일이 외부에서 변경되거나 삭제되었습니다. 다른 이름으로 저장하거나 다시 여세요.");
        }
    };
    unchanged();
    QSaveFile f(filename); f.setDirectWriteFallback(false);
    if (!f.open(QIODevice::WriteOnly)) fail("저장 파일을 만들 수 없습니다: " + f.errorString());
    if (f.write(bytes) != bytes.size() || !f.flush()) fail("저장 중 쓰기에 실패했습니다: " + f.errorString());
    unchanged();
    if (!f.commit()) fail("파일 교체에 실패했습니다: " + f.errorString());
    return hash(bytes).toStdString();
}
}
