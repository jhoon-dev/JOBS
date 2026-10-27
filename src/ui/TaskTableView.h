#pragma once
#include <QTableView>

namespace jobs {
class TaskTableView final : public QTableView {
public:
    using QTableView::QTableView;
    void commitEditors();
    int currentRow() const { return currentIndex().row(); }
    int currentColumn() const { return currentIndex().column(); }
    int rowCount() const { return model() ? model()->rowCount() : 0; }
    int columnCount() const { return model() ? model()->columnCount() : 0; }
    void setCurrentCell(int row, int column, QItemSelectionModel::SelectionFlags flags = QItemSelectionModel::ClearAndSelect) {
        selectionModel()->setCurrentIndex(model()->index(row, column), flags);
    }
};
}
