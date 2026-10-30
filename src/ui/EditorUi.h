#pragma once
#include <exception>
class QDialog;
class QWidget;
class QLabel;
namespace jobs {
// Common geometry uses the widget rectangle, rather than native optical insets.
void alignEditorControl(QWidget &control, int height);
void prepareEditorDialog(QDialog &dialog);
void showEditorError(QLabel &label, const std::exception &error, QWidget *field = nullptr);
} // namespace jobs
