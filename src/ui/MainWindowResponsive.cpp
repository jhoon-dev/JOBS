#include "ui/MainWindow.h"
#include "ui/TaskTableModel.h"
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QStatusBar>
#include <QTreeWidget>

namespace jobs {
namespace { QString qs(const std::string& value) { return QString::fromUtf8(value); } }
MainWindow::ViewKey MainWindow::viewKey() const {
    return {selectedWbs_, selectedBaseline_, taskSearch_->text(), taskFilter_->currentIndex(), taskSort_->currentIndex(),
            document_.readOnly(), baselinePreparing_, calculating_, document_.path()};
}
bool MainWindow::refreshIncremental() {
    const auto& project = document_.project();
    if (!rendered_ || !project || renderedSession_ != document_.sessionGeneration() || renderedView_ != viewKey() ||
        renderedCalculation_ != document_.calculationGeneration()) return false;
    if (renderedRevision_ == project->revision) { refreshStatus(); return true; }
    const auto& hint = document_.taskChange();
    if (!hint || hint->before != renderedRevision_ || hint->after != project->revision ||
        !taskSearch_->text().trimmed().isEmpty() || taskFilter_->currentIndex() || taskSort_->currentIndex()) return false;
    const auto previous = taskModel_->cachedTask(hint->row);
    const bool visible = taskModel_->containsSourceRow(hint->row);
    if (!taskModel_->synchronizeTask()) return false;
    // A valid result becomes stale on editing, so this path only adjusts the
    // known input-duration total. A calculation-generation change takes full refresh.
    if (visible && !hint->nameOnly) {
        const auto contribution = [&](const Task& task) {
            const bool missing = (isEffort(*project) && usesWork(task)) || !task.durationMinutes;
            return std::pair{missing ? std::int64_t{0} : *task.durationMinutes, std::size_t(missing)};
        };
        const auto before = contribution(previous), after = contribution(project->tasks[hint->row]);
        renderedRollup_.knownDurationMinutes += after.first - before.first;
        renderedRollup_.missingDurations -= before.second;
        renderedRollup_.missingDurations += after.second;
        refreshWbsSummary();
    }
    renderedRevision_ = project->revision;
    refreshDocumentSummary(); refreshStatus();
    return true;
}
void MainWindow::refreshDocumentSummary() {
    const auto& p = document_.project();
    if (!p) return;
    summary_->setText(qs(p->name) + "  |  시작일 " + qs(p->startDate) + "  |  revision " + QString::number(p->revision)
        + "  |  휴일 " + QString::number(p->holidays.size()) + "개"
        + "  |  상태일 " + (p->statusAt ? qs(*p->statusAt) : "미설정 · 현재 계획 모드")
        + "  |  기준선 " + QString::number(p->baselines.size()) + "개 · 실적 이력 " + QString::number(p->progress.size()) + "건"
        + "\n" + (document_.path().empty() ? "아직 저장하지 않은 프로젝트" : qs(document_.path()))
        + (document_.recoveryOrigin() ? " · 복구 사본: 새 파일에 저장 필요" : document_.requiresSaveAs() ? " · 이전 형식: 저장 시 새 파일 필요" : "")
        + (document_.recoveryOrigin() ? "\n복구 시각 UTC " + qs(document_.recoveryOrigin()->capturedAt) + " · 원본 r" + QString::number(document_.recoveryOrigin()->revision) + " · " + (document_.recoveryOrigin()->path.empty() ? "처음 저장 전" : qs(document_.recoveryOrigin()->path)) : "")
        + (document_.historyNotice().empty() ? QString{} : "\n편집 이력: " + qs(document_.historyNotice())));
}
void MainWindow::refreshWbsSummary() {
    if (!wbsTree_->currentItem()) return;
    const auto& rollup = renderedRollup_;
    wbsSummary_->setText("선택 범위: " + wbsTree_->currentItem()->text(0) + " · 작업 " + QString::number(rollup.taskCount)
        + "개 · 알려진 작업 기간 합계 " + QString::number(double(rollup.knownDurationMinutes) / 60, 'g', 10) + "h"
        + " · 기간 미정 " + QString::number(rollup.missingDurations) + "개 (합계는 WBS 경과기간과 다름)"
        + (rollup.start ? "\n계산 구간: " + qs(*rollup.start) + " → " + qs(*rollup.finish) : "\n계산 구간: 최신 유효한 결과 없음"));
}
void MainWindow::refreshStatus() {
    const auto& result = document_.calculation(); const bool valid = result && result->status == CalculationStatus::Valid;
    statusBar()->showMessage(QString(document_.readOnly() ? "읽기 전용 · " : document_.dirty() ? "미저장 변경 있음 · " : "명시 저장본과 같은 입력 · ")
        + (valid ? (result->isForecast ? "실적·잔여 예측 · 자원 미검증" : "선행관계·작업 달력 일정 · 자원 미검증") : "유효한 최신 일정 없음"));
}
}
