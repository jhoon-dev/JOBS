#include "ui/TaskTableView.h"
#include <QAbstractItemDelegate>
#include <QLineEdit>
#include <QPointer>

namespace jobs {
void TaskTableView::commitEditors() {
    for (auto* editor : findChildren<QLineEdit*>()) if (editor->isVisible()) {
        QPointer<QWidget> guard(editor);
        commitData(editor);
        if (guard) closeEditor(guard.data(), QAbstractItemDelegate::NoHint);
    }
}
}
