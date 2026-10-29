#pragma once
#include <QObject>
#include <QString>
#include <functional>
#include <memory>

namespace jobs {
struct StoreUpdateResult {
    bool success = false;
    unsigned count = 0;
    QString error;
};
class StoreUpdateClient : public QObject {
public:
    using Completion = std::function<void(StoreUpdateResult)>;
    using QObject::QObject;
    virtual void check(quintptr window, Completion) = 0;
    virtual void cancel() = 0;
};
std::unique_ptr<StoreUpdateClient> createStoreUpdateClient();
} // namespace jobs
