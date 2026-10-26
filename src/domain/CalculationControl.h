#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
namespace jobs {
enum class CalculationStage { Validation, Network, Calendars, Forward, Backward, Finished };
struct CalculationControl {
    std::atomic<bool> cancelRequested{false};
    std::atomic<CalculationStage> stage{CalculationStage::Validation};
    std::atomic<std::uint64_t> completed{0}, total{0}, checkpoints{0};
    void cancel() noexcept { cancelRequested.store(true,std::memory_order_relaxed); }
};
// Internal cooperative unwind. Deliberately independent of std::exception:
// horizon/constraint handlers must never turn cancellation into a conflict.
struct CalculationCancelled {};
class EvaluationCache {
    std::unordered_map<std::string,std::shared_ptr<const void>> values_;
    friend std::shared_ptr<const void> evaluationCached(const std::string&);
    friend void evaluationRemember(std::string,std::shared_ptr<const void>);
};
class EvaluationScope {
    struct State;
    std::unique_ptr<State> owned_;
    State* previous_=nullptr;
    static thread_local State* current_;
    friend void calculationCheckpoint();
    friend void calculationProgress(CalculationStage,std::uint64_t,std::uint64_t);
    friend std::shared_ptr<const void> evaluationCached(const std::string&);
    friend void evaluationRemember(std::string,std::shared_ptr<const void>);
public:
    // Nested un-controlled calls share only this immutable operation's cache.
    explicit EvaluationScope(std::shared_ptr<CalculationControl> control={},std::shared_ptr<EvaluationCache> cache={});
    ~EvaluationScope();
    EvaluationScope(const EvaluationScope&)=delete;
    EvaluationScope& operator=(const EvaluationScope&)=delete;
};
void calculationCheckpoint();
void calculationProgress(CalculationStage,std::uint64_t completed,std::uint64_t total);
std::shared_ptr<const void> evaluationCached(const std::string& key);
void evaluationRemember(std::string key,std::shared_ptr<const void> value);
}
