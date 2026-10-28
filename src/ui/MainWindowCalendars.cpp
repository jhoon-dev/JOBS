#include "ui/MainWindow.h"
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QInputDialog>
#include <QRegularExpression>
#include <QVBoxLayout>
#include <QLabel>
#include <algorithm>
namespace jobs {
namespace {
QString q(const std::string &s) {
    return QString::fromStdString(s);
}
QString time(int minute) {
    return QString("%1:%2").arg(minute / 60, 2, 10, QChar('0')).arg(minute % 60, 2, 10, QChar('0'));
}
int minute(const QString &text) {
    auto match = QRegularExpression("^([0-9]{2}):([0-9]{2})$").match(text);
    bool ok = false;
    auto h = match.captured(1).toInt(&ok), m = match.captured(2).toInt();
    if (!ok || h > 24 || m > 59 || (h == 24 && m))
        throw std::runtime_error("시간은 HH:mm이며 구간 끝 24:00을 허용합니다.");
    return h * 60 + m;
}
} // namespace
void MainWindow::manageCalendars() {
    if (!document_.project())
        return;
    QDialog dialog(this);
    dialog.setObjectName("calendarDialog");
    dialog.setWindowTitle("작업·자원 달력 등록·편집");
    dialog.resize(660, 530);
    auto *layout = new QVBoxLayout(&dialog);
    auto *note =
        new QLabel("프로젝트 기본 달력은 월–금 09–12 / 13–18과 프로젝트 휴일을 사용합니다.\n사용자 달력은 독립된 근무 "
                   "요일·시간대·휴일을 사용합니다. 새 정의/수정 후 작업 또는 자원 등록에서 배정하세요.\n야간 근무는 "
                   "자정을 기준으로 나눈 구간으로 입력합니다. 지연/선행 시간은 후행 달력 근무분입니다.");
    note->setWordWrap(true);
    layout->addWidget(note);
    auto *form = new QFormLayout;
    layout->addLayout(form);
    auto *choice = new QComboBox;
    choice->setObjectName("calendarChoice");
    form->addRow("달력", choice);
    auto *name = new QLineEdit;
    name->setObjectName("calendarName");
    form->addRow("이름", name);
    auto *weekdays = new QLineEdit;
    weekdays->setObjectName("calendarWeekdays");
    form->addRow("근무 요일 (월1~일7)", weekdays);
    auto *periods = new QLineEdit;
    periods->setObjectName("calendarPeriods");
    form->addRow("시간대 (쉼표 구분)", periods);
    auto *holidays = new QPlainTextEdit;
    holidays->setObjectName("calendarHolidays");
    form->addRow("휴일 YYYY-MM-DD (줄 구분)", holidays);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close);
    auto *apply = buttons->addButton("등록/변경 적용", QDialogButtonBox::ActionRole);
    apply->setObjectName("applyCalendar");
    auto *remove = buttons->addButton("미배정 달력 삭제", QDialogButtonBox::ActionRole);
    remove->setObjectName("removeCalendar");
    layout->addWidget(buttons);
    auto populate = [&](const std::string &selected) {
        choice->blockSignals(true);
        choice->clear();
        choice->addItem("새 달력", QString{});
        for (const auto &c : document_.project()->calendars)
            choice->addItem(q(c.name), q(c.id));
        choice->setCurrentIndex(std::max(0, choice->findData(q(selected))));
        choice->blockSignals(false);
    };
    auto load = [&] {
        TaskCalendar c;
        for (const auto &v : document_.project()->calendars)
            if (q(v.id) == choice->currentData().toString())
                c = v;
        name->setText(q(c.name));
        QStringList days, hours, dates;
        for (int d : c.weekdays)
            days << QString::number(d);
        for (const auto &s : c.periods)
            hours << time(s.begin) + "-" + time(s.end);
        for (const auto &d : c.holidays)
            dates << q(d);
        weekdays->setText(days.join(","));
        periods->setText(hours.join(","));
        holidays->setPlainText(dates.join("\n"));
        remove->setEnabled(!document_.readOnly() && !c.id.empty());
    };
    populate({});
    load();
    const bool editable = !document_.readOnly();
    name->setReadOnly(!editable);
    weekdays->setReadOnly(!editable);
    periods->setReadOnly(!editable);
    holidays->setReadOnly(!editable);
    apply->setEnabled(editable);
    connect(choice, &QComboBox::currentIndexChanged, &dialog, [&] { load(); });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(apply, &QPushButton::clicked, &dialog, [&] {
        try {
            TaskCalendar c;
            c.id = choice->currentData().toString().toStdString();
            c.name = name->text().trimmed().toStdString();
            c.weekdays.clear();
            c.periods.clear();
            for (const auto &s : weekdays->text().split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts)) {
                bool ok = false;
                const auto d = s.toInt(&ok);
                if (!ok)
                    throw std::runtime_error("근무 요일은 1~7입니다.");
                c.weekdays.push_back(d);
            }
            for (const auto &s : periods->text().split(',', Qt::SkipEmptyParts)) {
                auto parts = s.trimmed().split('-');
                if (parts.size() != 2)
                    throw std::runtime_error("시간대 예: 09:00-12:00,13:00-18:00");
                c.periods.push_back({minute(parts[0].trimmed()), minute(parts[1].trimmed())});
            }
            for (const auto &s : holidays->toPlainText().split(QRegularExpression("[\\s,]+"), Qt::SkipEmptyParts))
                c.holidays.push_back(s.toStdString());
            const auto id = document_.putCalendar(std::move(c));
            populate(id);
            load();
        } catch (const std::exception &e) {
            report(e);
        }
    });
    connect(remove, &QPushButton::clicked, &dialog, [&] {
        try {
            document_.removeCalendar(choice->currentData().toString().toStdString());
            populate({});
            load();
        } catch (const std::exception &e) {
            report(e);
        }
    });
    dialog.exec();
    refresh();
}
void MainWindow::chooseTaskCalendar() {
    if (!document_.project() || document_.readOnly() || selectedTask_.empty())
        return;
    const auto &p = *document_.project();
    auto it = std::find_if(p.tasks.begin(), p.tasks.end(), [&](const auto &t) { return t.id == selectedTask_; });
    if (it == p.tasks.end())
        return;
    QStringList labels{"프로젝트 기본 달력"};
    std::vector<std::string> ids{std::string{}};
    int current = 0;
    for (const auto &c : p.calendars) {
        labels << q(c.name) + " [" + q(c.id) + "]";
        ids.push_back(c.id);
        if (c.id == it->calendarId)
            current = int(ids.size() - 1);
    }
    bool ok = false;
    auto label = QInputDialog::getItem(this, "선택 작업 달력 배정", q(it->name), labels, current, false, &ok);
    if (!ok)
        return;
    try {
        document_.setTaskCalendar(std::size_t(it - p.tasks.begin()), ids.at(std::size_t(labels.indexOf(label))));
        refresh();
    } catch (const std::exception &e) {
        report(e);
    }
}
} // namespace jobs
