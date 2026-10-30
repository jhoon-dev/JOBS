#include "ui/WorkspaceDialog.h"
#include "ui/DesktopWorkspace.h"
#include "formatting/QtDiagnosticText.h"
#include <QAbstractTableModel>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFutureWatcher>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableView>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>
#include <algorithm>
#include <map>

namespace jobs {
namespace {
QString qs(const std::string &value) {
    return QString::fromStdString(value);
}
std::string str(const QString &value) {
    return value.toUtf8().toStdString();
}
QString state(CalculationStatus value) {
    switch (value) {
    case CalculationStatus::Valid:
        return "정상";
    case CalculationStatus::Conflict:
        return "충돌/과부하";
    case CalculationStatus::Incomplete:
        return "미완성/부분 범위";
    case CalculationStatus::Stale:
        return "변경됨";
    case CalculationStatus::Cancelled:
        return "취소";
    }
    return {};
}
QTableWidget *table(QStringList headers, QWidget *parent) {
    auto *result = new QTableWidget(0, headers.size(), parent);
    result->setHorizontalHeaderLabels(headers);
    result->setSelectionBehavior(QAbstractItemView::SelectRows);
    result->setSelectionMode(QAbstractItemView::SingleSelection);
    result->setEditTriggers(QAbstractItemView::NoEditTriggers);
    result->horizontalHeader()->setStretchLastSection(true);
    return result;
}
void row(QTableWidget *target, int index, const QStringList &values) {
    for (int column = 0; column < values.size(); ++column) {
        auto *item = new QTableWidgetItem(values[column]);
        item->setToolTip(values[column]);
        target->setItem(index, column, item);
    }
}
int integer(const QString &text, int minimum, int maximum) {
    bool ok = false;
    auto value = text.trimmed().toInt(&ok);
    if (!ok || value < minimum || value > maximum)
        throw AppError(ErrorCode::WORKSPACE_INVALID, {"integer"});
    return value;
}
std::optional<int> capacity(const QString &text) {
    return text.trimmed() == "?" ? std::optional<int>{} : std::optional<int>{integer(text, 0, maxSharedResourceUnits)};
}
int minute(const QString &text, bool finish) {
    auto parts = text.trimmed().split(':');
    if (parts.size() != 2)
        throw AppError(ErrorCode::CALENDAR_PERIODS_INVALID);
    auto hour = integer(parts[0], 0, finish ? 24 : 23), minutes = integer(parts[1], 0, 59);
    if (hour == 24 && minutes)
        throw AppError(ErrorCode::CALENDAR_PERIODS_INVALID);
    return hour * 60 + minutes;
}
QString timeText(int value) {
    return QString("%1:%2").arg(value / 60, 2, 10, QChar('0')).arg(value % 60, 2, 10, QChar('0'));
}
} // namespace
class SharedLoadModel final : public QAbstractTableModel {
public:
    explicit SharedLoadModel(QObject *parent) : QAbstractTableModel(parent) {}
    std::shared_ptr<const SharedResourceResult> replace(std::shared_ptr<const SharedResourceResult> result,
                                                        const Workspace &config) {
        beginResetModel();
        auto old = std::move(result_);
        result_ = std::move(result);
        names_.clear();
        for (const auto &resource : config.resources)
            names_[resource.id] = qs(resource.name);
        endResetModel();
        return old;
    }
    int rowCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() || !result_ ? 0 : int(result_->days.size());
    }
    int columnCount(const QModelIndex &parent = {}) const override {
        return parent.isValid() ? 0 : 9;
    }
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override {
        if (role != Qt::DisplayRole)
            return {};
        if (orientation == Qt::Vertical)
            return section + 1;
        const QStringList labels{"공유 자원", "날짜",         "기본 가용(%)", "최대 수요(%)", "수요(공수h)",
                                 "과부하(h)", "초과 공수(h)", "가용 미정(h)", "관련 작업"};
        return section >= 0 && section < labels.size() ? QVariant(labels[section]) : QVariant{};
    }
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (!result_ || !index.isValid() || index.row() < 0 || index.row() >= rowCount())
            return {};
        if (role != Qt::DisplayRole && role != Qt::ToolTipRole)
            return {};
        const auto &day = result_->days[std::size_t(index.row())];
        switch (index.column()) {
        case 0: {
            const auto found = names_.find(day.sharedId);
            return found == names_.end() ? qs(day.sharedId) : found->second;
        }
        case 1:
            return qs(day.date);
        case 2:
            return day.capacity ? QString::number(*day.capacity) : QString("미정");
        case 3:
            return QString::number(day.peakUnits);
        case 4:
            return QString::number(double(day.demandUnitMinutes) / 6000.0, 'f', 2);
        case 5:
            return QString::number(double(day.overloadMinutes) / 60.0, 'f', 2);
        case 6:
            return QString::number(double(day.excessUnitMinutes) / 6000.0, 'f', 2);
        case 7:
            return QString::number(double(day.unknownCapacityMinutes) / 60.0, 'f', 2);
        case 8:
            return QString::number(day.tasks.size()) + "개 작업";
        default:
            return {};
        }
    }

private:
    std::shared_ptr<const SharedResourceResult> result_;
    std::map<std::string, QString> names_;
};
WorkspaceDialog::WorkspaceDialog(DesktopWorkspace &workspace, QWidget *parent)
    : QDialog(parent), workspace_(workspace) {
    setObjectName("workspaceDialog");
    setWindowTitle("다중 프로젝트 · 공유 자원");
    resize(1200, 720);
    auto *layout = new QVBoxLayout(this);
    auto *files = new QHBoxLayout;
    layout->addLayout(files);
    const auto button = [&](const char *text, const char *name, QHBoxLayout *bar) {
        auto *value = new QPushButton(text);
        value->setObjectName(name);
        bar->addWidget(value);
        return value;
    };
    auto *create = button("새 프로젝트 창", "workspaceNewProject", files);
    auto *open = button("프로젝트 추가…", "workspaceAddProject", files);
    auto *openWorkspace = button("작업공간 열기…", "workspaceOpen", files);
    auto *save = button("작업공간 저장…", "workspaceSave", files);
    auto *saveAs = button("작업공간 사본…", "workspaceSaveAs", files);
    path_ = new QLabel;
    path_->setTextFormat(Qt::PlainText);
    path_->setWordWrap(true);
    layout->addWidget(path_);
    auto *note =
        new QLabel("프로젝트는 독립 파일/창입니다. 공유 자원은 이름 대신 명시적으로 연결합니다. 닫힌 프로젝트는 다시 "
                   "열어야 전체 범위를 검증할 수 있습니다.\n기본 가용은 근무 구간의 용량이며 공유 달력의 휴무 시간은 "
                   "0입니다. 같은 날짜여도 시간이 겹치지 않으면 수요를 합산하지 않습니다.");
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *tabs = new QTabWidget;
    layout->addWidget(tabs, 1);
    auto *projectPage = new QWidget;
    auto *projectLayout = new QVBoxLayout(projectPage);
    projects_ =
        table({"프로젝트", "창 상태", "입력 revision", "시작일", "계산 상태", "완료일", "저장 파일"}, projectPage);
    projects_->setObjectName("workspaceProjects");
    projectLayout->addWidget(projects_);
    auto *projectActions = new QHBoxLayout;
    projectLayout->addLayout(projectActions);
    auto *activate = button("선택 프로젝트 열기/전환", "workspaceActivate", projectActions);
    auto *remove = button("닫힌 프로젝트 제외", "workspaceRemoveProject", projectActions);
    tabs->addTab(projectPage, "프로젝트");
    auto *resourcePage = new QWidget;
    auto *resourceLayout = new QVBoxLayout(resourcePage);
    resources_ = table({"공유 자원", "종류", "기본 가용(%)", "연결 수"}, resourcePage);
    resources_->setObjectName("workspaceResources");
    resourceLayout->addWidget(resources_);
    auto *resourceActions = new QHBoxLayout;
    resourceLayout->addLayout(resourceActions);
    auto *add = button("공유 자원 추가", "workspaceAddResource", resourceActions);
    auto *edit = button("용량·달력 편집", "workspaceEditResource", resourceActions);
    auto *deleteResource = button("공유 자원 삭제", "workspaceDeleteResource", resourceActions);
    bindings_ = table({"프로젝트", "로컬 자원 ID", "공유 자원"}, resourcePage);
    bindings_->setObjectName("workspaceBindings");
    resourceLayout->addWidget(bindings_);
    auto *bindingActions = new QHBoxLayout;
    resourceLayout->addLayout(bindingActions);
    auto *bind = button("자원 연결/변경", "workspaceBind", bindingActions);
    auto *unbind = button("선택 연결 해제", "workspaceUnbind", bindingActions);
    tabs->addTab(resourcePage, "공유 자원·연결");
    auto *loadPage = new QWidget;
    auto *loadLayout = new QVBoxLayout(loadPage);
    auto *calculation = new QHBoxLayout;
    loadLayout->addLayout(calculation);
    mode_ = new QComboBox;
    mode_->setObjectName("workspaceMode");
    mode_->addItems({"현재 예측 · 상태일 이후 잔여", "계획 일정"});
    calculation->addWidget(mode_);
    analyze_ = button("통합 부하 계산", "workspaceAnalyze", calculation);
    cancel_ = button("계산 취소", "workspaceCancel", calculation);
    cancel_->setEnabled(false);
    load_ = new QTableView;
    load_->setObjectName("workspaceLoad");
    loadModel_ = new SharedLoadModel(this);
    load_->setModel(loadModel_);
    load_->setSelectionBehavior(QAbstractItemView::SelectRows);
    load_->setSelectionMode(QAbstractItemView::SingleSelection);
    load_->horizontalHeader()->setStretchLastSection(true);
    loadLayout->addWidget(load_);
    auto *navigation = new QHBoxLayout;
    loadLayout->addLayout(navigation);
    task_ = new QComboBox;
    task_->setObjectName("workspaceRelatedTask");
    navigation->addWidget(task_, 1);
    navigate_ = button("원본 작업으로 이동", "workspaceNavigate", navigation);
    navigate_->setEnabled(false);
    tabs->addTab(loadPage, "기간별 통합 부하");
    status_ = new QLabel("통합 부하를 계산하세요.");
    status_->setTextFormat(Qt::PlainText);
    status_->setWordWrap(true);
    status_->setObjectName("workspaceStatus");
    layout->addWidget(status_);
    diagnosticDetails_ = new QPlainTextEdit;
    diagnosticDetails_->setReadOnly(true);
    diagnosticDetails_->setMaximumHeight(160);
    diagnosticDetails_->setObjectName("workspaceDiagnosticDetails");
    layout->addWidget(diagnosticDetails_);
    auto *close = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(close);
    connect(close, &QDialogButtonBox::rejected, this, &QDialog::reject);
    const auto protect = [this](const std::function<void()> &action) {
        try {
            action();
        } catch (const std::exception &error) {
            QMessageBox::warning(this, "작업공간", qErrorText(error));
        }
    };
    connect(create, &QPushButton::clicked, this, [this] { workspace_.createProject(); });
    connect(open, &QPushButton::clicked, this, [this, protect] {
        auto path = QFileDialog::getOpenFileName(this, "프로젝트 추가", {}, "JOBS 프로젝트 (*.jobs)");
        if (!path.isEmpty())
            protect([&] { workspace_.openProject(str(path)); });
    });
    connect(openWorkspace, &QPushButton::clicked, this, [this, protect] {
        auto path = QFileDialog::getOpenFileName(this, "작업공간 열기", {}, "JOBS 작업공간 (*.jobsworkspace)");
        if (!path.isEmpty())
            protect([&] { workspace_.openWorkspace(str(path), this); });
    });
    connect(save, &QPushButton::clicked, this, [this] { workspace_.saveWorkspace(this); });
    connect(saveAs, &QPushButton::clicked, this, [this] { workspace_.saveWorkspace(this, true); });
    connect(activate, &QPushButton::clicked, this, [this, protect] {
        const auto index = projects_->currentRow();
        const auto config = workspace_.configuration();
        if (index < 0 || index >= int(config.projects.size()))
            return;
        const auto ref = config.projects[std::size_t(index)];
        protect([&] {
            if (!workspace_.activate(ref.documentId))
                workspace_.openProject(ref.path);
        });
    });
    connect(remove, &QPushButton::clicked, this, [this, protect] {
        const auto index = projects_->currentRow();
        if (index >= 0 && index < int(workspace_.configuration().projects.size()))
            protect([&] {
                workspace_.removeClosedProject(workspace_.configuration().projects[std::size_t(index)].documentId);
            });
    });
    connect(add, &QPushButton::clicked, this, [this] { editResource(true); });
    connect(edit, &QPushButton::clicked, this, [this] { editResource(false); });
    connect(bind, &QPushButton::clicked, this, [this] { bindResource(); });
    connect(deleteResource, &QPushButton::clicked, this, [this, protect] {
        auto config = workspace_.configuration();
        const auto index = resources_->currentRow();
        if (index < 0 || index >= int(config.resources.size()))
            return;
        const auto id = config.resources[std::size_t(index)].id;
        if (QMessageBox::question(this, "공유 자원 삭제",
                                  "공유 자원과 연결을 제거하시겠습니까? 프로젝트 원본은 변경하지 않습니다.") !=
            QMessageBox::Yes)
            return;
        std::erase_if(config.resources, [&](const auto &value) { return value.id == id; });
        std::erase_if(config.bindings, [&](const auto &value) { return value.sharedId == id; });
        protect([&] { workspace_.applyConfiguration(std::move(config)); });
    });
    connect(unbind, &QPushButton::clicked, this, [this, protect] {
        auto config = workspace_.configuration();
        const auto index = bindings_->currentRow();
        if (index < 0 || index >= int(config.bindings.size()))
            return;
        config.bindings.erase(config.bindings.begin() + index);
        protect([&] { workspace_.applyConfiguration(std::move(config)); });
    });
    connect(analyze_, &QPushButton::clicked, this, &WorkspaceDialog::startAnalysis);
    connect(cancel_, &QPushButton::clicked, this, &WorkspaceDialog::cancelAnalysis);
    connect(mode_, &QComboBox::currentIndexChanged, this, [this] {
        cancelAnalysis();
        resultGeneration_ = 0;
        navigate_->setEnabled(false);
        viewGeneration_ = ~std::uint64_t{0};
        refresh();
        status_->setText("계산 방식이 변경되었습니다. 다시 계산하세요.");
    });
    connect(load_->selectionModel(), &QItemSelectionModel::currentRowChanged, this, [this] { taskChoices(); });
    connect(navigate_, &QPushButton::clicked, this, [this] {
        if (!result_ || resultGeneration_ != workspace_.generation())
            return;
        const auto data = task_->currentData().toStringList();
        if (data.size() != 2 || !workspace_.activate(str(data[0]), str(data[1])))
            status_->setText(qDiagnosticText({"WORKSPACE_SOURCE_CHANGED", {}, {}}));
        else
            accept();
    });
    auto *timer = new QTimer(this);
    timer->setInterval(150);
    connect(timer, &QTimer::timeout, this, [this] { refresh(); });
    timer->start();
    refresh();
}
WorkspaceDialog::~WorkspaceDialog() {
    cancelAnalysis();
}
void WorkspaceDialog::cancelAnalysis() {
    if (control_)
        control_->cancel();
}
void WorkspaceDialog::refresh() {
    if (viewGeneration_ == workspace_.generation())
        return;
    viewGeneration_ = workspace_.generation();
    if (busy_)
        cancelAnalysis();
    if (result_ && resultGeneration_ != viewGeneration_) {
        status_->setText(qDiagnosticText({"WORKSPACE_SOURCE_CHANGED", {}, {}}));
        navigate_->setEnabled(false);
    }
    const auto &config = workspace_.configuration();
    path_->setText((workspace_.dirty() ? "변경 있음 · " : "") +
                   qs(workspace_.path().empty() ? "아직 저장하지 않은 작업공간" : workspace_.path()));
    projects_->setRowCount(int(config.projects.size()));
    for (int i = 0; i < projects_->rowCount(); ++i) {
        const auto &ref = config.projects[std::size_t(i)];
        auto *document = workspace_.document(ref.documentId);
        const WorkspaceProjectResult *source = nullptr;
        if (result_ && resultGeneration_ == viewGeneration_)
            for (const auto &value : result_->projects)
                if (value.projectId == ref.documentId)
                    source = &value;
        row(projects_, i,
            {qs(ref.name), document ? (document->dirty() ? "열림 · 미저장" : "열림") : "닫힘 · 분석 제외",
             document ? QString::number(document->project()->revision) : QString{},
             document ? qs(document->project()->startDate) : QString{},
             source ? state(source->status) : QString("미계산/변경"),
             source ? qs(source->finish.value_or("")) : QString{}, qs(ref.path)});
    }
    if (catalogGeneration_ == workspace_.catalogGeneration())
        return;
    catalogGeneration_ = workspace_.catalogGeneration();
    resources_->setRowCount(int(config.resources.size()));
    std::map<std::string, QString> names, projects;
    for (const auto &ref : config.projects)
        projects[ref.documentId] = qs(ref.name);
    for (int i = 0; i < resources_->rowCount(); ++i) {
        const auto &resource = config.resources[std::size_t(i)];
        names[resource.id] = qs(resource.name);
        row(resources_, i,
            {qs(resource.name), resource.type == "labor" ? "인력" : "장비",
             resource.maxUnits ? QString::number(*resource.maxUnits) : QString("미정"),
             QString::number(std::count_if(config.bindings.begin(), config.bindings.end(),
                                           [&](const auto &value) { return value.sharedId == resource.id; }))});
    }
    bindings_->setRowCount(int(config.bindings.size()));
    for (int i = 0; i < bindings_->rowCount(); ++i) {
        const auto &binding = config.bindings[std::size_t(i)];
        row(bindings_, i, {projects[binding.projectId], qs(binding.resourceId), names[binding.sharedId]});
    }
}
void WorkspaceDialog::editResource(bool create) {
    auto config = workspace_.configuration();
    const auto generation = workspace_.generation();
    const auto index = resources_->currentRow();
    if (!create && (index < 0 || index >= int(config.resources.size())))
        return;
    SharedResource resource = create ? SharedResource{} : config.resources[std::size_t(index)];
    if (create)
        resource.id = workspace_.newId();
    QDialog dialog(this);
    dialog.setObjectName("sharedResourceDialog");
    dialog.setWindowTitle("공유 자원 · 용량/달력");
    auto *form = new QFormLayout(&dialog);
    auto *name = new QLineEdit(qs(resource.name));
    name->setObjectName("sharedResourceName");
    form->addRow("이름", name);
    auto *type = new QComboBox;
    type->addItem("인력", "labor");
    type->addItem("장비", "equipment");
    type->setCurrentIndex(type->findData(qs(resource.type)));
    form->addRow("종류", type);
    auto *cap = new QLineEdit(resource.maxUnits ? QString::number(*resource.maxUnits) : QString("?"));
    cap->setObjectName("sharedResourceCapacity");
    form->addRow("기본 용량(100%=1명/대, ?=미정)", cap);
    QStringList days, periods, holidays, exceptions;
    for (auto day : resource.calendar.weekdays)
        days << QString::number(day);
    for (const auto &period : resource.calendar.periods)
        periods << timeText(period.begin) + "-" + timeText(period.end);
    for (const auto &day : resource.calendar.holidays)
        holidays << qs(day);
    for (const auto &day : resource.exceptions)
        exceptions << qs(day.date) + "=" + (day.units ? QString::number(*day.units) : QString("?"));
    auto *weekdays = new QLineEdit(days.join(','));
    form->addRow("근무 요일(월=1~일=7)", weekdays);
    auto *hours = new QLineEdit(periods.join(','));
    form->addRow("근무 구간", hours);
    auto *holiday = new QPlainTextEdit(holidays.join('\n'));
    form->addRow("휴일(한 줄에 YYYY-MM-DD)", holiday);
    auto *overrides = new QPlainTextEdit(exceptions.join('\n'));
    form->addRow("일별 용량(YYYY-MM-DD=100 또는 ?)", overrides);
    auto *error = new QLabel;
    error->setWordWrap(true);
    error->setTextFormat(Qt::PlainText);
    form->addRow(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            if (workspace_.generation() != generation)
                throw AppError(ErrorCode::WORKSPACE_SOURCE_CHANGED);
            resource.name = str(name->text().trimmed());
            resource.type = str(type->currentData().toString());
            resource.maxUnits = capacity(cap->text());
            resource.calendar.weekdays.clear();
            resource.calendar.periods.clear();
            resource.calendar.holidays.clear();
            resource.exceptions.clear();
            for (const auto &day : weekdays->text().split(',', Qt::SkipEmptyParts))
                resource.calendar.weekdays.push_back(integer(day, 1, 7));
            for (const auto &period : hours->text().split(',', Qt::SkipEmptyParts)) {
                const auto pair = period.split('-');
                if (pair.size() != 2)
                    throw AppError(ErrorCode::CALENDAR_PERIODS_INVALID);
                resource.calendar.periods.push_back({minute(pair[0], false), minute(pair[1], true)});
            }
            for (const auto &day : holiday->toPlainText().split('\n', Qt::SkipEmptyParts))
                resource.calendar.holidays.push_back(str(day.trimmed()));
            for (const auto &day : overrides->toPlainText().split('\n', Qt::SkipEmptyParts)) {
                auto pair = day.split('=');
                if (pair.size() != 2)
                    throw AppError(ErrorCode::RESOURCE_EXCEPTION_INVALID);
                resource.exceptions.push_back({str(pair[0].trimmed()), capacity(pair[1])});
            }
            if (create)
                config.resources.push_back(resource);
            else
                config.resources[std::size_t(index)] = resource;
            workspace_.applyConfiguration(config);
            dialog.accept();
        } catch (const std::exception &exception) {
            error->setText(qErrorText(exception));
        }
    });
    dialog.exec();
    refresh();
}
void WorkspaceDialog::bindResource() {
    const auto snapshots = workspace_.snapshots();
    auto config = workspace_.configuration();
    const auto generation = workspace_.generation();
    QDialog dialog(this);
    dialog.setObjectName("sharedBindingDialog");
    dialog.setWindowTitle("공유 자원 연결");
    auto *form = new QFormLayout(&dialog);
    auto *project = new QComboBox;
    project->setObjectName("bindingProject");
    for (const auto &snapshot : snapshots)
        project->addItem(qs(snapshot->name) + " · " + qs(snapshot->documentId), qs(snapshot->documentId));
    form->addRow("열린 프로젝트", project);
    auto *local = new QComboBox;
    local->setObjectName("bindingLocal");
    form->addRow("프로젝트 로컬 자원", local);
    auto *pool = new QComboBox;
    pool->setObjectName("bindingShared");
    for (const auto &resource : config.resources)
        pool->addItem(qs(resource.name), qs(resource.id));
    form->addRow("같은 실제 자원에 연결", pool);
    const auto fill = [&] {
        local->clear();
        const auto id = str(project->currentData().toString());
        for (const auto &snapshot : snapshots)
            if (snapshot->documentId == id)
                for (const auto &resource : snapshot->resources)
                    local->addItem(qs(resource.name) + " [" + qs(resource.id) + "]", qs(resource.id));
    };
    fill();
    connect(project, &QComboBox::currentIndexChanged, &dialog, [&] { fill(); });
    auto *error = new QLabel;
    error->setWordWrap(true);
    error->setTextFormat(Qt::PlainText);
    form->addRow(error);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    form->addRow(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        try {
            if (workspace_.generation() != generation)
                throw AppError(ErrorCode::WORKSPACE_SOURCE_CHANGED);
            const ResourceBinding binding{str(project->currentData().toString()), str(local->currentData().toString()),
                                          str(pool->currentData().toString())};
            const auto shared = std::find_if(config.resources.begin(), config.resources.end(),
                                             [&](const auto &value) { return value.id == binding.sharedId; });
            const Resource *resource = nullptr;
            for (const auto &snapshot : snapshots)
                if (snapshot->documentId == binding.projectId)
                    for (const auto &value : snapshot->resources)
                        if (value.id == binding.resourceId)
                            resource = &value;
            if (!resource || shared == config.resources.end() || resource->type != shared->type)
                throw AppError(ErrorCode::WORKSPACE_RESOURCE_MISSING, {binding.resourceId});
            std::erase_if(config.bindings, [&](const auto &value) {
                return value.projectId == binding.projectId && value.resourceId == binding.resourceId;
            });
            config.bindings.push_back(binding);
            workspace_.applyConfiguration(config);
            dialog.accept();
        } catch (const std::exception &exception) {
            error->setText(qErrorText(exception));
        }
    });
    dialog.exec();
    refresh();
}
void WorkspaceDialog::taskChoices() {
    task_->clear();
    navigate_->setEnabled(false);
    if (!result_ || resultGeneration_ != workspace_.generation())
        return;
    const auto index = load_->currentIndex().row();
    if (index < 0 || index >= int(result_->days.size()))
        return;
    for (const auto &reference : result_->days[std::size_t(index)].tasks) {
        auto *document = workspace_.document(reference.projectId);
        if (!document)
            continue;
        const auto snapshot = document->snapshot();
        const auto found = std::find_if(snapshot->tasks.begin(), snapshot->tasks.end(),
                                        [&](const auto &value) { return value.id == reference.taskId; });
        if (found != snapshot->tasks.end())
            task_->addItem(qs(snapshot->name) + " / " + qs(found->name),
                           QStringList{qs(reference.projectId), qs(reference.taskId)});
    }
    navigate_->setEnabled(task_->count() > 0);
}
void WorkspaceDialog::startAnalysis() {
    if (busy_)
        return;
    // Synchronize pending view notifications before freezing this operation's source.
    // Otherwise the next timer tick can cancel a fresh operation for an older edit.
    refresh();
    const auto generation = workspace_.generation();
    const auto config = workspace_.configuration();
    const auto snapshots = workspace_.snapshots();
    const bool forecast = mode_->currentIndex() == 0;
    result_.reset();
    auto old = loadModel_->replace({}, config);
    if (old)
        (void)QtConcurrent::run([old = std::move(old)] {});
    task_->clear();
    navigate_->setEnabled(false);
    busy_ = true;
    analyze_->setEnabled(false);
    mode_->setEnabled(false);
    cancel_->setEnabled(true);
    status_->setText("프로젝트별 일정과 공유 자원 부하를 계산 중…");
    control_ = std::make_shared<CalculationControl>();
    auto control = control_;
    struct Outcome {
        std::shared_ptr<SharedResourceResult> result;
        std::optional<Diagnostic> failure;
        bool cancelled = false;
    };
    auto *watcher = new QFutureWatcher<Outcome>(this);
    connect(watcher, &QFutureWatcher<Outcome>::finished, this, [this, watcher, control, generation, config] {
        auto outcome = watcher->result();
        watcher->deleteLater();
        busy_ = false;
        analyze_->setEnabled(true);
        mode_->setEnabled(true);
        cancel_->setEnabled(false);
        if (generation != workspace_.generation()) {
            status_->setText(qDiagnosticText({"WORKSPACE_SOURCE_CHANGED", {}, {}}));
            return;
        }
        if (control->cancelRequested.load() || outcome.cancelled) {
            status_->setText(qDiagnosticText({"WORKSPACE_CANCELLED", {}, {}}));
            return;
        }
        if (outcome.failure) {
            status_->setText(qDiagnosticText(*outcome.failure));
            return;
        }
        result_ = std::move(outcome.result);
        resultGeneration_ = generation;
        loadModel_->replace(result_, config);
        QStringList diagnostics;
        for (const auto &issue : result_->diagnostics)
            diagnostics << qDiagnosticText(issue);
        diagnosticDetails_->setPlainText(diagnostics.join('\n'));
        status_->setText(state(result_->status) + " · 포함 " + QString::number(result_->includedProjects) + "/" +
                         QString::number(result_->projects.size()) + " 프로젝트 · 미연결 자원 " +
                         QString::number(result_->unmappedResources) + "개 · 생략한 상세 진단 " +
                         QString::number(result_->omittedDiagnostics) + "건");
        viewGeneration_ = ~std::uint64_t{0};
        refresh();
        if (loadModel_->rowCount())
            load_->selectRow(0);
        taskChoices();
    });
    watcher->setFuture(QtConcurrent::run([config, snapshots, forecast, control] {
        Outcome outcome;
        try {
            outcome.result =
                std::make_shared<SharedResourceResult>(calculateSharedResources(config, snapshots, forecast, control));
        } catch (const CalculationCancelled &) {
            outcome.cancelled = true;
        } catch (const std::exception &error) {
            outcome.failure = diagnosticFromException(error);
        }
        return outcome;
    }));
}
} // namespace jobs
