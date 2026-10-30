#include "formatting/QtDiagnosticText.h"
#include "persistence/JsonRecoveryStore.h"
#include "persistence/ApplicationStorage.h"
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

class Busy : public std::runtime_error {
public:
    Busy() : std::runtime_error("다른 실행 중인 앱이 이 복구 세션을 사용하고 있습니다.") {}
};
bool link(const QFileInfo &f) {
    return f.isSymLink() || f.isJunction();
}
QString hash(const QByteArray &b) {
    return QString::fromLatin1(QCryptographicHash::hash(b, QCryptographicHash::Sha256).toHex());
}
bool uuid(const QString &s) {
    return !QUuid(s).isNull() && QUuid(s).toString(QUuid::WithoutBraces) == s;
}
QString name(std::uint64_t seq) {
    return QString("checkpoint-%1.jobrecovery").arg(seq, 9, 10, QChar('0'));
}
std::optional<std::uint64_t> sequence(const QString &n) {
    const auto match = QRegularExpression("^checkpoint-([0-9]{9})\\.jobrecovery$").match(n);
    if (!match.hasMatch())
        return {};
    auto value = match.captured(1).toULongLong();
    return value ? std::optional<std::uint64_t>{value} : std::nullopt;
}
void keys(const QJsonObject &o, const QStringList &allowed) {
    if (o.size() != allowed.size())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_001, {});
    for (const auto &k : o.keys())
        if (!allowed.contains(k))
            throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_002, {diagnosticArgument(k)});
}
QString string(const QJsonObject &o, const char *k) {
    if (!o[k].isString())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_003, {});
    return o[k].toString();
}
bool boolean(const QJsonObject &o, const char *k) {
    if (!o[k].isBool())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_004, {});
    return o[k].toBool();
}
std::uint64_t integer(const QJsonValue &v, std::uint64_t max) {
    auto d = v.toDouble(-1);
    if (!v.isDouble() || !std::isfinite(d) || d < 1 || d > double(max) || d != double(std::uint64_t(d)))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_005, {});
    return std::uint64_t(d);
}
QString safeDirectory(const QString &root, const QString &id, bool create) {
    if (!uuid(id))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_006, {});
    QFileInfo base(root);
    if (link(base))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_007, {});
    if (create && !QDir().mkpath(root))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_008, {});
    base.refresh();
    if (!base.isDir())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_009, {});
    const auto path = QDir(base.canonicalFilePath()).filePath(id);
    QFileInfo f(path);
    if (create && !QDir(base.canonicalFilePath()).mkdir(id))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_010, {});
    f.refresh();
    if (link(f) || !f.isDir() || f.canonicalFilePath() != path)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_011, {});
    return path;
}
QByteArray read(const QString &dir, std::uint64_t seq) {
    QFileInfo info(QDir(dir).filePath(name(seq)));
    if (link(info) || !info.isFile() || info.dir().canonicalPath() != dir)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_012, {});
    QFile f(info.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly) || f.size() > 64 * 1024 * 1024)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_013, {});
    auto b = f.read(64 * 1024 * 1024 + 1);
    if (f.error() != QFileDevice::NoError || b.size() > 64 * 1024 * 1024)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_014, {});
    return b;
}
struct Record {
    RecoverySnapshot snapshot;
    QString session;
    std::uint64_t sequence = 0;
};
Record decode(const QByteArray &bytes, const QString &expectedSession, std::uint64_t expectedSequence) {
    QJsonParseError error;
    auto json = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !json.isObject())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_015, {});
    auto root = json.object();
    keys(root, {"format", "schemaVersion", "complete", "payload", "sha256"});
    if (root["format"] != "JOBS-RECOVERY" || root["schemaVersion"] != QJsonValue(1) ||
        root["complete"] != QJsonValue(true) || !root["payload"].isObject())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_016, {});
    auto p = root["payload"].toObject();
    if (root["sha256"] != QJsonValue(hash(QJsonDocument(p).toJson(QJsonDocument::Compact))))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_017, {});
    keys(p, {"sessionId", "sequence", "capturedAt", "sourcePath", "sourceFingerprint", "sourceReadOnly",
             "sourceRequiresSaveAs", "lastSavedRevision", "projectSchema", "project"});
    Record r;
    r.session = string(p, "sessionId");
    r.sequence = integer(p["sequence"], 999999999);
    if (r.session != expectedSession || r.sequence != expectedSequence || !uuid(r.session) ||
        (p["projectSchema"] != QJsonValue(6) && p["projectSchema"] != QJsonValue(7) &&
         p["projectSchema"] != QJsonValue(8) && p["projectSchema"] != QJsonValue(9)) ||
        !p["project"].isObject())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_018, {});
    auto &s = r.snapshot;
    s.project = json::decodeProject(p["project"].toObject(), p["projectSchema"].toInt());
    s.capturedAt = string(p, "capturedAt").toStdString();
    s.sourcePath = string(p, "sourcePath").toStdString();
    s.sourceFingerprint = string(p, "sourceFingerprint").toStdString();
    s.sourceReadOnly = boolean(p, "sourceReadOnly");
    s.sourceRequiresSaveAs = boolean(p, "sourceRequiresSaveAs");
    if (!p["lastSavedRevision"].isNull())
        s.lastSavedRevision = integer(p["lastSavedRevision"], s.project.revision);
    if (!validUtcStamp(s.capturedAt) ||
        (s.sourcePath.empty() && (!s.sourceFingerprint.empty() || s.lastSavedRevision)) ||
        (!s.sourcePath.empty() && (s.sourceFingerprint.empty() || !s.lastSavedRevision)))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_019, {});
    return r;
}
QByteArray encode(const RecoverySnapshot &s, const QString &session, std::uint64_t seq) {
    validate(s.project);
    QJsonObject p{{"sessionId", session},
                  {"sequence", qint64(seq)},
                  {"capturedAt", QString::fromStdString(s.capturedAt)},
                  {"sourcePath", QString::fromStdString(s.sourcePath)},
                  {"sourceFingerprint", QString::fromStdString(s.sourceFingerprint)},
                  {"sourceReadOnly", s.sourceReadOnly},
                  {"sourceRequiresSaveAs", s.sourceRequiresSaveAs},
                  {"lastSavedRevision", s.lastSavedRevision ? QJsonValue(qint64(*s.lastSavedRevision)) : QJsonValue()},
                  {"projectSchema", 9},
                  {"project", json::encodeProject(s.project)}};
    auto bytes = QJsonDocument(QJsonObject{{"format", "JOBS-RECOVERY"},
                                           {"schemaVersion", 1},
                                           {"complete", true},
                                           {"payload", p},
                                           {"sha256", hash(QJsonDocument(p).toJson(QJsonDocument::Compact))}})
                     .toJson();
    if (bytes.size() > 64 * 1024 * 1024)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_020, {});
    auto decoded = decode(bytes, session, seq);
    if (decoded.snapshot.project != s.project)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_021, {});
    return bytes;
}
struct Original {
    QString state, differences;
};
Original originalState(const RecoverySnapshot &s) {
    if (s.sourcePath.empty())
        return {"처음 저장 전", "비교할 명시 저장본 없음 · 전체 메모리 입력을 복구합니다."};
    const auto source = QString::fromStdString(s.sourcePath);
    if (source.startsWith("//") || source.startsWith("\\\\"))
        return {"네트워크/특수 경로 확인 생략", "원본 연결 없이 복구 스냅샷을 독립적으로 열 수 있습니다."};
    QFileInfo info(source);
    if (!info.exists())
        return {"원본 없음/이동됨", "원본과의 변경 비교 불가 · 저장된 복구 스냅샷은 독립적으로 열 수 있습니다."};
    if (!info.isFile() || link(info))
        return {"원본 상태 확인 불가",
                "일반 파일인 원본만 자동으로 비교합니다. 복구 스냅샷은 독립적으로 열 수 있습니다."};
    QFile f(info.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly) || f.size() > 64 * 1024 * 1024)
        return {"원본 상태 확인 불가", "원본과의 변경 비교를 확인할 수 없습니다."};
    auto bytes = f.read(64 * 1024 * 1024 + 1);
    if (f.error() != QFileDevice::NoError || bytes.size() > 64 * 1024 * 1024)
        return {"원본 상태 확인 불가", "원본을 끝까지 확인할 수 없습니다."};
    if (hash(bytes) != QString::fromStdString(s.sourceFingerprint))
        return {"원본 변경 · 자동 병합 없음",
                "원본이 기록된 저장 지점과 달라 변경 비교를 생략합니다. 복구본과 현재 원본을 각각 보존하세요."};
    try {
        JsonProjectStore store;
        auto loaded = store.load(s.sourcePath);
        if (loaded.fingerprint != s.sourceFingerprint)
            return {"원본 비교 중 변경됨", "일관된 비교 불가 · 다시 검색하세요."};
        const auto &before = loaded.project;
        const auto &after = s.project;
        if (before.documentId != after.documentId || before.revision != s.lastSavedRevision)
            return {"원본 식별자/저장 지점 불일치", "변경 비교를 신뢰할 수 없습니다. 복구본 입력을 별도 확인하세요."};
        std::map<std::string, const Task *> oldTasks;
        for (const auto &t : before.tasks)
            oldTasks[t.id] = &t;
        int added = 0, changed = 0;
        for (const auto &t : after.tasks) {
            auto it = oldTasks.find(t.id);
            if (it == oldTasks.end())
                ++added;
            else {
                if (*it->second != t)
                    ++changed;
                oldTasks.erase(it);
            }
        }
        QStringList changes{QString("작업 추가 %1 / 제거 %2 / 변경 %3").arg(added).arg(oldTasks.size()).arg(changed)};
        if (before.wbsNodes != after.wbsNodes)
            changes << "WBS 변경";
        if (before.resources != after.resources)
            changes << "자원·가용량·단가 변경";
        if (before.assignments != after.assignments)
            changes << "배정 변경";
        if (before.holidays != after.holidays || before.startDate != after.startDate ||
            before.calendars != after.calendars)
            changes << "달력/시작일 변경";
        if (before.currency != after.currency || before.taskCosts != after.taskCosts)
            changes << "원가 설정 변경";
        if (before.statusAt != after.statusAt)
            changes << "상태일 " + QString::fromStdString(before.statusAt.value_or("없음")) + " → " +
                           QString::fromStdString(after.statusAt.value_or("없음"));
        if (before.baselines != after.baselines)
            changes << QString("기준선 %1 → %2").arg(before.baselines.size()).arg(after.baselines.size());
        if (before.progress != after.progress)
            changes << QString("실적 이력 %1 → %2").arg(before.progress.size()).arg(after.progress.size());
        if (before.costEntries != after.costEntries)
            changes << QString("원가 이력 %1 → %2").arg(before.costEntries.size()).arg(after.costEntries.size());
        if (before.scenarios != after.scenarios)
            changes << "저장 시나리오 변경";
        return {"마지막 명시 저장과 일치", "명시 저장 대비: " + changes.join(" · ")};
    } catch (const std::exception &) {
        return {"원본 내용 비교 불가", "복구본은 검증했으나 원본 입력의 변경 비교를 확인할 수 없습니다."};
    }
}
} // namespace
struct RecoverySession::Impl {
    QString root, id, directory;
    std::unique_ptr<QLockFile> lock;
    std::mutex mutex;
    std::atomic<bool> retired{false};
    std::uint64_t sequence = 0;
    bool writable = false;
    std::function<void()> beforePublish;
};
RecoverySession::RecoverySession(QString root, QString id, bool create, std::function<void()> beforePublish)
    : impl_(std::make_unique<Impl>()) {
    impl_->root = std::move(root);
    impl_->id = std::move(id);
    impl_->writable = create;
    impl_->beforePublish = std::move(beforePublish);
    impl_->directory = safeDirectory(impl_->root, impl_->id, create);
    const auto lockPath = QDir(impl_->directory).filePath("session.lock");
    if (link(QFileInfo(lockPath)))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_022, {});
    impl_->lock = std::make_unique<QLockFile>(lockPath);
    impl_->lock->setStaleLockTime(0);
    if (!impl_->lock->tryLock(0)) {
        if (impl_->lock->error() == QLockFile::LockFailedError) {
            qint64 pid = 0;
            QString host, app;
            if (impl_->lock->getLockInfo(&pid, &host, &app))
                throw Busy();
            throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_023, {});
        }
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_024, {});
    }
}
RecoverySession::~RecoverySession() {
    if (retired()) {
        try {
            cleanup();
        } catch (...) {
        }
    }
    const auto dir = impl_->directory;
    impl_->lock.reset();
    QDir().rmdir(dir);
}
QString RecoverySession::id() const {
    return impl_->id;
}
bool RecoverySession::retired() const noexcept {
    return impl_->retired.load();
}
void RecoverySession::retire() noexcept {
    impl_->retired.store(true);
}
void RecoverySession::cleanup() {
    std::lock_guard guard(impl_->mutex);
    if (!retired())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_025, {});
    const auto dir = safeDirectory(impl_->root, impl_->id, false);
    for (const auto &f : QDir(dir).entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot))
        if (sequence(f.fileName()) && !link(f)) {
            if (!QFile::remove(f.absoluteFilePath()))
                throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_026, {});
        }
}
std::uint64_t RecoverySession::checkpoint(const RecoverySnapshot &snapshot) {
    std::lock_guard guard(impl_->mutex);
    if (retired() || !impl_->writable)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_027, {});
    const auto dir = safeDirectory(impl_->root, impl_->id, false);
    const auto seq = ++impl_->sequence;
    if (seq > 999999999)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_028, {});
    auto bytes = encode(snapshot, impl_->id, seq);
    const auto path = QDir(dir).filePath(name(seq));
    if (QFileInfo::exists(path))
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_029, {});
    QSaveFile file(path);
    file.setDirectWriteFallback(false);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.flush())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_030, {diagnosticArgument(file.errorString())});
    if (impl_->beforePublish)
        impl_->beforePublish();
    if (retired())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_031, {});
    if (QFileInfo::exists(path) || !file.commit())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_032, {});
    // Retain three completed valid generations. Never prune a corrupt generation.
    std::vector<std::pair<std::uint64_t, QString>> valid;
    for (const auto &f : QDir(dir).entryInfoList(QDir::Files))
        if (auto n = sequence(f.fileName()); n && !link(f)) {
            try {
                decode(read(dir, *n), impl_->id, *n);
                valid.push_back({*n, f.absoluteFilePath()});
            } catch (...) {
            }
        }
    std::sort(valid.begin(), valid.end());
    while (valid.size() > 3) {
        if (!QFile::remove(valid.front().second))
            throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_033, {});
        valid.erase(valid.begin());
    }
    return seq;
}
JsonRecoveryStore::JsonRecoveryStore(QString root, std::function<void()> hook)
    : root_(QDir::cleanPath(QFileInfo(root).absoluteFilePath())), beforePublish_(std::move(hook)) {
    if (root.isEmpty())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_034, {});
}
QString JsonRecoveryStore::defaultDirectory() {
    return applicationStorage().recoveryDirectory;
}
std::shared_ptr<RecoverySession> JsonRecoveryStore::newSession() const {
    return std::shared_ptr<RecoverySession>(
        new RecoverySession(root_, QUuid::createUuid().toString(QUuid::WithoutBraces), true, beforePublish_));
}
RecoveryScan JsonRecoveryStore::scan() const {
    RecoveryScan result;
    QFileInfo root(root_);
    if (!root.exists())
        return result;
    if (link(root) || !root.isDir())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_035, {});
    auto dirs = QDir(root_).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot | QDir::System);
    if (dirs.size() > 1000)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_036, {});
    for (const auto &dir : dirs) {
        if (!uuid(dir.fileName()))
            continue;
        try {
            auto session = std::shared_ptr<RecoverySession>(new RecoverySession(root_, dir.fileName(), false));
            auto files =
                QDir(session->impl_->directory).entryInfoList(QDir::Files | QDir::System | QDir::NoDotAndDotDot);
            if (files.size() > 1000)
                throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_037, {});
            for (const auto &file : files)
                if (auto seq = sequence(file.fileName())) {
                    RecoveryCandidate c;
                    c.sessionId = dir.fileName();
                    c.sequence = *seq;
                    try {
                        auto bytes = read(session->impl_->directory, *seq);
                        auto record = decode(bytes, c.sessionId, *seq);
                        const auto &s = record.snapshot;
                        c.valid = true;
                        c.fileHash = hash(bytes);
                        c.projectName = s.project.name;
                        c.documentId = s.project.documentId;
                        c.revision = s.project.revision;
                        c.capturedAt = s.capturedAt;
                        c.sourcePath = s.sourcePath;
                        c.lastSavedRevision = s.lastSavedRevision;
                        c.tasks = s.project.tasks.size();
                        c.baselines = s.project.baselines.size();
                        c.progress = s.project.progress.size();
                        c.costs = s.project.costEntries.size();
                        auto original = originalState(s);
                        c.sourceState = std::move(original.state);
                        c.differences = std::move(original.differences);
                    } catch (const std::exception &e) {
                        c.failure = diagnosticFromException(e);
                        if (c.failure->path.empty())
                            c.failure->path = file.absoluteFilePath().toStdString();
                        c.error = qDiagnosticText(*c.failure);
                    }
                    result.candidates.push_back(std::move(c));
                }
        } catch (const Busy &) {
            ++result.activeSessions;
        } catch (const std::exception &e) {
            RecoveryCandidate c;
            c.sessionId = dir.fileName();
            c.failure = diagnosticFromException(e);
            if (c.failure->path.empty())
                c.failure->path = dir.absoluteFilePath().toStdString();
            c.error = qDiagnosticText(*c.failure);
            result.candidates.push_back(std::move(c));
        }
    }
    std::sort(result.candidates.begin(), result.candidates.end(), [](const auto &a, const auto &b) {
        if (a.capturedAt != b.capturedAt)
            return a.capturedAt > b.capturedAt;
        if (a.sessionId != b.sessionId)
            return a.sessionId < b.sessionId;
        return a.sequence > b.sequence;
    });
    return result;
}
std::unique_ptr<RecoveryClaim> JsonRecoveryStore::claim(const RecoveryCandidate &candidate) const {
    auto session = std::shared_ptr<RecoverySession>(new RecoverySession(root_, candidate.sessionId, false));
    if (!candidate.valid || !candidate.sequence || candidate.fileHash.isEmpty())
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_038, {});
    auto bytes = read(session->impl_->directory, candidate.sequence);
    if (hash(bytes) != candidate.fileHash)
        throw AppError(ErrorCode::FILE_JSONRECOVERYSTORE_039, {});
    auto record = decode(bytes, candidate.sessionId, candidate.sequence);
    auto claim = std::make_unique<RecoveryClaim>();
    claim->session = std::move(session);
    claim->snapshot = std::move(record.snapshot);
    return claim;
}
void JsonRecoveryStore::discardSession(const QString &id) const {
    auto session = std::shared_ptr<RecoverySession>(new RecoverySession(root_, id, false));
    session->retire();
    session->cleanup();
}
} // namespace jobs
