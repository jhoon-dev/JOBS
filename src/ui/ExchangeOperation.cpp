#include "ui/ExchangeOperation.h"
#include "domain/CalculationControl.h"
#include <QCloseEvent>
#include <QDialog>
#include <QFutureWatcher>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent/QtConcurrentRun>

namespace jobs {
namespace {
class Progress final : public QDialog {
public:
    Progress(QWidget *parent, QString label, std::shared_ptr<CalculationControl> control)
        : QDialog(parent), control_(std::move(control)) {
        setObjectName("exchangeOperation");
        setWindowTitle("파일 교환");
        setWindowModality(Qt::ApplicationModal);
        auto *layout = new QVBoxLayout(this);
        status_ = new QLabel(std::move(label), this);
        status_->setTextFormat(Qt::PlainText);
        layout->addWidget(status_);
        auto *progress = new QProgressBar(this);
        progress->setObjectName("exchangeProgress");
        progress->setRange(0, 0);
        layout->addWidget(progress);
        detail_ = new QLabel("준비 중…", this);
        detail_->setObjectName("exchangeProgressText");
        layout->addWidget(detail_);
        cancel_ = new QPushButton("취소", this);
        cancel_->setObjectName("exchangeCancel");
        layout->addWidget(cancel_);
        connect(cancel_, &QPushButton::clicked, this, &Progress::reject);
        connect(&timer_, &QTimer::timeout, this, [this] {
            if (!control_->cancelRequested.load())
                detail_->setText(
                    QString("처리·검증 중 · %1 / %2").arg(control_->completed.load()).arg(control_->total.load()));
        });
        timer_.start(100);
    }
    void reject() override {
        control_->cancel();
        cancel_->setEnabled(false);
        detail_->setText("취소 중 · 임시 파일을 정리하고 있습니다…");
    }
    void finish() {
        timer_.stop();
        QDialog::accept();
    }

protected:
    void closeEvent(QCloseEvent *event) override {
        reject();
        event->ignore();
    }

private:
    std::shared_ptr<CalculationControl> control_;
    QLabel *status_, *detail_;
    QPushButton *cancel_;
    QTimer timer_;
};
struct Result {
    bool cancelled = false;
    std::optional<Diagnostic> failure;
};
} // namespace
bool runExchangeOperation(QWidget *parent, Document &document, const QString &label, std::function<void()> worker,
                          bool publishing) {
    auto control = std::make_shared<CalculationControl>();
    const auto session = document.sessionGeneration();
    const auto revision = document.project() ? document.project()->revision : 0;
    bool stale = false;
    const auto token = document.subscribe([&](Document::Event) {
        if (document.sessionGeneration() != session ||
            (document.project() ? document.project()->revision : 0) != revision) {
            stale = true;
            control->cancel();
        }
    });
    struct Subscription {
        Document &document;
        std::uint64_t token;
        ~Subscription() {
            document.unsubscribe(token);
        }
    } subscription{document, token};
    Progress dialog(parent, label, control);
    QFutureWatcher<Result> watcher;
    QObject::connect(&watcher, &QFutureWatcher<Result>::finished, &dialog, &Progress::finish);
    watcher.setFuture(QtConcurrent::run([worker = std::move(worker), control, publishing] {
        Result result;
        try {
            EvaluationScope scope(control);
            calculationCheckpoint();
            worker();
            if (!publishing)
                calculationCheckpoint();
        } catch (const CalculationCancelled &) {
            result.cancelled = true;
        } catch (const std::exception &error) {
            result.failure = diagnosticFromException(error);
        }
        return result;
    }));
    if (!watcher.isFinished())
        dialog.exec();
    const auto result = watcher.result();
    if (result.failure)
        throw AppError(*result.failure);
    if (stale && (result.cancelled || !publishing))
        throw AppError(ErrorCode::EXCHANGE_SOURCE_CHANGED);
    return !result.cancelled && (publishing || !control->cancelRequested.load());
}
} // namespace jobs
