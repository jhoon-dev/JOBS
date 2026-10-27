#pragma once
#include "ui/TaskTableView.h"
#include <QPersistentModelIndex>

inline jobs::TaskTableView* taskTable(QWidget& window) {
    return dynamic_cast<jobs::TaskTableView*>(window.findChild<QTableView*>("taskTable"));
}
inline QModelIndex cell(QAbstractItemView* table, int row, int column) { return table->model()->index(row, column); }
inline QString cellText(QAbstractItemView* table, int row, int column) { return cell(table, row, column).data().toString(); }
inline bool setCellText(QAbstractItemView* table, int row, int column, const QString& text) {
    return table->model()->setData(cell(table, row, column), text, Qt::EditRole);
}
inline void selectCellRange(QAbstractItemView* table, int firstRow, int firstColumn, int lastRow, int lastColumn, bool select = true) {
    const QItemSelection range(cell(table, firstRow, firstColumn), cell(table, lastRow, lastColumn));
    table->selectionModel()->select(range, select ? QItemSelectionModel::Select : QItemSelectionModel::Deselect);
}
