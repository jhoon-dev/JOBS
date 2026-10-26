#pragma once
#include "application/Document.h"
#include <QDialog>
class QComboBox;
class QLabel;
class QTableWidget;
class QPlainTextEdit;
namespace jobs {
class ComparisonDialog final : public QDialog {
public:
    ComparisonDialog(Document& document, const std::string& baselineId, QWidget* parent = nullptr);
private:
    Document& document_;
    QComboBox* baseline_;
    QLabel* summary_;
    QTableWidget* table_;
    QPlainTextEdit* diagnostics_;
    QPlainTextEdit* details_;
    std::uint64_t generation_ = 0;
    void recalculate();
    void display(const ComparisonReport& report);
};
}
