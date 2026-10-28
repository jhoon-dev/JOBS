#pragma once
#include "calculation/SharedResources.h"
#include <QDialog>
class QTableWidget;
class QTableView;
class QLabel;
class QPushButton;
class QComboBox;
class QPlainTextEdit;
namespace jobs {
class DesktopWorkspace;
class SharedLoadModel;
class WorkspaceDialog final : public QDialog {
public:
    explicit WorkspaceDialog(DesktopWorkspace &, QWidget *parent = nullptr);
    ~WorkspaceDialog() override;
    void startAnalysis();
    void cancelAnalysis();
    std::shared_ptr<const SharedResourceResult> result() const {
        return result_;
    }
    bool busy() const {
        return busy_;
    }

private:
    DesktopWorkspace &workspace_;
    QTableWidget *projects_, *resources_, *bindings_;
    QTableView *load_;
    SharedLoadModel *loadModel_;
    QLabel *status_, *path_;
    QPlainTextEdit *diagnosticDetails_;
    QPushButton *analyze_, *cancel_, *navigate_;
    QComboBox *mode_, *task_;
    std::shared_ptr<CalculationControl> control_;
    std::shared_ptr<const SharedResourceResult> result_;
    std::uint64_t viewGeneration_ = ~std::uint64_t{0}, resultGeneration_ = 0;
    std::uint64_t catalogGeneration_ = ~std::uint64_t{0};
    bool busy_ = false;
    void refresh();
    void editResource(bool create);
    void bindResource();
    void taskChoices();
};
} // namespace jobs
