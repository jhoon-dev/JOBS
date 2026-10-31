#pragma once
#include "application/Document.h"
#include <QObject>
#include <QString>

namespace jobs {
class AsyncEditController final : public QObject {
public:
    using Completion = std::function<void(EditState, const QString &)>;
    explicit AsyncEditController(Document &, QObject *parent = nullptr);
    ~AsyncEditController() override;
    std::shared_ptr<CalculationControl> submit(EditCommand, QString label, Completion completion);
    void cancel();
    bool cancelDurationEdit(const std::string &taskId);
    bool busy() const {
        return busy_;
    }
    std::shared_ptr<CalculationControl> control() const {
        return control_;
    }
    const QString &label() const {
        return label_;
    }
    std::function<void()> stateChanged;
    std::function<void()> beforeApply;
    std::function<Document::ViewPreparation()> captureView;

private:
    struct Pending {
        Document::EditRequest request;
        QString label;
        Completion completion;
        std::shared_ptr<CalculationControl> control;
        std::optional<std::string> durationTaskId;
    };
    Document &document_;
    std::uint64_t listener_ = 0;
    std::uint64_t activeSession_ = 0;
    bool busy_ = false, applying_ = false;
    EditState cancelState_ = EditState::Cancelled;
    std::shared_ptr<CalculationControl> control_;
    QString label_;
    std::optional<Pending> pending_;
    std::optional<std::string> activeDurationTaskId_;
    void start(Pending);
    void invalidate(EditState);
};
} // namespace jobs
