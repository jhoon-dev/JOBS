#pragma once
#include "persistence/JsonRecoveryStore.h"
#include <QDialog>
class QTableWidget;
class QLabel;
class QPushButton;
class QCloseEvent;
namespace jobs {
class RecoveryDialog final : public QDialog {
public:
    RecoveryDialog(JsonRecoveryStore, QWidget* parent = nullptr, std::optional<RecoveryScan> initial = {});
    std::shared_ptr<RecoveryClaim> takeClaim() { return std::move(claim_); }
    void reject() override;
protected:
    void closeEvent(QCloseEvent*) override;
private:
    JsonRecoveryStore store_;
    RecoveryScan scan_;
    QTableWidget* table_;
    QLabel *message_, *details_;
    QPushButton *restore_, *discard_, *reload_;
    std::shared_ptr<RecoveryClaim> claim_;
    bool busy_ = false;
    bool protectedOperation_ = false;
    void reload();
    void populate(RecoveryScan);
    void select();
    void restore();
    void discard();
    void setBusy(bool);
};
}
