#include "formatting/QtDiagnosticText.h"
#include "ui/MainWindow.h"
#include "ui/ExchangeOperation.h"
#include "persistence/ProjectXml.h"
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
QString qs(const std::string &s) {
    return QString::fromUtf8(s.data(), qsizetype(s.size()));
}
QString preview(QWidget *parent, const ProjectXmlExchange &e, const QString &proposed) {
    QDialog dialog(parent);
    dialog.setObjectName("projectXmlPreview");
    dialog.setWindowTitle(e.importing ? "Project XML 가져오기 · 변환 확인" : "Project XML 내보내기 · 변환 확인");
    dialog.resize(800, 580);
    auto *layout = new QVBoxLayout(&dialog);
    auto *detail = new QPlainTextEdit(projectXmlPreview(e), &dialog);
    detail->setObjectName("xmlDiagnostics");
    detail->setReadOnly(true);
    layout->addWidget(detail);
    auto *note = new QLabel("새 교환 폴더 전체 경로 · 기존 폴더는 덮어쓰지 않습니다", &dialog);
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *destination = new QLineEdit(proposed, &dialog);
    destination->setObjectName("xmlDestination");
    destination->setAccessibleName("새 교환 폴더 경로");
    layout->addWidget(destination);
    auto *choose = new QPushButton("상위 폴더 선택…", &dialog);
    layout->addWidget(choose);
    QObject::connect(choose, &QPushButton::clicked, &dialog, [&] {
        auto path = QFileDialog::getExistingDirectory(&dialog, "교환 폴더를 만들 위치",
                                                      QFileInfo(destination->text()).absolutePath());
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
} // namespace
void MainWindow::importXml() {
    const auto path = QFileDialog::getOpenFileName(this, "Project XML 가져오기", {}, "Project XML (*.xml)");
    if (path.isEmpty())
        return;
    try {
        const auto generation = document_.sessionGeneration();
        const auto revision = document_.project() ? document_.project()->revision : 0;
        auto prepared = prepareExchange<ProjectXmlExchange>(this, document_, "XML 읽기·변환·검증 중…", [path] {
            return importProjectXml(readProjectXml(path), QFileInfo(path).fileName());
        });
        if (!prepared)
            return;
        auto exchange = std::move(*prepared);
        auto proposed =
            QDir(QFileInfo(path).absolutePath()).filePath(QFileInfo(path).completeBaseName() + "-jobs-import");
        auto destination = preview(this, exchange, proposed);
        if (destination.isEmpty())
            return;
        if (generation != document_.sessionGeneration() ||
            revision != (document_.project() ? document_.project()->revision : 0))
            throw AppError(ErrorCode::MAINWINDOWXER_001, {});
        if (!confirmChanges())
            return;
        const auto publishSession = document_.sessionGeneration();
        const auto publishRevision = document_.project() ? document_.project()->revision : 0;
        QString folder;
        if (!runExchangeOperation(
                this, document_, "XML 사본 게시·검증 중…", [&] { folder = publishProjectXml(exchange, destination); },
                true))
            return;
        if (document_.sessionGeneration() != publishSession ||
            (document_.project() ? document_.project()->revision : 0) != publishRevision)
            throw AppError(ErrorCode::EXCHANGE_SOURCE_CHANGED);
        document_.open(QDir(folder).filePath("project.jobs").toUtf8().toStdString());
        refresh();
        statusBar()->showMessage("XML 계획 변환 완료 · 원본/진단: " + folder);
    } catch (const std::exception &e) {
        report(e);
    }
}
void MainWindow::exportXml() {
    if (!document_.project())
        return;
    try {
        const auto generation = document_.sessionGeneration(), revision = document_.project()->revision;
        auto snapshot = document_.snapshot();
        auto prepared = prepareExchange<ProjectXmlExchange>(this, document_, "XML 내보내기 준비·검증 중…",
                                                            [snapshot] { return exportProjectXml(*snapshot); });
        snapshot.reset();
        if (!prepared)
            return;
        auto exchange = std::move(*prepared);
        auto parent = document_.path().empty() ? QDir::homePath() : QFileInfo(qs(document_.path())).absolutePath();
        auto destination = preview(this, exchange, QDir(parent).filePath("jobs-xml-export"));
        if (destination.isEmpty())
            return;
        if (document_.sessionGeneration() != generation || !document_.project() ||
            document_.project()->revision != revision)
            throw AppError(ErrorCode::MAINWINDOWXER_002, {});
        QString folder;
        if (!runExchangeOperation(
                this, document_, "XML 게시·기준선 검증 중…", [&] { folder = publishProjectXml(exchange, destination); },
                true))
            return;
        statusBar()->showMessage("XML 내보내기 완료 · 원본/진단: " + folder);
    } catch (const std::exception &e) {
        report(e);
    }
}
} // namespace jobs
