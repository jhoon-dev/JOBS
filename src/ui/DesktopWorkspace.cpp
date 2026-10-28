#include "ui/DesktopWorkspace.h"
#include "ui/MainWindow.h"
#include "ui/WorkspaceDialog.h"
#include "persistence/JsonRecoveryStore.h"
#include "formatting/QtDiagnosticText.h"
#include <QDialog>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <set>

namespace jobs {
struct DesktopWorkspace::Session {
    std::uint64_t serial = 0, listener = 0;
    std::unique_ptr<Document> document;
    QPointer<MainWindow> window;
};
DesktopWorkspace::DesktopWorkspace(std::shared_ptr<JsonProjectStore> files, std::shared_ptr<JsonRecoveryStore> recovery,
                                   QObject *parent)
    : QObject(parent), files_(std::move(files)), recovery_(std::move(recovery)) {
    configuration_.id = files_->newId();
    saved_ = configuration_;
}
DesktopWorkspace::~DesktopWorkspace() {
    shuttingDown_ = true;
    for (auto &session : sessions_)
        if (session->window)
            delete session->window.data();
    sessions_.clear();
}
void DesktopWorkspace::synchronize(Session &session) {
    ++generation_;
    const auto snapshot = session.document->snapshot();
    if (!snapshot)
        return;
    auto found = std::find_if(configuration_.projects.begin(), configuration_.projects.end(),
                              [&](const auto &ref) { return ref.documentId == snapshot->documentId; });
    ProjectReference reference{snapshot->documentId, session.document->path(), snapshot->name};
    if (found == configuration_.projects.end()) {
        configuration_.projects.push_back(std::move(reference));
        ++catalogGeneration_;
    } else if (*found != reference) {
        *found = std::move(reference);
        ++catalogGeneration_;
    }
}
MainWindow *DesktopWorkspace::adopt(std::unique_ptr<Document> document) {
    auto session = std::make_unique<Session>();
    session->serial = ++serial_;
    session->document = std::move(document);
    auto *pointer = session.get();
    auto *window = new MainWindow(*session->document, recovery_.get(), this);
    session->window = window;
    window->setAttribute(Qt::WA_DeleteOnClose);
    session->listener = session->document->subscribe([this, pointer](Document::Event) { synchronize(*pointer); });
    const auto serial = session->serial;
    connect(window, &QObject::destroyed, this, [this, serial] {
        if (shuttingDown_)
            return;
        ++generation_;
        // QObject emits destroyed before deleting child controllers which still borrow Document.
        QTimer::singleShot(0, this, [this, serial] {
            const auto it = std::find_if(sessions_.begin(), sessions_.end(),
                                         [&](const auto &item) { return item->serial == serial; });
            if (it == sessions_.end())
                return;
            if (const auto snapshot = (*it)->document->snapshot(); snapshot && (*it)->document->path().empty()) {
                const auto id = snapshot->documentId;
                std::erase_if(configuration_.projects, [&](const auto &ref) { return ref.documentId == id; });
                std::erase_if(configuration_.bindings, [&](const auto &binding) { return binding.projectId == id; });
            }
            sessions_.erase(it);
            ++generation_;
            ++catalogGeneration_;
        });
    });
    sessions_.push_back(std::move(session));
    synchronize(*pointer);
    window->show();
    return window;
}
MainWindow *DesktopWorkspace::newWindow() {
    return adopt(std::make_unique<Document>(*files_));
}
void DesktopWorkspace::createProject() {
    auto *created = newWindow();
    created->create();
}
MainWindow *DesktopWorkspace::openProject(const std::string &path, bool readOnly) {
    const auto normalized = files_->normalizePath(path);
    for (const auto &session : sessions_)
        if (session->window && files_->sameFile(normalized, session->document->path())) {
            session->window->showNormal();
            session->window->raise();
            session->window->activateWindow();
            return session->window;
        }
    auto document = std::make_unique<Document>(*files_);
    document->open(normalized, readOnly);
    const auto id = document->project()->documentId;
    if (this->document(id))
        throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {id});
    for (const auto &reference : configuration_.projects)
        if (reference.documentId == id && !reference.path.empty() && !files_->sameFile(reference.path, normalized))
            throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {id});
    return adopt(std::move(document));
}
Document *DesktopWorkspace::document(const std::string &id) const {
    for (const auto &session : sessions_)
        if (session->window && session->window->isVisible() && session->document->project() &&
            session->document->project()->documentId == id)
            return session->document.get();
    return nullptr;
}
MainWindow *DesktopWorkspace::window(const std::string &id) const {
    for (const auto &session : sessions_)
        if (session->window && session->window->isVisible() && session->document->project() &&
            session->document->project()->documentId == id)
            return session->window;
    return nullptr;
}
std::vector<std::shared_ptr<const Project>> DesktopWorkspace::snapshots() const {
    std::vector<std::shared_ptr<const Project>> result;
    for (const auto &session : sessions_)
        if (session->window && session->window->isVisible())
            if (auto snapshot = session->document->snapshot())
                result.push_back(std::move(snapshot));
    return result;
}
bool DesktopWorkspace::activate(const std::string &projectId, const std::string &taskId) {
    auto *target = window(projectId);
    if (!target)
        return false;
    target->showNormal();
    target->raise();
    target->activateWindow();
    return taskId.empty() || target->selectTaskById(taskId);
}
void DesktopWorkspace::applyConfiguration(Workspace proposed) {
    validateWorkspace(proposed);
    configuration_ = std::move(proposed);
    engaged_ = true;
    ++generation_;
    ++catalogGeneration_;
}
void DesktopWorkspace::removeClosedProject(const std::string &id) {
    if (document(id))
        throw AppError(ErrorCode::WORKSPACE_INVALID, {"close the project window first"});
    auto proposed = configuration_;
    std::erase_if(proposed.projects, [&](const auto &ref) { return ref.documentId == id; });
    std::erase_if(proposed.bindings, [&](const auto &binding) { return binding.projectId == id; });
    applyConfiguration(std::move(proposed));
}
bool DesktopWorkspace::saveWorkspace(QWidget *parent, bool as) {
    try {
        for (const auto &session : sessions_)
            if (session->window && session->document->project())
                if (session->document->dirty() || session->document->path().empty())
                    throw AppError(ErrorCode::WORKSPACE_SAVE_PROJECTS_FIRST, {session->document->project()->name});
        auto target = path_;
        if (as || target.empty()) {
            auto selected =
                QFileDialog::getSaveFileName(parent, "작업공간 사본 저장", {}, "JOBS 작업공간 (*.jobsworkspace)");
            if (selected.isEmpty())
                return false;
            if (!selected.endsWith(".jobsworkspace", Qt::CaseInsensitive))
                selected += ".jobsworkspace";
            target = files_->normalizePath(selected.toStdString());
            if (as && files_->sameFile(target, path_))
                throw AppError(ErrorCode::WORKSPACE_FILE_EXISTS);
        }
        const bool same = files_->sameFile(target, path_);
        auto lease = same ? nullptr : files_->lock(target);
        auto proposed = configuration_;
        if (!same && !path_.empty())
            proposed.id = files_->newId();
        const auto fingerprint =
            JsonWorkspaceStore(*files_).save(target, proposed, same ? fingerprint_ : std::string{});
        configuration_ = std::move(proposed);
        saved_ = configuration_;
        path_ = target;
        fingerprint_ = fingerprint;
        if (!same)
            lease_ = std::move(lease);
        engaged_ = true;
        ++generation_;
        return true;
    } catch (const std::exception &error) {
        QMessageBox::warning(parent, "작업공간 저장 실패", qErrorText(error));
        return false;
    }
}
bool DesktopWorkspace::confirmClosing(MainWindow *closing) {
    if (!engaged_ || !dirty())
        return true;
    if (std::any_of(sessions_.begin(), sessions_.end(), [&](const auto &session) {
            return session->window && session->window->isVisible() && session->window != closing;
        }))
        return true;
    const auto answer =
        QMessageBox::question(closing, "작업공간 변경 저장", "프로젝트 목록·공유 자원 연결의 변경을 저장하시겠습니까?",
                              QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Cancel)
        return false;
    return answer == QMessageBox::Discard || saveWorkspace(closing);
}
void DesktopWorkspace::openWorkspace(const std::string &path, QWidget *parent) {
    if (engaged_ && dirty()) {
        const auto answer = QMessageBox::question(
            parent, "작업공간 변경 저장", "현재 공유 자원/연결 변경을 저장한 뒤 작업공간을 여시겠습니까?",
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
        if (answer == QMessageBox::Cancel || (answer == QMessageBox::Save && !saveWorkspace(parent)))
            return;
    }
    struct Prepared {
        std::shared_ptr<JsonProjectStore> files;
        StoredWorkspace stored;
        std::unique_ptr<WriteLease> lease;
        std::vector<std::unique_ptr<Document>> documents;
        std::optional<Diagnostic> failure;
        bool cancelled = false;
    };
    const auto generation = generation_;
    const auto normalized = files_->normalizePath(path);
    const bool same = files_->sameFile(normalized, path_) && bool(lease_);
    std::vector<std::pair<std::string, std::string>> active;
    for (const auto &session : sessions_)
        if (session->window && session->document->project())
            active.emplace_back(session->document->path(), session->document->project()->documentId);
    auto control = std::make_shared<CalculationControl>();
    QDialog dialog(parent);
    dialog.setWindowTitle("작업공간 열기");
    dialog.setWindowModality(Qt::ApplicationModal);
    auto *layout = new QVBoxLayout(&dialog);
    auto *status = new QLabel("프로젝트 파일과 잠금을 준비하고 있습니다…");
    layout->addWidget(status);
    auto *cancel = new QPushButton("취소");
    layout->addWidget(cancel);
    connect(cancel, &QPushButton::clicked, &dialog, [&] {
        control->cancel();
        status->setText("취소 중…");
    });
    connect(&dialog, &QDialog::rejected, &dialog, [control] { control->cancel(); });
    QFutureWatcher<std::shared_ptr<Prepared>> watcher;
    connect(&watcher, &QFutureWatcher<std::shared_ptr<Prepared>>::finished, &dialog, &QDialog::accept);
    watcher.setFuture(QtConcurrent::run([files = files_, normalized, same, active, control] {
        auto result = std::make_shared<Prepared>();
        result->files = files;
        try {
            EvaluationScope scope(control);
            calculationCheckpoint();
            if (!same)
                result->lease = files->lock(normalized);
            result->stored = JsonWorkspaceStore(*files).load(normalized);
            std::set<std::string> identities;
            for (const auto &reference : result->stored.workspace.projects) {
                calculationCheckpoint();
                bool already = false;
                for (const auto &[path, id] : active) {
                    if (files->sameFile(path, reference.path)) {
                        if (id != reference.documentId)
                            throw AppError(ErrorCode::WORKSPACE_PROJECT_ID_MISMATCH, {reference.path});
                        already = true;
                    } else if (id == reference.documentId)
                        throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {id});
                }
                if (already)
                    continue;
                auto document = std::make_unique<Document>(*files);
                document->open(reference.path);
                const auto id = document->project()->documentId;
                if (id != reference.documentId)
                    throw AppError(ErrorCode::WORKSPACE_PROJECT_ID_MISMATCH, {reference.path});
                if (!identities.insert(id).second)
                    throw AppError(ErrorCode::WORKSPACE_DUPLICATE_PROJECT, {id});
                result->documents.push_back(std::move(document));
            }
        } catch (const CalculationCancelled &) {
            result->cancelled = true;
        } catch (const std::exception &error) {
            result->failure = diagnosticFromException(error);
        }
        return result;
    }));
    dialog.exec();
    if (!watcher.isFinished() || control->cancelRequested.load())
        return;
    auto result = watcher.result();
    if (result->cancelled)
        return;
    if (result->failure) {
        QMessageBox::warning(parent, "작업공간 열기 실패", qDiagnosticText(*result->failure));
        return;
    }
    if (generation_ != generation) {
        QMessageBox::warning(parent, "작업공간 변경", qDiagnosticText({"WORKSPACE_SOURCE_CHANGED", {}, {}}));
        return;
    }
    // No visible changes or new windows until every referenced document is ready.
    const auto saved = result->stored.workspace;
    auto proposed = saved;
    // Already-open independent projects remain open and become explicit members.
    for (const auto &snapshot : snapshots())
        if (std::none_of(proposed.projects.begin(), proposed.projects.end(),
                         [&](const auto &ref) { return ref.documentId == snapshot->documentId; })) {
            auto *source = document(snapshot->documentId);
            proposed.projects.push_back({snapshot->documentId, source->path(), snapshot->name});
        }
    try {
        validateWorkspace(proposed);
    } catch (const std::exception &error) {
        QMessageBox::warning(parent, "작업공간 열기 실패", qErrorText(error));
        return;
    }
    for (auto &document : result->documents)
        adopt(std::move(document));
    configuration_ = std::move(proposed);
    saved_ = saved;
    path_ = result->stored.path;
    fingerprint_ = result->stored.fingerprint;
    if (!same)
        lease_ = std::move(result->lease);
    engaged_ = true;
    ++generation_;
    ++catalogGeneration_;
}
void DesktopWorkspace::showWorkspace(QWidget *parent) {
    engaged_ = true;
    auto *dialog = new WorkspaceDialog(*this, parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}
} // namespace jobs
