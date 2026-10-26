#pragma once
#include "application/Document.h"
#include "calculation/Resources.h"
#include <QDialog>
class QComboBox;
class QSpinBox;
class QLabel;
class QTableWidget;
class QPlainTextEdit;
class QLineEdit;
class QPushButton;
namespace jobs {
class ResourceComparisonDialog final : public QDialog {
public:
    ResourceComparisonDialog(Document&, QWidget* parent = nullptr);
private:
    Document& document_;
    QComboBox *sourceChoice_, *otherChoice_, *priority_;
    QSpinBox* horizon_;
    QLineEdit* name_;
    QPushButton* save_;
    QLabel* summary_;
    QTableWidget *tasks_, *before_, *after_, *other_;
    QPlainTextEdit* diagnostics_;
    std::optional<Project> source_;
    std::optional<LevelingResult> result_;
    std::uint64_t generation_ = 0;
    void recalculate();
    void invalidate();
    void saveScenario();
};
}
