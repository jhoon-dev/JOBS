#include "ui/AsyncEditController.h"
#include <QFutureWatcher>
#include <QPointer>
#include <QtConcurrent/QtConcurrentRun>

namespace jobs {
AsyncEditController::AsyncEditController(Document& document, QObject* parent) : QObject(parent), document_(document) {
    setObjectName("asyncEditController");
    listener_ = document_.subscribe([this](Document::Event event) {
        if (!applying_ && event != Document::Event::Saved) invalidate(EditState::Stale);
        // Save As changes the session even though the notification is Saved.
        if (!applying_ && event == Document::Event::Saved && busy_ && activeSession_ != document_.sessionGeneration())
            invalidate(EditState::Stale);
    });
}
AsyncEditController::~AsyncEditController() {
    if (control_) control_->cancel();
    if (pending_) pending_->control->cancel();
    document_.unsubscribe(listener_);
}
std::shared_ptr<CalculationControl> AsyncEditController::submit(EditCommand command, QString label, Completion completion) {
    auto control = std::make_shared<CalculationControl>();
    Pending next{document_.makeEditRequest(std::move(command), captureView ? captureView() : Document::ViewPreparation{}), std::move(label), std::move(completion), control};
    QPointer<AsyncEditController> guard(this);
    if (busy_) {
        invalidate(EditState::Cancelled);
        if (!guard) { control->cancel(); return control; }
        pending_.emplace(std::move(next));
    } else {
        if (pending_) invalidate(EditState::Cancelled);
        if (!guard) { control->cancel(); return control; }
        start(std::move(next));
    }
    return control;
}
void AsyncEditController::cancel() { invalidate(EditState::Cancelled); }
void AsyncEditController::invalidate(EditState reason) {
    if (!busy_ && !pending_) return;
    cancelState_ = reason;
    if (control_) control_->cancel();
    if (pending_) {
        pending_->control->cancel();
        auto callback = std::move(pending_->completion);
        pending_.reset();
        QPointer<AsyncEditController> guard(this);
        if (callback) callback(reason, reason == EditState::Stale ? "문서가 변경되어 대기 편집을 취소했습니다." : "편집 계산을 취소했습니다.");
        if (!guard) return;
    }
    if (stateChanged) stateChanged();
}
void AsyncEditController::start(Pending pending) {
    busy_ = true; cancelState_ = EditState::Cancelled;
    label_ = std::move(pending.label);
    control_ = std::move(pending.control);
    activeSession_ = document_.sessionGeneration();
    auto control = control_;
    auto* watcher = new QFutureWatcher<std::shared_ptr<Document::PreparedEdit>>(this);
    connect(watcher, &QFutureWatcher<std::shared_ptr<Document::PreparedEdit>>::finished, this,
            [this, watcher, control, completion = std::move(pending.completion)] {
        EditState state = EditState::Failed;
        QString message;
        QPointer<AsyncEditController> guard(this);
        struct Retire {
            std::shared_ptr<Document::PreparedEdit> result;
            ~Retire() {
                if (!result) return;
                auto roots = result->takeRetiredProjects();
                if (!roots.empty()) (void)QtConcurrent::run([roots = std::move(roots)] {});
            }
        } retire;
        try {
            auto prepared = watcher->result();
            retire.result = prepared;
            if (control->cancelRequested.load()) state = cancelState_;
            else {
                if(beforeApply)beforeApply();
                if(!guard)return;
                if(control->cancelRequested.load())state=cancelState_;
                else {
                applying_ = true;
                try { state = document_.applyPreparedEdit(*prepared); }
                catch (...) { if (guard) applying_ = false; throw; }
                if (!guard) return;
                applying_ = false;
                }
            }
            if (state == EditState::Failed) message = QString::fromUtf8(prepared->error());
        } catch (const CalculationCancelled&) { state = EditState::Cancelled; }
          catch (const std::exception& error) { message = QString::fromUtf8(error.what()); }
        if (!guard) return;
        if (state == EditState::Cancelled) message = "편집 계산을 취소했습니다. 입력은 변경하지 않았습니다.";
        if (state == EditState::Stale) message = "문서가 변경되어 오래된 편집 결과를 반영하지 않았습니다.";
        busy_ = false; control_.reset(); label_.clear();
        watcher->deleteLater();
        if (completion) completion(state, message);
        if (!guard) return;
        if (pending_ && !busy_) {
            auto next = std::move(*pending_); pending_.reset();
            start(std::move(next));
        } else if (stateChanged) stateChanged();
    });
    watcher->setFuture(QtConcurrent::run([request = std::move(pending.request), control]() mutable {
        return Document::prepareEdit(std::move(request), control);
    }));
    if (stateChanged) stateChanged();
}
}
