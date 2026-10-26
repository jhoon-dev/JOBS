#include "ui/ResourceComparisonDialog.h"
#include "calculation/WorkCalendar.h"
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QDialogButtonBox>
#include <QFutureWatcher>
#include <QDateTime>
#include <QSignalBlocker>
#include <QColor>
#include <QtConcurrent/QtConcurrentRun>
#include <unordered_map>
#include <set>
#include <map>
#include <algorithm>
namespace jobs {
namespace {
QString qs(const std::string& s) { return QString::fromUtf8(s.data(), qsizetype(s.size())); }
std::string str(const QString& s) { return s.toUtf8().toStdString(); }
QString hours(std::int64_t n) { return QString::number(double(n) / 60, 'f', 2); }
QString work(std::int64_t n) { return QString::number(double(n) / 6000, 'f', 2); }
QString state(CalculationStatus s) {
    return s == CalculationStatus::Valid ? "배정 자원 검증 통과" : s == CalculationStatus::Incomplete ? "미정 / 입력 불완전" : "과부하 / 배치 불가";
}
QTableWidget* table(QWidget* parent, const char* name, const QStringList& columns) {
    auto* t = new QTableWidget(0, columns.size(), parent); t->setObjectName(name); t->setHorizontalHeaderLabels(columns);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers); t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents); return t;
}
void fillLoad(QTableWidget* t, const Project& p, const ResourceLoad& load) {
    std::unordered_map<std::string, QString> resources, tasks;
    for (const auto& r : p.resources) resources.emplace(r.id, qs(r.name));
    for (const auto& r : p.tasks) tasks.emplace(r.id, qs(r.name));
    t->setRowCount(int(load.days.size()));
    for (int i = 0; i < t->rowCount(); ++i) {
        const auto& d = load.days[std::size_t(i)]; QStringList assigned;
        for (const auto& id : d.taskIds) assigned << tasks.at(id);
        QString ratio = !d.capacity ? "미정" : *d.capacity == 0 ? "가용 0%" : QString::number(double(d.peakUnits) / *d.capacity * 100, 'f', 1) + "%";
        QStringList values{resources.at(d.resourceId), qs(d.date), d.capacity ? QString::number(*d.capacity) + "%" : "미정",
            work(d.demandUnitMinutes), QString::number(d.peakUnits) + "%", ratio, d.capacity ? hours(d.overloadMinutes) : "미정", d.capacity ? work(d.excessUnitMinutes) : "미정", assigned.join(", ")};
        for (int c = 0; c < values.size(); ++c) {
            auto* item = new QTableWidgetItem(values[c]); item->setData(Qt::UserRole, qs(d.resourceId)); item->setToolTip("자원 ID: " + qs(d.resourceId));
            if (!d.capacity) { item->setBackground(QColor("#fff2c6")); item->setForeground(QColor("#4d3a00")); }
            else if (d.overloadMinutes) { item->setBackground(QColor("#ffe0e0")); item->setForeground(QColor("#6f1720")); }
            t->setItem(i, c, item);
        }
    }
}
std::int64_t overload(const ResourceLoad& l) { std::int64_t n = 0; for (const auto& d : l.days) n += d.overloadMinutes; return n; }
QString overloadText(const ResourceLoad& l) {
    return std::any_of(l.days.begin(), l.days.end(), [](const auto& d) { return !d.capacity; }) ? "미정 (알려진 구간 " + hours(overload(l)) + "h)" : hours(overload(l)) + "h";
}
struct Computed {
    LevelingResult main;
    std::optional<LevelingResult> other;
    std::optional<Project> otherProject;
    QString error;
};
}
ResourceComparisonDialog::ResourceComparisonDialog(Document& doc, QWidget* parent) : QDialog(parent), document_(doc) {
    setObjectName("resourceComparisonDialog"); setWindowTitle("기간별 자원 부하 · 평준화 시나리오 비교"); resize(1480, 850);
    auto* layout = new QVBoxLayout(this); auto* sources = new QHBoxLayout; layout->addLayout(sources);
    sourceChoice_ = new QComboBox(this); sourceChoice_->setObjectName("levelingSource"); sourceChoice_->addItem("현재 입력 → 새 시나리오 A", QString{});
    otherChoice_ = new QComboBox(this); otherChoice_->setObjectName("levelingOther"); otherChoice_->addItem("B 비교 없음", QString{});
    for (const auto& s : doc.project()->scenarios) {
        auto label = qs(s.name) + " · rev " + QString::number(s.inputRevision) + " · " + qs(s.createdAt);
        sourceChoice_->addItem(label, qs(s.id)); otherChoice_->addItem(label, qs(s.id));
    }
    sources->addWidget(new QLabel("A 입력", this)); sources->addWidget(sourceChoice_, 1); sources->addWidget(new QLabel("저장된 B", this)); sources->addWidget(otherChoice_, 1);
    auto* controls = new QHBoxLayout; layout->addLayout(controls);
    priority_ = new QComboBox(this); priority_->setObjectName("levelingPriority"); priority_->addItem("작업 행 순서", "task-order"); priority_->addItem("짧은 잔여 기간 우선", "shortest-first"); priority_->addItem("긴 잔여 기간 우선", "longest-first");
    horizon_ = new QSpinBox(this); horizon_->setObjectName("levelingHorizon"); horizon_->setRange(1, 5200); horizon_->setValue(260); horizon_->setSuffix(" 근무일");
    controls->addWidget(new QLabel("준비된 작업 중 우선순위", this)); controls->addWidget(priority_); controls->addWidget(new QLabel("계획 시작부터 탐색 범위", this)); controls->addWidget(horizon_);
    auto* run = new QPushButton("부하 계산 / 평준화 비교", this); run->setObjectName("runLeveling"); controls->addWidget(run); controls->addStretch();
    summary_ = new QLabel(this); summary_->setObjectName("levelingSummary"); summary_->setTextFormat(Qt::PlainText); summary_->setWordWrap(true); summary_->setMinimumHeight(125); summary_->setTextInteractionFlags(Qt::TextSelectableByMouse); layout->addWidget(summary_);
    auto* tabs = new QTabWidget(this); tabs->setObjectName("resourceTabs"); layout->addWidget(tabs, 1);
    tasks_ = table(this, "levelingTasks", {"작업", "원본 시작", "원본 완료", "A 시작", "A 완료", "잔여 시작 지연(h)", "B 시작", "B 완료", "실제 날짜"}); tabs->addTab(tasks_, "일정 비교");
    const QStringList columns{"자원", "날짜", "가용량", "수요 공수(h)", "최대 배정률", "최대 사용률", "과부하 시간(h)", "초과 공수(h)", "작업"};
    before_ = table(this, "loadBefore", columns); tabs->addTab(before_, "A 원본 부하");
    after_ = table(this, "loadAfter", columns); tabs->addTab(after_, "A 평준화 후");
    other_ = table(this, "loadOther", columns); tabs->addTab(other_, "B 평준화 후");
    diagnostics_ = new QPlainTextEdit(this); diagnostics_->setObjectName("levelingDiagnostics"); diagnostics_->setReadOnly(true); diagnostics_->setMaximumHeight(110); layout->addWidget(diagnostics_);
    auto* note = new QLabel("전체 작업 범위 · 정률/기간 고정 · 작업 분할 없음 · 여유 초과 지연 허용 · 원본보다 앞당기지 않음 · 같은 우선순위는 행 순서\n상태일 설정 시 잔여 수요만 비교하고 실제 날짜는 고정합니다. 공수는 인시/장비시이며 서로 합산하지 않습니다. 부하 표는 수요가 있는 날짜만 표시합니다. 평준화 결과의 CPM 여유·임계는 재산정하지 않습니다.", this); note->setWordWrap(true); layout->addWidget(note);
    auto* saveRow = new QHBoxLayout; layout->addLayout(saveRow);
    name_ = new QLineEdit(this); name_->setObjectName("scenarioName"); name_->setPlaceholderText("보존할 시나리오 이름");
    save_ = new QPushButton("A 입력·정책을 새 시나리오로 보존", this); save_->setObjectName("saveScenario"); save_->setEnabled(false);
    saveRow->addWidget(name_, 1); saveRow->addWidget(save_);
    auto* close = new QDialogButtonBox(QDialogButtonBox::Close, this); layout->addWidget(close); connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(run, &QPushButton::clicked, this, [this] { recalculate(); });
    connect(priority_, &QComboBox::currentIndexChanged, this, [this] { invalidate(); }); connect(horizon_, &QSpinBox::valueChanged, this, [this] { invalidate(); });
    connect(sourceChoice_, &QComboBox::currentIndexChanged, this, [this] { recalculate(); }); connect(otherChoice_, &QComboBox::currentIndexChanged, this, [this] { recalculate(); });
    connect(save_, &QPushButton::clicked, this, [this] { saveScenario(); }); recalculate();
}
void ResourceComparisonDialog::invalidate() {
    ++generation_; result_.reset(); source_.reset(); save_->setEnabled(false); tasks_->setRowCount(0); before_->setRowCount(0); after_->setRowCount(0); other_->setRowCount(0); diagnostics_->clear(); summary_->setText("조건을 변경했습니다. 부하 계산 / 평준화 비교를 실행하세요.");
}
void ResourceComparisonDialog::recalculate() {
    invalidate(); const auto generation = generation_;
    const Project current = *document_.project(); Project snapshot = current;
    const auto selected = str(sourceChoice_->currentData().toString()), otherId = str(otherChoice_->currentData().toString());
    std::optional<LevelingScenario> secondary;
    for (const auto& s : current.scenarios) {
        if (s.id == selected) {
            snapshot = scenarioProject(s); QSignalBlocker b1(priority_), b2(horizon_);
            priority_->setCurrentIndex(priority_->findData(qs(s.priority))); horizon_->setValue(s.horizonWorkDays);
        }
        if (s.id == otherId) secondary = s;
    }
    priority_->setEnabled(selected.empty()); horizon_->setEnabled(selected.empty());
    auto priority = str(priority_->currentData().toString()); int horizon = horizon_->value();
    summary_->setText("자원 수요·시나리오 계산 중…");
    auto* watcher = new QFutureWatcher<Computed>(this);
    connect(watcher, &QFutureWatcher<Computed>::finished, this, [this, watcher, generation, snapshot, current, selected] {
        auto computed = watcher->result(); watcher->deleteLater(); if (generation != generation_) return;
        if (!computed.error.isEmpty()) { summary_->setText(computed.error); return; }
        if (!document_.project() || document_.project()->documentId != current.documentId || document_.project()->revision != current.revision) { summary_->setText("계산 중 문서가 변경되었습니다. 다시 계산하세요."); return; }
        source_ = snapshot; result_ = computed.main; const auto& r = *result_;
        save_->setEnabled(selected.empty() && !document_.readOnly());
        auto finish = [](const ScheduleResult& s) { return s.projectFinish ? qs(*s.projectFinish) : "—"; };
        auto units = [&](const ResourceLoad& l) {
            std::map<std::string, std::int64_t> totals;
            for (const auto& d : l.days) for (const auto& resource : snapshot.resources) if (resource.id == d.resourceId) totals[resource.type] += d.demandUnitMinutes;
            return "인력 " + work(totals["labor"]) + " 인시 · 장비 " + work(totals["equipment"]) + " 장비시";
        };
        QString summary = "A 원본 rev " + QString::number(snapshot.revision) + " · " + qs(snapshot.documentId) + " · " + (snapshot.statusAt ? "잔여 예측 / 상태일 " + qs(*snapshot.statusAt) : "계획")
            + "\n완료: 원본 " + finish(r.original) + " → A " + finish(r.leveled) + " · " + state(r.leveled.status == CalculationStatus::Valid ? r.after.status : r.leveled.status)
            + "\n배정 작업 " + QString::number(r.before.assignedTasks) + "/" + QString::number(r.before.totalTasks) + " · 자원별 과부하 시간 합계 " + overloadText(r.before) + " → " + (r.leveled.status == CalculationStatus::Valid ? overloadText(r.after) : "—")
            + " · 수요 " + units(r.before) + (r.leveled.status == CalculationStatus::Valid ? " → " + units(r.after) : "")
            + "\n정책 " + qs(r.leveled.calculationPolicy) + " · 탐색 " + QString::number(r.horizonWorkDays) + " 기본 달력 근무일 · 원본 자동 적용 없음";
        if (computed.other && computed.otherProject) {
            summary += "\nB 원본 rev " + QString::number(computed.otherProject->revision) + " · " + qs(computed.other->priority) + " / " + QString::number(computed.other->horizonWorkDays) + " 근무일 · 완료 " + finish(computed.other->leveled) + " · " + state(computed.other->leveled.status == CalculationStatus::Valid ? computed.other->after.status : computed.other->leveled.status);
            if (static_cast<const PlanInput&>(snapshot) != static_cast<const PlanInput&>(*computed.otherProject) || snapshot.statusAt != computed.otherProject->statusAt || snapshot.progress != computed.otherProject->progress)
                summary += " · A/B 입력이 다릅니다: 우선순위만의 효과로 해석할 수 없습니다.";
        }
        summary_->setText(summary);
        fillLoad(before_, snapshot, r.before); fillLoad(after_, snapshot, r.after);
        if (computed.otherProject && computed.other) fillLoad(other_, *computed.otherProject, computed.other->after);
        std::unordered_map<std::string, const TaskSchedule*> original, leveled, other;
        for (const auto& row : r.original.tasks) original.emplace(row.taskId, &row);
        for (const auto& row : r.leveled.tasks) leveled.emplace(row.taskId, &row);
        if (computed.other) for (const auto& row : computed.other->leveled.tasks) other.emplace(row.taskId, &row);
        std::vector<Task> all = snapshot.tasks;
        if (computed.otherProject) for (const auto& t : computed.otherProject->tasks) if (std::none_of(all.begin(), all.end(), [&](const auto& a) { return a.id == t.id; })) all.push_back(t);
        std::map<std::string,WorkCalendar> taskCalendars; tasks_->setRowCount(int(all.size()));
        for (int i = 0; i < tasks_->rowCount(); ++i) {
            const auto& t = all[std::size_t(i)]; const auto* a = original.contains(t.id) ? original.at(t.id) : nullptr;
            const auto* b = leveled.contains(t.id) ? leveled.at(t.id) : nullptr; const auto* c = other.contains(t.id) ? other.at(t.id) : nullptr;
            QString delay = "—";
            if (a && b) {
                if(!taskCalendars.contains(t.calendarId)) taskCalendars.emplace(t.calendarId,WorkCalendar(snapshot,t));
                const auto& calendar=taskCalendars.at(t.calendarId);
                delay = hours(calendar.offset(b->remainingStart.value_or(b->start)) - calendar.offset(a->remainingStart.value_or(a->start)));
            }
            const auto* actual = progressAsOf(snapshot, t.id);
            QStringList values{qs(t.name), a ? qs(a->start) : "—", a ? qs(a->finish) : "—", b ? qs(b->start) : "—", b ? qs(b->finish) : "—", delay, c ? qs(c->start) : "—", c ? qs(c->finish) : "—", actual && actual->actualStart ? "고정 보존" : "—"};
            for (int col = 0; col < values.size(); ++col) { auto* item = new QTableWidgetItem(values[col]); item->setData(Qt::UserRole, qs(t.id)); item->setToolTip(qs(t.id)); tasks_->setItem(i, col, item); }
        }
        QStringList messages;
        auto append = [&](const QString& label, const std::vector<Diagnostic>& ds) { for (const auto& d : ds) { QStringList ids; for (const auto& id : d.taskIds) ids << qs(id); messages << label + " [" + qs(d.code) + "] " + qs(d.message) + (ids.empty() ? "" : " · " + ids.join(", ")); } };
        append("A 원본 일정", r.original.diagnostics); append("A 원본 부하", r.before.diagnostics); append("A 평준화", r.diagnostics); append("A 결과", r.after.diagnostics);
        if (computed.other) append("B 평준화", computed.other->diagnostics);
        diagnostics_->setPlainText(messages.join('\n'));
    });
    watcher->setFuture(QtConcurrent::run([snapshot, priority, horizon, secondary] {
        Computed c;
        try { c.main = levelResources(snapshot, priority, horizon); if (secondary) { c.otherProject = scenarioProject(*secondary); c.other = levelResources(*c.otherProject, secondary->priority, secondary->horizonWorkDays); } }
        catch (const std::exception& e) { c.error = QString::fromUtf8(e.what()); }
        return c;
    }));
}
void ResourceComparisonDialog::saveScenario() {
    if (!source_ || !result_ || !sourceChoice_->currentData().toString().isEmpty()) return;
    try {
        auto id = document_.saveScenario(str(name_->text().trimmed()), result_->priority, result_->horizonWorkDays,
            QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ssZ").toStdString(), source_->documentId, source_->revision);
        const auto& s = document_.project()->scenarios.back(); auto label = qs(s.name) + " · rev " + QString::number(s.inputRevision) + " · " + qs(s.createdAt);
        { QSignalBlocker b(sourceChoice_); sourceChoice_->addItem(label, qs(id)); otherChoice_->addItem(label, qs(id)); sourceChoice_->setCurrentIndex(sourceChoice_->count() - 1); }
        recalculate();
    } catch (const std::exception& e) { diagnostics_->setPlainText(QString::fromUtf8(e.what())); }
}
}
