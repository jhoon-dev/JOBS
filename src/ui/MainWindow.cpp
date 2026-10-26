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
#include <QFutureWatcher>
#include <QtConcurrent/QtConcurrentRun>
#include <QRegularExpression>
#include <functional>
#include <cmath>
#include <unordered_map>

namespace jobs {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
}
MainWindow::MainWindow(Document& document) : document_(document) {
    resize(1320, 800);
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
    auto* projectMenu = menuBar()->addMenu("프로젝트(&P)");
    calculate_ = projectMenu->addAction("일정 계산(&C)"); calculate_->setShortcut(QKeySequence(Qt::Key_F9));
    tools->addAction(calculate_); connect(calculate_, &QAction::triggered, this, &MainWindow::calculateSchedule);
    calendar_ = projectMenu->addAction("휴일 편집(&H)…"); tools->addAction(calendar_);
    connect(calendar_, &QAction::triggered, this, &MainWindow::editHolidays);
    auto* central = new QWidget(this); auto* layout = new QVBoxLayout(central);
    auto* heading = new QLabel("<h1>JOBS</h1>프로젝트 일정 · 로컬 프로젝트 파일", central);
    layout->addWidget(heading);
    summary_ = new QLabel(central); summary_->setTextFormat(Qt::PlainText); summary_->setWordWrap(true);
    summary_->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(summary_);
    table_ = new QTableWidget(0, 9, central);
    table_->setHorizontalHeaderLabels({"작업명", "기간(h / m)", "FS 선행(행 번호)", "시작", "완료", "총여유(h)", "임계", "자원", "안정 ID"});
    table_->setAccessibleName("프로젝트 작업 목록");
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    for (int column = 1; column < 9; ++column) table_->horizontalHeader()->setSectionResizeMode(column, QHeaderView::ResizeToContents);
    table_->setColumnHidden(8, true); // Stable ID remains on the task tooltip; relations persist IDs, never row numbers.
    table_->setSelectionBehavior(QAbstractItemView::SelectRows); layout->addWidget(table_);
    diagnostics_ = new QPlainTextEdit(central); diagnostics_->setReadOnly(true); diagnostics_->setMaximumHeight(160);
    diagnostics_->setObjectName("calculationDiagnostics"); diagnostics_->setAccessibleName("계산 상태 및 진단");
    layout->addWidget(diagnostics_);
    auto* note = new QLabel("월–금 09–12 / 13–18 · 1일 8시간 · 휴일 편집 가능 · 날짜는 시간대 없는 현지 근무시각\n기간: 시간(8, 1.5) 또는 분(1m), 빈칸=미정, 0=마일스톤 · 선행: 쉼표로 행 번호 입력(예: 1,3), FS 0 lag\nF9: 일정 계산 · 자원 제약·간트·자동복구는 후속 구현입니다.", central);
    note->setWordWrap(true); layout->addWidget(note); setCentralWidget(central);
    connect(table_, &QTableWidget::cellChanged, this, [this](int row, int column) {
        if (column > 2) return;
        try {
            const auto text = table_->item(row, column)->text().trimmed();
            if (column == 0) document_.renameTask(std::size_t(row), str(text));
            else if (column == 1) {
                std::optional<std::int64_t> minutes;
                if (QRegularExpression("^[0-9]+m$").match(text).hasMatch()) {
                    bool ok = false; auto value = text.chopped(1).toULongLong(&ok);
                    if (!ok || value > 5000000) throw std::runtime_error("기간은 5,000,000분 이하여야 합니다.");
                    minutes = static_cast<std::int64_t>(value);
                } else if (!text.isEmpty()) {
                    if (!QRegularExpression("^[0-9]+(?:\\.[0-9]{1,3})?$").match(text).hasMatch())
                        throw std::runtime_error("기간은 음수가 아닌 시간 수로 입력하세요. 예: 8, 1.5 (최소 단위 1분)");
                    bool ok = false; auto value = text.toDouble(&ok) * 60;
                    if (!ok || !std::isfinite(value) || value > 5000000 || std::abs(value - std::round(value)) > 0.000001)
                        throw std::runtime_error("기간은 정수 근무분으로 환산되어야 하며 5,000,000분 이하여야 합니다.");
                    minutes = static_cast<std::int64_t>(std::round(value));
                }
                document_.setDuration(std::size_t(row), minutes);
            } else {
                std::vector<std::string> ids;
                for (const auto& token : text.split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts)) {
                    bool ok = false; auto number = token.toULongLong(&ok);
                    if (!ok || number < 1 || number > document_.project()->tasks.size())
                        throw std::runtime_error("선행 작업은 존재하는 행 번호를 쉼표로 구분해 입력하세요.");
                    ids.push_back(document_.project()->tasks[std::size_t(number - 1)].id);
                }
                document_.setPredecessors(std::size_t(row), std::move(ids));
            }
        }
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
    calculate_->setEnabled(p.has_value() && !calculating_);
    calendar_->setEnabled(p.has_value() && !document_.readOnly());
    table_->setRowCount(p ? int(p->tasks.size()) : 0);
    if (!p) { summary_->setText("새로 만들기(Ctrl+N) 또는 열기(Ctrl+O)로 시작하세요."); diagnostics_->clear(); statusBar()->showMessage("준비"); return; }
    summary_->setText(qs(p->name) + "  |  시작일 " + qs(p->startDate) + "  |  revision " + QString::number(p->revision)
        + "  |  휴일 " + QString::number(p->holidays.size()) + "개"
        + "\n" + (document_.path().empty() ? "아직 저장하지 않은 프로젝트" : qs(document_.path()))
        + (document_.requiresSaveAs() ? " · 이전 형식: 저장 시 새 파일 필요" : ""));
    const auto& calculation = document_.calculation();
    const bool valid = calculation && calculation->status == CalculationStatus::Valid;
    std::unordered_map<std::string, const TaskSchedule*> schedules;
    if (valid) for (const auto& row : calculation->tasks) schedules.emplace(row.taskId, &row);
    std::unordered_map<std::string, int> numbers;
    for (int row = 0; row < int(p->tasks.size()); ++row) numbers.emplace(p->tasks[std::size_t(row)].id, row + 1);
    for (int row = 0; row < int(p->tasks.size()); ++row) {
        const auto& t = p->tasks[std::size_t(row)];
        QStringList predecessors;
        for (const auto& id : t.predecessors) predecessors << (numbers.contains(id) ? QString::number(numbers.at(id)) : "누락:" + qs(id));
        auto found = schedules.find(t.id); const auto* s = found == schedules.end() ? nullptr : found->second;
        const auto duration = !t.durationMinutes ? QString{} : *t.durationMinutes % 3 == 0 ?
            QString::number(double(*t.durationMinutes) / 60, 'g', 12) : QString::number(*t.durationMinutes) + "m";
        QStringList values{qs(t.name), duration,
            predecessors.join(","), s ? qs(s->start) : "미계산", s ? qs(s->finish) : "미계산",
            s ? QString::number(double(s->totalFloatMinutes) / 60, 'g', 12) : "—", s ? (s->critical ? "임계" : "여유") : "—", "미설정", qs(t.id)};
        for (int column = 0; column < values.size(); ++column) {
            auto* item = new QTableWidgetItem(values[column]);
            if (column > 2 || document_.readOnly()) item->setFlags(item->flags() & ~Qt::ItemIsEditable);
            item->setToolTip("작업 ID: " + qs(t.id));
            table_->setItem(row, column, item);
        }
    }
    QString state = "미계산 · F9로 계산";
    if (calculation) {
        switch (calculation->status) {
        case CalculationStatus::Valid: state = "계산 완료 · 프로젝트 완료 " + qs(*calculation->projectFinish); break;
        case CalculationStatus::Incomplete: state = "입력 불완전 · 전체 완료일 없음"; break;
        case CalculationStatus::Conflict: state = "계산 충돌 · 전체 완료일 없음"; break;
        case CalculationStatus::Stale: state = "오래된 계산 · F9로 다시 계산"; break;
        }
        state += "\n입력 revision " + QString::number(calculation->inputRevision) + " · " + qs(calculation->engineVersion) + " · " + qs(calculation->profile);
        if (calculation->status != CalculationStatus::Stale) {
            int count = 0;
            for (const auto& d : calculation->diagnostics) {
                if (++count > 20) { state += "\n추가 진단 " + QString::number(calculation->diagnostics.size() - 20) + "개"; break; }
                state += "\n[" + qs(d.code) + "] " + qs(d.message);
                if (!d.taskIds.empty()) {
                    QStringList path;
                    for (const auto& id : d.taskIds) path << (numbers.contains(id) ? QString::number(numbers.at(id)) : qs(id));
                    state += " (작업 경로: " + path.join(" → ") + ")";
                }
            }
        }
    }
    diagnostics_->setPlainText(calculating_ ? "계산 중 · 편집할 수 있습니다. 변경 시 계산 결과는 오래됨으로 처리됩니다." : state);
    statusBar()->showMessage(QString(document_.readOnly() ? "읽기 전용 · " : document_.dirty() ? "미저장 변경 있음 · " : "저장된 revision · ")
        + (valid ? "기간·FS 논리 일정 · 자원 미검증" : "유효한 최신 일정 없음"));
}
void MainWindow::calculateSchedule() {
    if (!document_.project() || calculating_) return;
    const auto snapshot = *document_.project(); calculating_ = true; refresh();
    auto* watcher = new QFutureWatcher<ScheduleResult>(this);
    connect(watcher, &QFutureWatcher<ScheduleResult>::finished, this, [this, watcher] {
        calculating_ = false;
        try { document_.applyCalculation(watcher->result()); } catch (const std::exception& e) { report(e); }
        watcher->deleteLater(); refresh();
    });
    watcher->setFuture(QtConcurrent::run([snapshot] { return jobs::calculate(snapshot); }));
}
void MainWindow::editHolidays() {
    if (!document_.project() || document_.readOnly()) return;
    QDialog dialog(this); dialog.setWindowTitle("휴일 편집"); dialog.resize(400, 350);
    auto* layout = new QVBoxLayout(&dialog);
    layout->addWidget(new QLabel("하루 전체 휴일 · 한 줄에 YYYY-MM-DD 한 개\n주말은 기본적으로 쉽니다.", &dialog));
    auto* dates = new QPlainTextEdit(&dialog); dates->setAccessibleName("휴일 날짜 목록");
    QStringList lines; for (const auto& d : document_.project()->holidays) lines << qs(d);
    dates->setPlainText(lines.join('\n')); layout->addWidget(dates);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog); layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return;
    std::vector<std::string> holidays;
    for (auto line : dates->toPlainText().split('\n')) if (!line.trimmed().isEmpty()) holidays.push_back(str(line.trimmed()));
    try { document_.setHolidays(std::move(holidays)); refresh(); } catch (const std::exception& e) { report(e); }
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
        if (as || document_.path().empty() || document_.readOnly() || document_.requiresSaveAs()) {
            QFileDialog dialog(this, "다른 이름으로 저장 · 기존 파일은 덮어쓰지 않습니다");
            dialog.setAcceptMode(QFileDialog::AcceptSave); dialog.setNameFilter("JOBS 프로젝트 (*.jobs)");
            dialog.setDefaultSuffix("jobs"); dialog.selectFile(qs(document_.project()->name) + ".jobs");
            if (dialog.exec() != QDialog::Accepted) return false;
            document_.saveAs(str(dialog.selectedFiles().first()));
        } else document_.save();
        refresh(); statusBar()->showMessage("저장 완료"); return true;
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
    layout->addRow("기본 근무달력", new QLabel("월–금 · 09–12 / 13–18\n1일 8시간 · 휴일은 생성 후 편집", &dialog));
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
