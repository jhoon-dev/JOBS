#include "persistence/JsonProjectStore.h"
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
std::string stringField(const QJsonObject& o, const QString& key) {
    if (!o.value(key).isString()) fail("문자열 필드 누락 또는 형식 오류: " + key);
    return str(o.value(key).toString());
}
QJsonObject payload(const Project& p) {
    QJsonArray tasks;
    for (const auto& t : p.tasks) {
        QJsonArray predecessors;
        for (const auto& id : t.predecessors) predecessors.append(qs(id));
        tasks.append(QJsonObject{{"id", qs(t.id)}, {"name", qs(t.name)},
            {"durationMinutes", t.durationMinutes ? QJsonValue(double(*t.durationMinutes)) : QJsonValue(QJsonValue::Null)},
            {"predecessors", predecessors}});
    }
    QJsonArray holidays;
    for (const auto& date : p.holidays) holidays.append(qs(date));
    return {{"documentId", qs(p.documentId)}, {"derivedFrom", qs(p.derivedFrom)},
            {"revision", double(p.revision)}, {"name", qs(p.name)},
            {"startDate", qs(p.startDate)}, {"calendar", qs(p.calendar)}, {"tasks", tasks},
            {"holidays", holidays}, {"calculationProfile", qs(p.calculationProfile)}};
}
Project decode(const QByteArray& bytes) {
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject()) fail("손상된 JSON 프로젝트 파일입니다.");
    const auto root = json.object();
    keys(root, {"format", "schemaVersion", "payload", "sha256"});
    if (root.value("format") != "JOBS") fail("JOBS 프로젝트 형식이 아닙니다.");
    const bool legacy = root.value("schemaVersion") == QJsonValue(1);
    if (!legacy && root.value("schemaVersion") != QJsonValue(2)) fail("지원하지 않는 파일 버전입니다. 원본은 변경하지 않았습니다.");
    if (!root.value("payload").isObject()) fail("프로젝트 본문이 없습니다.");
    auto o = root.value("payload").toObject();
    if (root.value("sha256").toString().toLatin1() != hash(QJsonDocument(o).toJson(QJsonDocument::Compact)))
        fail("파일 무결성 검사에 실패했습니다.");
    auto fields = QStringList{"documentId", "derivedFrom", "revision", "name", "startDate", "calendar", "tasks"};
    if (!legacy) fields.append({"holidays", "calculationProfile"});
    keys(o, fields);
    Project p;
    p.documentId = stringField(o, "documentId"); p.derivedFrom = stringField(o, "derivedFrom");
    p.name = stringField(o, "name"); p.startDate = stringField(o, "startDate"); p.calendar = stringField(o, "calendar");
    if (!legacy) {
        p.calculationProfile = stringField(o, "calculationProfile");
        if (!o.value("holidays").isArray()) fail("휴일 목록 형식이 잘못되었습니다.");
        for (const auto& date : o.value("holidays").toArray()) {
            if (!date.isString()) fail("휴일 날짜는 문자열이어야 합니다.");
            p.holidays.push_back(str(date.toString()));
        }
    }
    double rev = o.value("revision").toDouble(-1);
    if (!std::isfinite(rev) || rev < 1 || rev > 9007199254740991.0 || std::floor(rev) != rev) fail("잘못된 revision입니다.");
    p.revision = static_cast<std::uint64_t>(rev);
    if (!o.value("tasks").isArray()) fail("작업 목록 형식이 잘못되었습니다.");
    for (const auto& value : o.value("tasks").toArray()) {
        if (!value.isObject()) fail("작업 형식이 잘못되었습니다.");
        auto t = value.toObject();
        keys(t, legacy ? QStringList{"id", "name"} : QStringList{"id", "name", "durationMinutes", "predecessors"});
        Task task{stringField(t, "id"), stringField(t, "name"), std::nullopt, {}};
        if (!legacy) {
            if (!t.contains("durationMinutes")) fail("기간 필드가 없습니다.");
            auto duration = t.value("durationMinutes");
            if (!duration.isNull()) {
                double minutes = duration.toDouble(-1);
                if (!duration.isDouble() || !std::isfinite(minutes) || minutes < 0 || minutes > 5000000 || std::floor(minutes) != minutes)
                    fail("기간은 정수 근무분 또는 null이어야 합니다.");
                task.durationMinutes = static_cast<std::int64_t>(minutes);
            }
            if (!t.value("predecessors").isArray()) fail("선행 작업 목록 형식이 잘못되었습니다.");
            for (const auto& id : t.value("predecessors").toArray()) {
                if (!id.isString()) fail("선행 작업 ID는 문자열이어야 합니다.");
                task.predecessors.push_back(str(id.toString()));
            }
        }
        p.tasks.push_back(std::move(task));
    }
    validate(p); return p;
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
    const bool legacy = QJsonDocument::fromJson(bytes).object().value("schemaVersion") == QJsonValue(1);
    return {std::move(project), hash(bytes).toStdString(), legacy};
}
std::string JsonProjectStore::save(const std::string& path, const Project& project, const std::string& expected) {
    validate(project);
    auto body = payload(project);
    auto bytes = QJsonDocument(QJsonObject{{"format", "JOBS"}, {"schemaVersion", 2}, {"payload", body},
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
