#pragma once
#include "application/Document.h"
#include <QString>
#include <functional>
#include <optional>
class QWidget;
namespace jobs {
// Returns only after worker cleanup. Publication owns its final cancellation boundary.
bool runExchangeOperation(QWidget *, Document &, const QString &, std::function<void()> worker,
                          bool publishing = false);
template <class T, class F>
std::optional<T> prepareExchange(QWidget *parent, Document &document, const QString &label, F function) {
    std::optional<T> output;
    if (!runExchangeOperation(parent, document, label, [&] { output.emplace(function()); }))
        return {};
    return output;
}
} // namespace jobs
