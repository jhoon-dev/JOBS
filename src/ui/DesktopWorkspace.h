#pragma once
#include "application/Document.h"
#include "domain/Workspace.h"
#include "persistence/JsonProjectStore.h"
#include "persistence/JsonWorkspaceStore.h"
#include <QObject>
#include <QPointer>

class QWidget;
namespace jobs {
class MainWindow;
class JsonRecoveryStore;
class UpdateController;
class DesktopWorkspace final : public QObject {
public:
    explicit DesktopWorkspace(std::shared_ptr<JsonProjectStore>, std::shared_ptr<JsonRecoveryStore>,
                              QObject *parent = nullptr);
    ~DesktopWorkspace() override;
    void setUpdates(UpdateController *updates) {
        updates_ = updates;
    }
    bool prepareForUpdate();
    void finishUpdate();
    MainWindow *newWindow();
    MainWindow *openProject(const std::string &path, bool readOnly = false);
    void createProject();
    void openWorkspace(const std::string &path, QWidget *parent = nullptr);
    bool saveWorkspace(QWidget *parent = nullptr, bool saveAs = false);
    bool confirmClosing(MainWindow *);
    void showWorkspace(QWidget *parent = nullptr);
    void applyConfiguration(Workspace);
    const Workspace &configuration() const {
        return configuration_;
    }
    bool dirty() const {
        return configuration_ != saved_;
    }
    std::uint64_t generation() const {
        return generation_;
    }
    std::uint64_t catalogGeneration() const {
        return catalogGeneration_;
    }
    std::vector<std::shared_ptr<const Project>> snapshots() const;
    Document *document(const std::string &projectId) const;
    MainWindow *window(const std::string &projectId) const;
    bool activate(const std::string &projectId, const std::string &taskId = {});
    void removeClosedProject(const std::string &projectId);
    const std::string &path() const {
        return path_;
    }
    std::string newId() const {
        return files_->newId();
    }

private:
    struct Session;
    std::shared_ptr<JsonProjectStore> files_;
    std::shared_ptr<JsonRecoveryStore> recovery_;
    std::vector<std::unique_ptr<Session>> sessions_;
    Workspace configuration_, saved_;
    std::string path_, fingerprint_;
    std::unique_ptr<WriteLease> lease_;
    std::uint64_t generation_ = 0, serial_ = 0, catalogGeneration_ = 0;
    bool engaged_ = false, shuttingDown_ = false;
    UpdateController *updates_ = nullptr;
    MainWindow *adopt(std::unique_ptr<Document>);
    void synchronize(Session &);
};
} // namespace jobs
