#include "formatting/QtDiagnosticText.h"
#include "formatting/PlanTokens.h"
#include "ui/MainWindow.h"
#include "ui/EditorUi.h"
#include "calculation/Effort.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <cmath>

namespace jobs {
namespace {
struct EffortPreview {
    std::optional<std::int64_t> duration, work;
    QString text;
};
EffortPreview previewEffort(const Project &snapshot, const Task &task,
                            const std::shared_ptr<CalculationControl> &control) {
    try {
        EvaluationScope evaluation(control);
        const auto plan = calculatePlan(snapshot);
        if (plan.status != CalculationStatus::Valid)
            return {{}, {}, "최신 계획 계산 불가: 입력/진단을 확인하세요."};
        for (const auto &row : plan.tasks)
            if (row.taskId == task.id) {
                PlanInput input = snapshot;
                input.calculationProfile = effortProfile;
                TaskExecution model(input, task);
                const auto duration = model.duration(row), work = model.work(row);
                calculationCheckpoint();
                return {duration, work,
                        "현재 계획: " + QString::fromStdString(row.start) + " → " + QString::fromStdString(row.finish) +
                            " · 활성 기간 " + QString::number(duration) + "분 · 공수 " +
                            QString::number(double(work) / 6000.0, 'f', 4) + "h"};
            }
        return {{}, {}, "선택 작업이 없습니다."};
    } catch (const CalculationCancelled &) {
        return {{}, {}, "미리보기 계산을 취소했습니다."};
    } catch (const std::exception &error) {
        return {{}, {}, qErrorText(error)};
    }
}
} // namespace
void MainWindow::editTaskEffort() {
    if (!document_.project() || selectedTask_.empty())
        return;
    const auto snapshot = document_.snapshot();
    const auto found = std::find_if(snapshot->tasks.begin(), snapshot->tasks.end(),
                                    [&](const auto &task) { return task.id == selectedTask_; });
    if (found == snapshot->tasks.end())
        return;
    const Task task = *found;
    const auto session = document_.sessionGeneration();
    const bool editable = !document_.readOnly();
    QDialog dialog(this);
    dialog.setObjectName("effortDialog");
    dialog.setWindowTitle("공수·작업 유형·분할 · " + QString::fromStdString(task.name));
    dialog.resize(760, 640);
    auto *layout = new QVBoxLayout(&dialog);
    auto *note =
        new QLabel("100% = 자원 1단위 · 1공수시간 = 자원 1단위 × 1시간 (인력/장비 합산)\n"
                   "고정 기간: 기간 유지, 공수 입력 시 배정률 조정. 고정 공수: 공수 유지, 기간 입력 시 배정률 조정.\n"
                   "고정 단위: 배정률 유지, 기간 입력→공수 계산 / 공수 입력→완료일 계산.\n"
                   "배정률은 정수%입니다. 역산 시 1% 반올림으로 요청 값과 차이가 날 수 있습니다.");
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *form = new QFormLayout;
    layout->addLayout(form);
    auto *type = new QComboBox;
    type->setObjectName("taskType");
    type->addItem("고정 기간", "fixed-duration");
    type->addItem("고정 공수", "fixed-work");
    type->addItem("고정 단위", "fixed-units");
    type->setCurrentIndex(type->findData(QString::fromStdString(enumToken(task.taskType))));
    form->addRow("작업 유형", type);
    auto *basis = new QComboBox;
    basis->setObjectName("effortBasis");
    basis->addItem("기간 입력", "duration");
    basis->addItem("공수 입력", "work");
    basis->setCurrentIndex(usesWork(task) ? 1 : 0);
    form->addRow("이번 변경의 입력 기준", basis);
    auto *duration = new QLineEdit(task.durationMinutes ? QString::number(*task.durationMinutes) : QString{});
    duration->setObjectName("effortDuration");
    duration->setPlaceholderText("근무분 정수 · 미정이면 빈칸");
    form->addRow("기간 (근무분)", duration);
    auto *work = new QLineEdit(task.workUnitMinutes ? QString::number(double(*task.workUnitMinutes) / 6000.0, 'f', 4)
                                                    : QString{});
    work->setObjectName("effortHours");
    work->setPlaceholderText("자원시간 · 최대 소수 4자리");
    form->addRow("공수 (자원시간)", work);
    auto *measured = new QLabel("현재 계획 미리보기 계산 중…");
    measured->setObjectName("effortPreview");
    measured->setWordWrap(true);
    layout->addWidget(measured);
    auto *gaps = new QPlainTextEdit;
    gaps->setObjectName("taskBreaks");
    gaps->setPlaceholderText("2026-10-26 13:00 | 2026-10-28 09:00\n중단 시작 | 재개 시각 (한 줄에 한 구간)");
    QStringList lines;
    for (const auto &pause : task.breaks)
        lines << QString::fromStdString(pause.start) + " | " + QString::fromStdString(pause.finish);
    gaps->setPlainText(lines.join('\n'));
    form->addRow("분할 중단 구간", gaps);
    auto *gapNote = new QLabel(
        "중단은 이 작업의 절대 시각 구간입니다. 날짜순으로 겹치지 않게 입력하세요.\n"
        "빈칸으로 적용하면 분할을 해제합니다. 날짜 이동 후에도 중단 시각은 유지되며 완료 뒤 구간은 효과가 없습니다.");
    gapNote->setWordWrap(true);
    layout->addWidget(gapNote);
    auto *error = new QLabel;
    error->setObjectName("effortError");
    error->setWordWrap(true);
    layout->addWidget(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);
    auto *previewCancel = buttons->addButton("미리보기 취소", QDialogButtonBox::ActionRole);
    previewCancel->setObjectName("cancelEffortPreview");
    bool pending = false, durationChanged = false, workChanged = false, choiceChanged = false, fillingPreview = false;
    auto fields = [&] {
        const bool enabled = editable && !pending, workInput = basis->currentData() == "work";
        const auto kind = taskTypeFromToken(type->currentData().toString().toStdString());
        type->setEnabled(enabled);
        basis->setEnabled(enabled);
        gaps->setReadOnly(!enabled);
        duration->setEnabled(enabled && (!workInput || kind == TaskType::FixedDuration));
        work->setEnabled(enabled && (workInput || kind == TaskType::FixedWork));
        buttons->button(QDialogButtonBox::Ok)->setEnabled(enabled);
    };
    connect(type, &QComboBox::currentIndexChanged, &dialog, [&] {
        choiceChanged = true;
        fields();
    });
    connect(basis, &QComboBox::currentIndexChanged, &dialog, [&] {
        choiceChanged = true;
        fields();
    });
    connect(duration, &QLineEdit::textChanged, &dialog, [&] {
        if (!fillingPreview)
            durationChanged = true;
    });
    connect(work, &QLineEdit::textChanged, &dialog, [&] {
        if (!fillingPreview)
            workChanged = true;
    });
    fields();
    auto previewControl = std::make_shared<CalculationControl>();
    connect(previewCancel, &QPushButton::clicked, &dialog, [previewControl] { previewControl->cancel(); });
    connect(&dialog, &QDialog::finished, &dialog, [previewControl] { previewControl->cancel(); });
    const auto listener = document_.subscribe([this, previewControl, snapshot, session](auto) {
        if (!document_.project() || document_.sessionGeneration() != session ||
            document_.project()->documentId != snapshot->documentId ||
            document_.project()->revision != snapshot->revision)
            previewControl->cancel();
    });
    struct Unsubscribe {
        Document &document;
        std::uint64_t token;
        ~Unsubscribe() {
            document.unsubscribe(token);
        }
    } unsubscribe{document_, listener};
    auto *previewTimer = new QTimer(&dialog);
    previewTimer->setInterval(100);
    connect(previewTimer, &QTimer::timeout, &dialog, [=] {
        measured->setText(previewControl->cancelRequested.load()
                              ? "미리보기 취소 중…"
                              : "현재 계획 계산 중 · " + QString::number(previewControl->completed.load()) + " / " +
                                    QString::number(previewControl->total.load()));
    });
    auto *watcher = new QFutureWatcher<EffortPreview>(&dialog);
    connect(watcher, &QFutureWatcher<EffortPreview>::finished, &dialog, [&, watcher, previewTimer] {
        previewTimer->stop();
        previewCancel->setEnabled(false);
        const auto result = watcher->result();
        watcher->deleteLater();
        if (previewControl->cancelRequested.load()) {
            measured->setText("미리보기 계산을 취소했습니다.");
            return;
        }
        measured->setText(result.text);
        fillingPreview = true;
        if (!choiceChanged && !workChanged && !usesWork(task) && result.work)
            work->setText(QString::number(double(*result.work) / 6000.0, 'f', 4));
        if (!choiceChanged && !durationChanged && usesWork(task) && result.duration)
            duration->setText(QString::number(*result.duration));
        fillingPreview = false;
    });
    previewTimer->start();
    watcher->setFuture(
        QtConcurrent::run([snapshot, task, previewControl] { return previewEffort(*snapshot, task, previewControl); }));
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            if (!document_.project() || document_.project()->documentId != snapshot->documentId ||
                document_.project()->revision != snapshot->revision || document_.sessionGeneration() != session)
                throw AppError(ErrorCode::MAINWINDOWEFFORT_001, {});
            auto minutes = task.durationMinutes, hours = task.workUnitMinutes;
            bool okay = false;
            if (duration->isEnabled()) {
                minutes.reset();
                if (!duration->text().trimmed().isEmpty()) {
                    const auto value = duration->text().trimmed().toLongLong(&okay);
                    if (!okay || value < 0 || value > 5000000)
                        throw AppError(ErrorCode::MAINWINDOWEFFORT_002, {});
                    minutes = value;
                }
            }
            if (work->isEnabled()) {
                hours.reset();
                if (!work->text().trimmed().isEmpty()) {
                    const auto value = work->text().trimmed().toDouble(&okay);
                    if (!okay || !std::isfinite(value) || value < 0 || value > double(maxWork) / 6000.0)
                        throw AppError(ErrorCode::MAINWINDOWEFFORT_003, {});
                    hours = std::llround(value * 6000.0);
                }
            }
            std::vector<TaskBreak> breaks;
            for (const auto &line : gaps->toPlainText().split('\n', Qt::SkipEmptyParts)) {
                const auto parts = line.split('|');
                if (parts.size() != 2)
                    throw AppError(ErrorCode::MAINWINDOWEFFORT_004, {});
                breaks.push_back({parts[0].trimmed().toStdString(), parts[1].trimmed().toStdString()});
            }
            previewControl->cancel();
            submitDialogEdit(dialog, error, buttons,
                             EffortEdit{task.id, taskTypeFromToken(type->currentData().toString().toStdString()),
                                        basis->currentData() == "work", minutes, hours, std::move(breaks)},
                             "공수·기간·배정 역산", [&](bool busy) {
                                 pending = busy;
                                 fields();
                             });
        } catch (const std::exception &exception) {
            error->setText(qErrorText(exception));
        }
    });
    prepareEditorDialog(dialog);
    dialog.exec();
    previewControl->cancel();
    refresh();
    if (editable && document_.project() && document_.sessionGeneration() == session &&
        document_.project()->revision != snapshot->revision)
        calculateSchedule();
}
} // namespace jobs
