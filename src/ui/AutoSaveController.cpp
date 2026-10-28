#include "formatting/QtDiagnosticText.h"
#include "ui/AutoSaveController.h"
#include <QDateTime>
#include <QFutureWatcher>
#include <QtConcurrent>
#include <algorithm>
namespace jobs {
namespace {
struct Result {
    QString error;
    std::uint64_t sequence = 0;
};
} // namespace
AutoSaveController::AutoSaveController(Document &d, JsonRecoveryStore store, QObject *parent, int intervalMs,
                                       int idleMs)
    : QObject(parent), document_(d), store_(std::move(store)), intervalMs_(std::max(1, intervalMs)),
      idleMs_(std::max(1, idleMs)) {
    setObjectName("autoSaveController");
    idle_.setSingleShot(true);
    deadline_.setSingleShot(true);
    connect(&idle_, &QTimer::timeout, this, &AutoSaveController::checkpointNow);
    connect(&deadline_, &QTimer::timeout, this, &AutoSaveController::checkpointNow);
    listener_ = document_.subscribe([this](auto event) { changed(event); });
    changed(Document::Event::Edited);
}
AutoSaveController::~AutoSaveController() {
    document_.unsubscribe(listener_);
    idle_.stop();
    deadline_.stop();
    statusChanged = {};
}
void AutoSaveController::setStatus(QString s) {
    status_ = std::move(s);
    if (statusChanged)
        statusChanged();
}
void AutoSaveController::retire(std::shared_ptr<RecoverySession> session) {
    if (!session)
        return;
    session->retire();
    auto *watcher = new QFutureWatcher<QString>(this);
    connect(watcher, &QFutureWatcher<QString>::finished, this, [this, watcher] {
        const auto error = watcher->result();
        watcher->deleteLater();
        if (!error.isEmpty()) {
            cleanupError_ = error;
            setStatus(status_);
        }
    });
    watcher->setFuture(QtConcurrent::run([session = std::move(session)] {
        try {
            session->cleanup();
            return QString{};
        } catch (const std::exception &e) {
            return qErrorText(e);
        }
    }));
}
void AutoSaveController::changed(Document::Event event) {
    ++serial_;
    if (event == Document::Event::Saved || event == Document::Event::Replaced || !document_.dirty() ||
        document_.readOnly()) {
        idle_.stop();
        deadline_.stop();
        retire(std::move(session_));
        if (event == Document::Event::Saved)
            retire(std::move(recoveredSession_));
        else if (event == Document::Event::Replaced)
            recoveredSession_.reset();
        storedSerial_ = 0;
    }
    if (finished_)
        return;
    if (!enabled_) {
        setStatus("자동복구 저장 중지 · 기존 복구본 보존");
        return;
    }
    if (!document_.project()) {
        setStatus("자동복구 저장 대기 · 문서 없음");
        return;
    }
    if (document_.readOnly()) {
        setStatus("읽기 전용 · 자동복구 저장 안 함");
        return;
    }
    if (!document_.dirty()) {
        setStatus("명시 저장본과 같은 입력 · 복구본 정리");
        return;
    }
    setStatus(busy_ ? "자동복구 저장 중 · 새 변경 대기" : "자동복구 저장 대기 · 원본 파일 보존");
    schedule();
}
void AutoSaveController::schedule() {
    if (!enabled_ || finished_ || !document_.project() || !document_.dirty() || document_.readOnly())
        return;
    idle_.start(idleMs_);
    if (!deadline_.isActive())
        deadline_.start(intervalMs_);
}
void AutoSaveController::setEnabled(bool enabled) {
    enabled_ = enabled;
    idle_.stop();
    deadline_.stop();
    if (!enabled_)
        setStatus("자동복구 저장 중지 · 기존 복구본 보존");
    else
        changed(Document::Event::Edited);
}
void AutoSaveController::setIntervalMs(int interval) {
    intervalMs_ = std::max(1, interval);
    deadline_.stop();
    schedule();
}
void AutoSaveController::checkpointNow() {
    if (!enabled_ || finished_ || busy_ || !document_.project() || !document_.dirty() || document_.readOnly() ||
        storedSerial_ == serial_)
        return;
    idle_.stop();
    deadline_.stop();
    try {
        if (!session_)
            session_ = store_.newSession();
        auto request = document_.recoverySnapshotRequest(
            QDateTime::currentDateTimeUtc().toString("yyyy-MM-dd'T'HH:mm:ss'Z'").toStdString());
        const auto revision = document_.project()->revision, serial = serial_;
        auto session = session_;
        busy_ = true;
        setStatus("자동복구 저장 중 · revision " + QString::number(revision));
        auto *watcher = new QFutureWatcher<Result>(this);
        connect(watcher, &QFutureWatcher<Result>::finished, this, [this, watcher, session, serial, revision] {
            auto result = watcher->result();
            watcher->deleteLater();
            busy_ = false;
            if (session_ == session && !session->retired()) {
                if (!result.error.isEmpty())
                    setStatus("자동복구 저장 실패 · " + result.error);
                else {
                    storedSerial_ = serial;
                    setStatus((enabled_ ? "자동복구 저장됨" : "자동복구 중지 · 마지막 복구본") +
                              QString(" · r%1 / 세대 %2").arg(revision).arg(result.sequence) +
                              (document_.project() && document_.project()->revision != revision
                                   ? " · 현재 r" + QString::number(document_.project()->revision) + " 새 변경 대기"
                                   : " · 명시 저장과 별도"));
                }
            }
            if (!result.error.isEmpty() && session_ == session)
                deadline_.start(intervalMs_);
            else if (serial_ != storedSerial_)
                schedule();
        });
        watcher->setFuture(QtConcurrent::run([session, request = std::move(request)]() mutable {
            Result out;
            try {
                auto snapshot = request();
                request = {}; // Release the read-only source before disk I/O.
                out.sequence = session->checkpoint(snapshot);
            } catch (const std::exception &e) {
                out.error = qErrorText(e);
            }
            return out;
        }));
    } catch (const std::exception &e) {
        setStatus("자동복구 저장 실패 · " + qErrorText(e));
        deadline_.start(intervalMs_);
    }
}
void AutoSaveController::finishDocument() {
    finished_ = true;
    idle_.stop();
    deadline_.stop();
    retire(std::move(session_));
    recoveredSession_.reset();
}
void AutoSaveController::adoptRecovered(std::shared_ptr<RecoveryClaim> claim) {
    recoveredSession_ = std::move(claim->session);
}
} // namespace jobs
