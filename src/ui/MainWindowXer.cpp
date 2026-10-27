#include "persistence/PrimaveraXer.h"
#include "ui/MainWindow.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStatusBar>
#include <QVBoxLayout>
#include <stdexcept>
namespace jobs {
namespace {
QString qs(const std::string &s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
QString preview(QWidget *parent, const PrimaveraXerExchange &e, const QString &proposed) {
    QDialog dialog(parent);
    dialog.setObjectName("primaveraXerPreview");
    dialog.setWindowTitle(e.importing ? "Primavera XER 가져오기 · 변환 확인" : "Primavera XER 내보내기 · 변환 확인");
    dialog.resize(800, 580);
    auto *layout = new QVBoxLayout(&dialog);
    auto *detail = new QPlainTextEdit(primaveraXerPreview(e), &dialog);
    detail->setObjectName("xerDiagnostics");
    detail->setReadOnly(true);
    layout->addWidget(detail);
    auto *note = new QLabel("새 교환 폴더 전체 경로 · 기존 폴더는 덮어쓰지 않습니다", &dialog);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *destination = new QLineEdit(proposed, &dialog);
    destination->setObjectName("xerDestination");
    destination->setAccessibleName("새 교환 폴더 경로");
    layout->addWidget(destination);
    auto *choose = new QPushButton("상위 폴더 선택…", &dialog);
    layout->addWidget(choose);
    QObject::connect(choose, &QPushButton::clicked, &dialog, [&] {
        auto path = QFileDialog::getExistingDirectory(&dialog, "교환 폴더를 만들 위치", QFileInfo(destination->text()).absolutePath());
        if (!path.isEmpty())
            destination->setText(QDir(path).filePath(QFileInfo(destination->text()).fileName()));
    });
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText(e.importing ? "변환 확인 · 새 사본 열기" : "변환 확인 · 내보내기");
    layout->addWidget(buttons);
    auto *error = new QLabel(&dialog);
    error->setWordWrap(true);
    layout->addWidget(error);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        auto path = QFileInfo(destination->text().trimmed());
        if (destination->text().trimmed().isEmpty() || path.exists() || path.isSymLink() || !path.dir().exists()) {
            error->setText("존재하는 상위 폴더 아래의 새 폴더 경로를 입력하세요.");
            return;
        }
        dialog.accept();
    });
    return dialog.exec() == QDialog::Accepted ? destination->text().trimmed() : QString{};
}
std::optional<XerEncoding> encoding(QWidget *parent) {
    QDialog dialog(parent);
    dialog.setObjectName("xerEncodingDialog");
    dialog.setWindowTitle("XER 원본 문자 인코딩");
    auto *layout = new QVBoxLayout(&dialog);
    auto *label = new QLabel(
        "원본 인코딩을 선택하세요. 자동 추측하지 않습니다.\nCP949 등은 원본을 보관하고 UTF-8 사본으로 변환해야 합니다.", &dialog);
    label->setWordWrap(true);
    layout->addWidget(label);
    auto *choice = new QComboBox(&dialog);
    choice->setObjectName("xerEncoding");
    choice->addItems({"UTF-8 (BOM 허용 · JOBS 내보내기)", "Windows-1252 (서유럽)"});
    layout->addWidget(choice);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted)
        return {};
    return choice->currentIndex() == 0 ? XerEncoding::Utf8 : XerEncoding::Windows1252;
}
} // namespace
void MainWindow::importXer() {
    const auto path = QFileDialog::getOpenFileName(this, "Primavera XER 가져오기", {}, "Primavera XER (*.xer)");
    if (path.isEmpty())
        return;
    auto charset = encoding(this);
    if (!charset)
        return;
    try {
        const auto generation = document_.sessionGeneration(), revision = document_.project() ? document_.project()->revision : 0;
        auto exchange = importPrimaveraXer(readPrimaveraXer(path), QFileInfo(path).fileName(), *charset);
        auto destination =
            preview(this, exchange, QDir(QFileInfo(path).absolutePath()).filePath(QFileInfo(path).completeBaseName() + "-xer-import"));
        if (destination.isEmpty())
            return;
        if (generation != document_.sessionGeneration() || revision != (document_.project() ? document_.project()->revision : 0))
            throw std::runtime_error("미리보기 중 문서가 변경되었습니다. 다시 가져오세요.");
        if (!confirmChanges())
            return;
        auto folder = publishPrimaveraXer(exchange, destination);
        document_.open(QDir(folder).filePath("project.jobs").toUtf8().toStdString());
        refresh();
        statusBar()->showMessage("XER 계획 변환 완료 · 원본/진단: " + folder);
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::exportXer() {
    if (!document_.project())
        return;
    try {
        auto exchange = exportPrimaveraXer(*document_.project());
        auto parent = document_.path().empty() ? QDir::homePath() : QFileInfo(qs(document_.path())).absolutePath();
        auto destination = preview(this, exchange, QDir(parent).filePath("jobs-xer-export"));
        if (destination.isEmpty())
            return;
        if (!document_.project() || *document_.project() != exchange.project)
            throw std::runtime_error("미리보기 중 문서가 변경되었습니다. 다시 내보내세요.");
        auto folder = publishPrimaveraXer(exchange, destination);
        statusBar()->showMessage("XER 내보내기 완료 · 원본/진단: " + folder);
    } catch (const std::exception &e) {
        report(e);
    }
}
} // namespace jobs
