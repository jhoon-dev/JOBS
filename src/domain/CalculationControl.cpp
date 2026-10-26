#include "domain/CalculationControl.h"
#include <unordered_map>
namespace jobs {
struct EvaluationScope::State {
    std::shared_ptr<CalculationControl> control;
    std::shared_ptr<EvaluationCache> cache;
};
thread_local EvaluationScope::State* EvaluationScope::current_=nullptr;
EvaluationScope::EvaluationScope(std::shared_ptr<CalculationControl> control,std::shared_ptr<EvaluationCache> cache):previous_(current_) {
    if(!current_||control||cache){owned_=std::make_unique<State>();owned_->control=std::move(control);owned_->cache=cache?std::move(cache):std::make_shared<EvaluationCache>();current_=owned_.get();}
}
EvaluationScope::~EvaluationScope(){if(owned_)current_=previous_;}
void calculationCheckpoint(){auto* state=EvaluationScope::current_;if(state&&state->control){state->control->checkpoints.fetch_add(1,std::memory_order_relaxed);if(state->control->cancelRequested.load(std::memory_order_relaxed))throw CalculationCancelled{};}}
void calculationProgress(CalculationStage stage,std::uint64_t done,std::uint64_t total){calculationCheckpoint();auto* state=EvaluationScope::current_;if(state&&state->control){state->control->stage.store(stage,std::memory_order_relaxed);state->control->completed.store(done,std::memory_order_relaxed);state->control->total.store(total,std::memory_order_relaxed);}}
std::shared_ptr<const void> evaluationCached(const std::string& key){auto* state=EvaluationScope::current_;if(!state)return {};auto it=state->cache->values_.find(key);return it==state->cache->values_.end()?std::shared_ptr<const void>{}:it->second;}
void evaluationRemember(std::string key,std::shared_ptr<const void> value){auto* state=EvaluationScope::current_;if(state&&state->cache->values_.size()<1024)state->cache->values_.emplace(std::move(key),std::move(value));}
}
