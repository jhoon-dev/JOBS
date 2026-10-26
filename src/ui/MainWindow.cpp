#include "ui/MainWindow.h"
#include <QAction>
#include <QCloseEvent>
#include <QDateEdit>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTableWidget>
#include <QToolBar>
#include <QVBoxLayout>
#include <functional>

namespace jobs {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
}
MainWindow::MainWindow(Document& document) : document_(document) {
    resize(1180, 760);
    auto* menu = menuBar()->addMenu("파일(&F)");
    auto* tools = addToolBar("파일 및 작업"); tools->setMovable(false);
    auto action = [&](const QString& name, const QKeySequence& shortcut, std::function<void()> callback) {
        auto* a = menu->addAction(name); a->setShortcut(shortcut);
        connect(a, &QAction::triggered, this, std::move(callback)); tools->addAction(a); return a;
    };
    action("새로 만들기(&N)", QKeySequence::New, [this] { create(); });
    action("열기(&O)…", QKeySequence::Open, [this] { open(); });
    save_ = action("저장(&S)", QKeySequence::Save, [this] { save(false); });
    saveAs_ = action("다른 이름으로 저장(&A)…", QKeySequence::SaveAs, [this] { save(true); });
    menu->addSeparator();
    auto* quit = menu->addAction("종료(&X)"); quit->setShortcut(QKeySequence::Quit);
    connect(quit, &QAction::triggered, this, &QWidget::close);
    add_ = tools->addAction("작업 추가"); add_->setShortcut(QKeySequence("Ctrl+Insert"));
    connect(add_, &QAction::triggered, this, [this] {
        bool ok = false;
        auto name = QInputDialog::getText(this, "작업 추가", "작업명", QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok) return;
        try { document_.addTask(str(name)); refresh(); } catch (const std::exception& e) { report(e); }
    });
    auto* central = new QWidget(this); auto* layout = new QVBoxLayout(central);
    auto* heading = new QLabel("<h1>JOBS</h1>프로젝트 일정 · 로컬 프로젝트 파일", central);
    layout->addWidget(heading);
    summary_ = new QLabel(central); summary_->setTextFormat(Qt::PlainText); summary_->setWordWrap(true);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(summary_);
    table_ = new QTableWidget(0, 5, central);
    table_->setHorizontalHeaderLabels({"작업명", "기간", "선행 작업", "자원", "안정 ID"});
    table_->setAccessibleName("프로젝트 작업 목록");
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows); layout->addWidget(table_);
    auto* note = new QLabel("기본 달력: 월–금 · 1일 8시간 · 휴일 없음\n기간·선행·자원은 미설정입니다. 일정 계산·간트·자동복구는 후속 구현입니다.", central);
    note->setWordWrap(true); layout->addWidget(note); setCentralWidget(central);
    connect(table_, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (column != 0) return;
        try { document_.renameTask(std::size_t(row), str(table_->item(row, 0)->text().trimmed())); }
        catch (const std::exception& e) { report(e); }
        refresh();
    });
    refresh();
}
void MainWindow::report(const std::exception& e) { QMessageBox::warning(this, "JOBS · 작업 실패", QString::fromUtf8(e.what())); }
void MainWindow::refresh() {
    const QSignalBlocker blocker(table_);
    const auto& p = document_.project();
    setWindowTitle((p ? qs(p->name) : "프로젝트를 만들거나 여세요") + "[*] — JOBS");
    setWindowModified(document_.dirty());
    save_->setEnabled(p.has_value()); saveAs_->setEnabled(p.has_value());
    add_->setEnabled(p.has_value() && !document_.readOnly());
    table_->setRowCount(p ? int(p->tasks.size()) : 0);
    if (!p) { summary_->setText("새로 만들기(Ctrl+N) 또는 열기(Ctrl+O)로 시작하세요."); statusBar()->showMessage("준비"); return; }
    summary_->setText(qs(p->name) + "  |  시작일 " + qs(p->startDate) + "  |  revision " + QString::number(p->revision)
        + "\n" + (document_.path().empty() ? "아직 저장하지 않은 프로젝트" : qs(document_.path())));
    for (int row = 0; row < int(p->tasks.size()); ++row) {
        const auto& t = p->tasks[std::size_t(row)];
        QStringList values{qs(t.name), "미산정", "미설정", "미설정", qs(t.id)};
        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values[column]);
            if (column != 0 || document_.readOnly()) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            table_->setItem(row, column, item);
        }
    }
    statusBar()->showMessage(document_.readOnly() ? "읽기 전용 · 다른 이름으로 저장하여 편집" :
        document_.dirty() ? "미저장 변경 있음 · 일정 미계산" : "저장된 revision · 일정 미계산");
}
bool MainWindow::confirmChanges() {
    if (!document_.dirty()) return true;
    auto result = QMessageBox::warning(this, "미저장 변경", "현재 프로젝트의 변경 내용을 저장하시겠습니까?",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (result == QMessageBox::Cancel) return false;
    return result != QMessageBox::Save || save(false);
}
bool MainWindow::save(bool as) {
    if (!document_.project()) return false;
    try {
        if (as || document_.path().empty() || document_.readOnly()) {
            QFileDialog dialog(this, "다른 이름으로 저장 · 기존 파일은 덮어쓰지 않습니다");
            dialog.setAcceptMode(QFileDialog::AcceptSave); dialog.setNameFilter("JOBS 프로젝트 (*.jobs)");
            dialog.setDefaultSuffix("jobs"); dialog.selectFile(qs(document_.project()->name) + ".jobs");
            if (dialog.exec() != QDialog::Accepted) return false;
            document_.saveAs(str(dialog.selectedFiles().first()));
        } else document_.save();
        refresh(); statusBar()->showMessage("저장 완료 · 일정 미계산"); return true;
    } catch (const std::exception& e) { report(e); return false; }
}
void MainWindow::create() {
    QDialog dialog(this); dialog.setWindowTitle("새 프로젝트"); dialog.resize(480, 440);
    auto* layout = new QFormLayout(&dialog);
    auto* name = new QLineEdit(&dialog); name->setAccessibleName("프로젝트명");
    auto* date = new QDateEdit(QDate::currentDate(), &dialog); date->setDisplayFormat("yyyy-MM-dd"); date->setCalendarPopup(true);
    date->setAccessibleName("프로젝트 시작일");
    auto* tasks = new QPlainTextEdit(&dialog); tasks->setPlaceholderText("설계\n개발\n검증"); tasks->setAccessibleName("작업명 목록 · 한 줄에 하나");
    auto* error = new QLabel(&dialog); error->setWordWrap(true);
    layout->addRow("프로젝트명(&N)", name); layout->addRow("시작일(&D)", date);
    layout->addRow("기본 근무달력", new QLabel("월–금 · 09–18시 중 휴게 1시간\n1일 8시간 · 휴일 없음", &dialog));
    layout->addRow("작업명 목록(&T)", tasks); layout->addRow(error);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (name->text().trimmed().isEmpty()) { error->setText("프로젝트명을 입력하세요."); name->setFocus(); return; }
        dialog.accept();
    });
    if (dialog.exec() != QDialog::Accepted || !confirmChanges()) return;
    std::vector<std::string> names;
    for (auto line : tasks->toPlainText().split('\n')) if (!line.trimmed().isEmpty()) names.push_back(str(line.trimmed()));
    try { document_.create(str(name->text().trimmed()), str(date->date().toString(Qt::ISODate)), names); refresh(); }
    catch (const std::exception& e) { report(e); }
}
void MainWindow::open() {
    auto path = QFileDialog::getOpenFileName(this, "프로젝트 열기", {}, "JOBS 프로젝트 (*.jobs);;모든 파일 (*)");
    if (path.isEmpty() || !confirmChanges()) return;
    try { document_.open(str(path)); refresh(); }
    catch (const std::exception& e) {
        auto result = QMessageBox::warning(this, "열기 실패", QString::fromUtf8(e.what()) + "\n읽기 전용으로 열기를 시도하시겠습니까?",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (result == QMessageBox::Yes) {
            try { document_.open(str(path), true); refresh(); } catch (const std::exception& inner) { report(inner); }
        }
    }
}
void MainWindow::closeEvent(QCloseEvent* event) { if (confirmChanges()) event->accept(); else event->ignore(); }
}
