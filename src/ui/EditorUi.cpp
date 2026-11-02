#include "ui/EditorUi.h"
#include "formatting/QtDiagnosticText.h"
#include <QPushButton>
#include <QToolButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QVariant>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTableWidget>
#include <QHeaderView>
#include <QScrollArea>
#include <QScreen>
#include <QVBoxLayout>
#include <algorithm>

namespace jobs {
void alignEditorControl(QWidget &control, int height) {
    control.setAttribute(Qt::WA_LayoutUsesWidgetRect);
    control.setFixedHeight(height);
}
void prepareEditorDialog(QDialog &dialog) {
    if (dialog.property("editorUiPrepared").toBool())
        return;
    dialog.setProperty("editorUiPrepared", true);
    const int height = std::max(32, dialog.fontMetrics().height() + 16);
    for (auto *control : dialog.findChildren<QWidget *>()) {
        if (auto *combo = qobject_cast<QComboBox *>(control)) {
            // Project/baseline names must not become the window's minimum width.
            combo->setMinimumContentsLength(8);
            combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
            combo->setToolTip(combo->currentText());
            QObject::connect(combo, &QComboBox::currentTextChanged, combo, &QWidget::setToolTip);
        }
        if (auto *label = qobject_cast<QLabel *>(control))
            label->setWordWrap(true);
        const bool input =
            qobject_cast<QComboBox *>(control) || qobject_cast<QAbstractSpinBox *>(control) ||
            (qobject_cast<QLineEdit *>(control) && !qobject_cast<QAbstractSpinBox *>(control->parentWidget()));
        if (input || qobject_cast<QPushButton *>(control) || qobject_cast<QToolButton *>(control) ||
            qobject_cast<QCheckBox *>(control) || qobject_cast<QRadioButton *>(control))
            alignEditorControl(*control, height);
        if (auto *button = qobject_cast<QPushButton *>(control)) {
            auto text = button->text();
            text.remove('&');
            button->setMinimumWidth(button->fontMetrics().horizontalAdvance(text) + 28);
        }
    }
    for (auto *form : dialog.findChildren<QFormLayout *>()) {
        form->setRowWrapPolicy(QFormLayout::WrapLongRows);
        form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
        form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        form->setHorizontalSpacing(16);
        form->setVerticalSpacing(10);
        for (int row = 0; row < form->rowCount(); ++row) {
            auto *labelItem = form->itemAt(row, QFormLayout::LabelRole);
            auto *fieldItem = form->itemAt(row, QFormLayout::FieldRole);
            auto *label = labelItem ? qobject_cast<QLabel *>(labelItem->widget()) : nullptr;
            auto *field = fieldItem ? fieldItem->widget() : nullptr;
            if (label && field) {
                label->setBuddy(field);
                if (field->accessibleName().isEmpty())
                    field->setAccessibleName(label->text());
            }
        }
    }
    for (auto *table : dialog.findChildren<QTableWidget *>()) {
        table->verticalHeader()->setDefaultSectionSize(height + 4);
        table->verticalHeader()->setMinimumSectionSize(height + 4);
    }
    if (dialog.layout()) {
        dialog.layout()->setSpacing(10);
        QLayoutItem *footer = nullptr;
        // Keep the final action row outside the scrolling body. A short window
        // must still offer Close/Cancel without scrolling past a large table.
        for (int i = dialog.layout()->count() - 1; i >= 0; --i) {
            auto *item = dialog.layout()->itemAt(i);
            bool actions =
                qobject_cast<QDialogButtonBox *>(item->widget()) || qobject_cast<QPushButton *>(item->widget());
            if (auto *row = item->layout()) {
                actions = row->count() > 0;
                for (int j = 0; j < row->count(); ++j) {
                    auto *entry = row->itemAt(j);
                    actions = actions && (entry->spacerItem() || qobject_cast<QPushButton *>(entry->widget()));
                }
            }
            if (actions) {
                footer = dialog.layout()->takeAt(i);
                break;
            }
        }
        auto *content = new QWidget;
        content->setLayout(dialog.layout());
        auto *scroll = new QScrollArea(&dialog);
        scroll->setObjectName("editorScrollArea");
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setWidget(content);
        auto *outer = new QVBoxLayout(&dialog);
        outer->setContentsMargins(8, 8, 8, 8);
        outer->addWidget(scroll);
        if (footer) {
            if (auto *row = footer->layout()) {
                row->setParent(nullptr);
                outer->addLayout(row);
            } else {
                auto *widget = footer->widget();
                delete footer;
                outer->addWidget(widget);
            }
        }
        if (dialog.screen()) {
            const auto available = dialog.screen()->availableGeometry().size();
            dialog.resize(std::min(dialog.width(), std::max(300, available.width() - 40)),
                          std::min(dialog.height(), std::max(240, available.height() - 70)));
        }
    }
}
void showEditorError(QLabel &label, const std::exception &error, QWidget *field) {
    label.setTextFormat(Qt::PlainText);
    label.setWordWrap(true);
    label.setAccessibleName("입력 오류 · 수정 후 다시 적용하세요");
    label.setText(qErrorText(error));
    if (field && field->isEnabled()) {
        field->setFocus(Qt::OtherFocusReason);
        if (auto *line = qobject_cast<QLineEdit *>(field))
            line->selectAll();
    }
    // The feedback may be below a tall editor on a small display. Keep it in
    // view without discarding the focused field's draft.
    if (label.parentWidget() && label.parentWidget()->layout())
        label.parentWidget()->layout()->activate();
    for (auto *parent = label.parentWidget(); parent; parent = parent->parentWidget())
        if (auto *scroll = qobject_cast<QScrollArea *>(parent)) {
            scroll->ensureWidgetVisible(&label, 12, 12);
            break;
        }
}
} // namespace jobs
