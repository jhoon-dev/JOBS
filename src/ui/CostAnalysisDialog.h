#pragma once
#include "application/Document.h"
#include "calculation/Costs.h"
#include <QDialog>
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QTableWidget;
namespace jobs {
class CostChart;
class CostAnalysisDialog final : public QDialog {
public:
    CostAnalysisDialog(Document&, const std::string& baselineId, QWidget* parent = nullptr);
private:
    Document& document_;
    QComboBox *baseline_, *formula_;
    QLabel* summary_;
    QTableWidget *tasks_, *groups_, *days_;
    QPlainTextEdit* diagnostics_;
    CostChart* chart_;
    std::uint64_t generation_ = 0;
    void recalculate();
    void display(const CostAnalysis&);
};
}
