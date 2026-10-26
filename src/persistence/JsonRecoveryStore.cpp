#include "persistence/JsonRecoveryStore.h"
#include "persistence/ProjectJson.h"
#include "persistence/JsonProjectStore.h"
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QRegularExpression>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUuid>
#include <atomic>
#include <mutex>
#include <map>
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jobs {
namespace {
[[noreturn]] void fail(const QString& s) { throw std::runtime_error(s.toUtf8().toStdString()); }
class Busy : public std::runtime_error { public: Busy() : std::runtime_error("다른 실행 중인 앱이 이 복구 세션을 사용하고 있습니다.") {} };
bool link(const QFileInfo& f) { return f.isSymLink() || f.isJunction(); }
QString hash(const QByteArray& b) { return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex()); }
bool uuid(const QString& s) { return !QUuid(s).isNull() && QUuid(s).toString(QUuid::WithoutBraces) == s; }
QString name(std::uint64_t seq) { return QString("checkpoint-%1.jobrecovery").arg(seq, 9, 10, QChar('0')); }
std::optional<std::uint64_t> sequence(const QString& n) {
    const auto match = QRegularExpression("^checkpoint-([0-9]{9})\\.jobrecovery$").match(n);
    if (!match.hasMatch()) return {};
    auto value = match.captured(1).toULongLong(); return value ? std::optional<std::uint64_t>{value} : std::nullopt;
}
void keys(const QJsonObject& o, const QStringList& allowed) { if (o.size() != allowed.size()) fail("복구 메타데이터 필드가 누락되었습니다."); for (const auto& k : o.keys()) if (!allowed.contains(k)) fail("지원하지 않는 복구 필드: " + k); }
QString string(const QJsonObject& o, const char* k) { if (!o[k].isString()) fail("복구 문자열 필드 오류"); return o[k].toString(); }
bool boolean(const QJsonObject& o, const char* k) { if (!o[k].isBool()) fail("복구 논리 필드 오류"); return o[k].toBool(); }
std::uint64_t integer(const QJsonValue& v, std::uint64_t max) { auto d = v.toDouble(-1); if (!v.isDouble() || !std::isfinite(d) || d < 1 || d > double(max) || d != double(std::uint64_t(d))) fail("복구 정수 필드 오류"); return std::uint64_t(d); }
QString safeDirectory(const QString& root, const QString& id, bool create) {
    if (!uuid(id)) fail("잘못된 복구 세션 ID입니다.");
    QFileInfo base(root); if (link(base)) fail("복구 폴더에 링크/정션을 사용할 수 없습니다.");
    if (create && !QDir().mkpath(root)) fail("복구 폴더를 만들 수 없습니다.");
    base.refresh(); if (!base.isDir()) fail("복구 폴더가 없습니다.");
    const auto path = QDir(base.canonicalFilePath()).filePath(id); QFileInfo f(path);
    if (create && !QDir(base.canonicalFilePath()).mkdir(id)) fail("복구 세션 폴더를 만들 수 없습니다.");
    f.refresh(); if (link(f) || !f.isDir() || f.canonicalFilePath() != path) fail("안전하지 않은 복구 세션 경로입니다.");
    return path;
}
QByteArray read(const QString& dir, std::uint64_t seq) {
    QFileInfo info(QDir(dir).filePath(name(seq)));
    if (link(info) || !info.isFile() || info.dir().canonicalPath() != dir) fail("복구 파일 경로가 잘못되었습니다.");
    QFile f(info.absoluteFilePath()); if (!f.open(QIODevice::ReadOnly) || f.size() > 64 * 1024 * 1024) fail("복구 파일 읽기 실패/64 MiB 한도 초과입니다.");
    auto b = f.read(64 * 1024 * 1024 + 1); if (f.error() != QFileDevice::NoError || b.size() > 64 * 1024 * 1024) fail("복구 파일을 끝까지 읽지 못했거나 64 MiB 한도를 초과했습니다."); return b;
}
struct Record { RecoverySnapshot snapshot; QString session; std::uint64_t sequence = 0; };
Record decode(const QByteArray& bytes, const QString& expectedSession, std::uint64_t expectedSequence) {
    QJsonParseError error; auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject()) fail("손상되었거나 미완료인 복구 JSON입니다.");
    auto root = json.object(); keys(root, {"format", "schemaVersion", "complete", "payload", "sha256"});
    if (root["format"] != "JOBS-RECOVERY" || root["schemaVersion"] != QJsonValue(1) || root["complete"] != QJsonValue(true) || !root["payload"].isObject()) fail("지원하지 않는 버전/미완료 복구본입니다.");
    auto p = root["payload"].toObject();
    if (root["sha256"] != QJsonValue(hash(QJsonDocument(p).toJson(QJsonDocument::Compact)))) fail("복구본 무결성 검사에 실패했습니다.");
    keys(p, {"sessionId", "sequence", "capturedAt", "sourcePath", "sourceFingerprint", "sourceReadOnly", "sourceRequiresSaveAs", "lastSavedRevision", "projectSchema", "project"});
    Record r; r.session = string(p, "sessionId"); r.sequence = integer(p["sequence"], 999999999);
    if (r.session != expectedSession || r.sequence != expectedSequence || !uuid(r.session) || (p["projectSchema"] != QJsonValue(6) && p["projectSchema"] != QJsonValue(7) && p["projectSchema"] != QJsonValue(8) && p["projectSchema"] != QJsonValue(9)) || !p["project"].isObject()) fail("복구 세션·세대·프로젝트 버전이 일치하지 않습니다.");
    auto& s = r.snapshot; s.project = json::decodeProject(p["project"].toObject(), p["projectSchema"].toInt()); s.capturedAt = string(p, "capturedAt").toStdString();
    s.sourcePath = string(p, "sourcePath").toStdString(); s.sourceFingerprint = string(p, "sourceFingerprint").toStdString(); s.sourceReadOnly = boolean(p, "sourceReadOnly"); s.sourceRequiresSaveAs = boolean(p, "sourceRequiresSaveAs");
    if (!p["lastSavedRevision"].isNull()) s.lastSavedRevision = integer(p["lastSavedRevision"], s.project.revision);
    if (!validUtcStamp(s.capturedAt) || (s.sourcePath.empty() && (!s.sourceFingerprint.empty() || s.lastSavedRevision)) || (!s.sourcePath.empty() && (s.sourceFingerprint.empty() || !s.lastSavedRevision))) fail("복구 시각/원본 저장 지점이 잘못되었습니다.");
    return r;
}
QByteArray encode(const RecoverySnapshot& s, const QString& session, std::uint64_t seq) {
    validate(s.project);
    QJsonObject p{{"sessionId", session}, {"sequence", qint64(seq)}, {"capturedAt", QString::fromStdString(s.capturedAt)}, {"sourcePath", QString::fromStdString(s.sourcePath)}, {"sourceFingerprint", QString::fromStdString(s.sourceFingerprint)}, {"sourceReadOnly", s.sourceReadOnly}, {"sourceRequiresSaveAs", s.sourceRequiresSaveAs}, {"lastSavedRevision", s.lastSavedRevision ? QJsonValue(qint64(*s.lastSavedRevision)) : QJsonValue()}, {"projectSchema", 9}, {"project", json::encodeProject(s.project)}};
    auto bytes = QJsonDocument(QJsonObject{{"format", "JOBS-RECOVERY"}, {"schemaVersion", 1}, {"complete", true}, {"payload", p}, {"sha256", hash(QJsonDocument(p).toJson(QJsonDocument::Compact))}}).toJson();
    if (bytes.size() > 64 * 1024 * 1024) fail("자동 복구본 64 MiB 한도를 초과했습니다.");
    auto decoded = decode(bytes, session, seq);
    if (decoded.snapshot.project != s.project) fail("자동 복구본 저장 전 검증에 실패했습니다.");
    return bytes;
}
struct Original { QString state, differences; };
Original originalState(const RecoverySnapshot& s) {
    if (s.sourcePath.empty()) return {"처음 저장 전", "비교할 명시 저장본 없음 · 전체 메모리 입력을 복구합니다."};
    const auto source = QString::fromStdString(s.sourcePath);
    if (source.startsWith("//") || source.startsWith("\\\\")) return {"네트워크/특수 경로 확인 생략", "원본 연결 없이 복구 스냅샷을 독립적으로 열 수 있습니다."};
    QFileInfo info(source); if (!info.exists()) return {"원본 없음/이동됨", "원본과의 변경 비교 불가 · 저장된 복구 스냅샷은 독립적으로 열 수 있습니다."};
    if (!info.isFile() || link(info)) return {"원본 상태 확인 불가", "일반 파일인 원본만 자동으로 비교합니다. 복구 스냅샷은 독립적으로 열 수 있습니다."};
    QFile f(info.absoluteFilePath()); if (!f.open(QIODevice::ReadOnly) || f.size() > 64 * 1024 * 1024) return {"원본 상태 확인 불가", "원본과의 변경 비교를 확인할 수 없습니다."};
    auto bytes = f.read(64 * 1024 * 1024 + 1); if (f.error() != QFileDevice::NoError || bytes.size() > 64 * 1024 * 1024) return {"원본 상태 확인 불가", "원본을 끝까지 확인할 수 없습니다."};
    if (hash(bytes) != QString::fromStdString(s.sourceFingerprint)) return {"원본 변경 · 자동 병합 없음", "원본이 기록된 저장 지점과 달라 변경 비교를 생략합니다. 복구본과 현재 원본을 각각 보존하세요."};
    try {
        JsonProjectStore store; auto loaded = store.load(s.sourcePath);
        if (loaded.fingerprint != s.sourceFingerprint) return {"원본 비교 중 변경됨", "일관된 비교 불가 · 다시 검색하세요."};
        const auto& before = loaded.project; const auto& after = s.project;
        if (before.documentId != after.documentId || before.revision != s.lastSavedRevision) return {"원본 식별자/저장 지점 불일치", "변경 비교를 신뢰할 수 없습니다. 복구본 입력을 별도 확인하세요."};
        std::map<std::string, const Task*> oldTasks; for (const auto& t : before.tasks) oldTasks[t.id] = &t;
        int added = 0, changed = 0;
        for (const auto& t : after.tasks) { auto it = oldTasks.find(t.id); if (it == oldTasks.end()) ++added; else { if (*it->second != t) ++changed; oldTasks.erase(it); } }
        QStringList changes{QString("작업 추가 %1 / 제거 %2 / 변경 %3").arg(added).arg(oldTasks.size()).arg(changed)};
        if (before.wbsNodes != after.wbsNodes) changes << "WBS 변경";
        if (before.resources != after.resources) changes << "자원·가용량·단가 변경";
        if (before.assignments != after.assignments) changes << "배정 변경";
        if (before.holidays != after.holidays || before.startDate != after.startDate || before.calendars != after.calendars) changes << "달력/시작일 변경";
        if (before.currency != after.currency || before.taskCosts != after.taskCosts) changes << "원가 설정 변경";
        if (before.statusAt != after.statusAt) changes << "상태일 " + QString::fromStdString(before.statusAt.value_or("없음")) + " → " + QString::fromStdString(after.statusAt.value_or("없음"));
        if (before.baselines != after.baselines) changes << QString("기준선 %1 → %2").arg(before.baselines.size()).arg(after.baselines.size());
        if (before.progress != after.progress) changes << QString("실적 이력 %1 → %2").arg(before.progress.size()).arg(after.progress.size());
        if (before.costEntries != after.costEntries) changes << QString("원가 이력 %1 → %2").arg(before.costEntries.size()).arg(after.costEntries.size());
        if (before.scenarios != after.scenarios) changes << "저장 시나리오 변경";
        return {"마지막 명시 저장과 일치", "명시 저장 대비: " + changes.join(" · ")};
    } catch (const std::exception&) { return {"원본 내용 비교 불가", "복구본은 검증했으나 원본 입력의 변경 비교를 확인할 수 없습니다."}; }
}
}
struct RecoverySession::Impl {
    QString root, id, directory;
    std::unique_ptr<QLockFile> lock;
    std::mutex mutex;
    std::atomic<bool> retired{false};
    std::uint64_t sequence = 0;
    bool writable = false;
    std::function<void()> beforePublish;
};
RecoverySession::RecoverySession(QString root, QString id, bool create, std::function<void()> beforePublish) : impl_(std::make_unique<Impl>()) {
    impl_->root = std::move(root); impl_->id = std::move(id); impl_->writable = create;
    impl_->beforePublish = std::move(beforePublish);
    impl_->directory = safeDirectory(impl_->root, impl_->id, create);
    const auto lockPath = QDir(impl_->directory).filePath("session.lock"); if (link(QFileInfo(lockPath))) fail("복구 잠금에 링크를 사용할 수 없습니다.");
    impl_->lock = std::make_unique<QLockFile>(lockPath); impl_->lock->setStaleLockTime(0);
    if (!impl_->lock->tryLock(0)) {
        if (impl_->lock->error() == QLockFile::LockFailedError) {
            qint64 pid = 0; QString host, app;
            if (impl_->lock->getLockInfo(&pid, &host, &app)) throw Busy();
            fail("복구 잠금 소유자를 확인할 수 없습니다. 복구본을 보존했습니다. session.lock과 실행 중인 앱을 확인하세요.");
        }
        fail("복구 세션의 잠금을 얻을 수 없습니다.");
    }
}
RecoverySession::~RecoverySession() {
    if (retired()) { try { cleanup(); } catch (...) {} }
    const auto dir = impl_->directory; impl_->lock.reset(); QDir().rmdir(dir);
}
QString RecoverySession::id() const { return impl_->id; }
bool RecoverySession::retired() const noexcept { return impl_->retired.load(); }
void RecoverySession::retire() noexcept { impl_->retired.store(true); }
void RecoverySession::cleanup() {
    std::lock_guard guard(impl_->mutex); if (!retired()) fail("진행 중인 복구 세션은 제거할 수 없습니다.");
    const auto dir = safeDirectory(impl_->root, impl_->id, false);
    for (const auto& f : QDir(dir).entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot)) if (sequence(f.fileName()) && !link(f)) {
        if (!QFile::remove(f.absoluteFilePath())) fail("복구본 정리를 완료하지 못했습니다.");
    }
}
std::uint64_t RecoverySession::checkpoint(const RecoverySnapshot& snapshot) {
    std::lock_guard guard(impl_->mutex);
    if (retired() || !impl_->writable) fail("종료되었거나 읽기 전용인 복구 세션입니다.");
    const auto dir = safeDirectory(impl_->root, impl_->id, false); const auto seq = ++impl_->sequence;
    if (seq > 999999999) fail("자동 복구 세대 한도 초과입니다.");
    auto bytes = encode(snapshot, impl_->id, seq); const auto path = QDir(dir).filePath(name(seq));
    if (QFileInfo::exists(path)) fail("동일 복구 세대가 이미 존재합니다.");
    QSaveFile file(path); file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.flush()) fail("자동 복구본을 쓸 수 없습니다: " + file.errorString());
    if (impl_->beforePublish) impl_->beforePublish();
    if (retired()) fail("문서가 전환되어 자동 복구 저장을 취소했습니다.");
    if (QFileInfo::exists(path) || !file.commit()) fail("자동 복구본을 완성하지 못했습니다.");
    // Retain three completed valid generations. Never prune a corrupt generation.
    std::vector<std::pair<std::uint64_t, QString>> valid;
    for (const auto& f : QDir(dir).entryInfoList(QDir::Files)) if (auto n = sequence(f.fileName()); n && !link(f)) {
        try { decode(read(dir, *n), impl_->id, *n); valid.push_back({*n, f.absoluteFilePath()}); } catch (...) {}
    }
    std::sort(valid.begin(), valid.end());
    while (valid.size() > 3) { if (!QFile::remove(valid.front().second)) fail("새 복구본은 완성했으나 오래된 세대 정리에 실패했습니다."); valid.erase(valid.begin()); }
    return seq;
}
JsonRecoveryStore::JsonRecoveryStore(QString root, std::function<void()> hook) : root_(QDir::cleanPath(QFileInfo(root).absoluteFilePath())), beforePublish_(std::move(hook)) { if (root.isEmpty()) fail("복구 폴더 경로가 없습니다."); }
QString JsonRecoveryStore::defaultDirectory() { return QDir(QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)).filePath("recovery"); }
std::shared_ptr<RecoverySession> JsonRecoveryStore::newSession() const { return std::shared_ptr<RecoverySession>(new RecoverySession(root_, QUuid::createUuid().toString(QUuid::WithoutBraces), true, beforePublish_)); }
RecoveryScan JsonRecoveryStore::scan() const {
    RecoveryScan result; QFileInfo root(root_); if (!root.exists()) return result;
    if (link(root) || !root.isDir()) fail("복구 폴더에 접근할 수 없습니다.");
    auto dirs = QDir(root_).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System); if (dirs.size() > 1000) fail("복구 세션 검색 한도(1,000개)를 초과했습니다. 복구 폴더를 확인하세요.");
    for (const auto& dir : dirs) {
        if (!uuid(dir.fileName())) continue;
        try {
            auto session = std::shared_ptr<RecoverySession>(new RecoverySession(root_, dir.fileName(), false));
            auto files = QDir(session->impl_->directory).entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot);
            if (files.size() > 1000) fail("복구 세대 검색 한도 초과입니다.");
            for (const auto& file : files) if (auto seq = sequence(file.fileName())) {
                RecoveryCandidate c; c.sessionId = dir.fileName(); c.sequence = *seq;
                try {
                    auto bytes = read(session->impl_->directory, *seq); auto record = decode(bytes, c.sessionId, *seq); const auto& s = record.snapshot;
                    c.valid = true; c.fileHash = hash(bytes); c.projectName = s.project.name; c.documentId = s.project.documentId; c.revision = s.project.revision; c.capturedAt = s.capturedAt; c.sourcePath = s.sourcePath; c.lastSavedRevision = s.lastSavedRevision;
                    c.tasks = s.project.tasks.size(); c.baselines = s.project.baselines.size(); c.progress = s.project.progress.size(); c.costs = s.project.costEntries.size(); auto original = originalState(s); c.sourceState = std::move(original.state); c.differences = std::move(original.differences);
                } catch (const std::exception& e) { c.error = QString::fromUtf8(e.what()); }
                result.candidates.push_back(std::move(c));
            }
        } catch (const Busy&) { ++result.activeSessions; }
        catch (const std::exception& e) { RecoveryCandidate c; c.sessionId = dir.fileName(); c.error = QString::fromUtf8(e.what()); result.candidates.push_back(std::move(c)); }
    }
    std::sort(result.candidates.begin(), result.candidates.end(), [](const auto& a, const auto& b) { if (a.capturedAt != b.capturedAt) return a.capturedAt > b.capturedAt; if (a.sessionId != b.sessionId) return a.sessionId < b.sessionId; return a.sequence > b.sequence; });
    return result;
}
std::unique_ptr<RecoveryClaim> JsonRecoveryStore::claim(const RecoveryCandidate& candidate) const {
    auto session = std::shared_ptr<RecoverySession>(new RecoverySession(root_, candidate.sessionId, false));
    if (!candidate.valid || !candidate.sequence || candidate.fileHash.isEmpty()) fail("검증된 복구 세대를 선택하세요.");
    auto bytes = read(session->impl_->directory, candidate.sequence); if (hash(bytes) != candidate.fileHash) fail("선택 후 복구본이 변경되었습니다. 다시 검색하세요.");
    auto record = decode(bytes, candidate.sessionId, candidate.sequence); auto claim = std::make_unique<RecoveryClaim>(); claim->session = std::move(session); claim->snapshot = std::move(record.snapshot); return claim;
}
void JsonRecoveryStore::discardSession(const QString& id) const { auto session = std::shared_ptr<RecoverySession>(new RecoverySession(root_, id, false)); session->retire(); session->cleanup(); }
}
