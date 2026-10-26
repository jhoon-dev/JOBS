#pragma once
#include "application/Document.h"
#include "reporting/ReportExport.h"
#include <QDialog>
#include <memory>
class QComboBox;
class QCheckBox;
class QDateEdit;
class QPushButton;
class QLabel;
class QTabWidget;
class QCloseEvent;
namespace jobs {
class ReportDialog final : public QDialog {
public:
    ReportDialog(Document&, const std::string& wbs, const std::string& baseline, QWidget* parent = nullptr);
    void reject() override;
protected:
    void closeEvent(QCloseEvent*) override;
private:
    Document& document_;
    QComboBox *scope_, *baseline_, *formula_, *family_, *paper_, *orientation_, *fontSize_;
    QCheckBox* period_;
    QDateEdit *from_, *to_;
    QPushButton *refresh_, *xlsx_, *pdf_;
    QLabel* message_;
    QTabWidget* tabs_;
    std::shared_ptr<const ProjectReport> report_;
    std::uint64_t generation_ = 0;
    bool exporting_ = false;
    void prepare();
    void clear();
    void showReport();
    bool current() const;
    void exportFile(bool pdf);
    void exportEnabled();
};
}
