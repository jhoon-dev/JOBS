#pragma once
#include "domain/Project.h"
#include <memory>
#include <tuple>

namespace jobs {
// Internal ownership only. Public Document borrows remain const; snapshots own
// the old state. A synchronous write must detach before returning a mutable row.
class ProjectState {
public:
    const std::optional<Project>& value() const {
        static const std::optional<Project> empty;
        return state_ ? *state_ : empty;
    }
    bool has_value() const { return value().has_value(); }
    explicit operator bool() const { return has_value(); }
    const Project& operator*() const { return value().value(); }
    const Project* operator->() const { return &**this; }
    Project& write() {
        if (state_.use_count() != 1) state_ = std::make_shared<std::optional<Project>>(value());
        return state_->value();
    }
    ProjectState& operator=(Project project) {
        auto next = std::make_shared<std::optional<Project>>(std::move(project));
        state_ = std::move(next); return *this;
    }
    void reset() { state_.reset(); }
    bool shared() const { return state_.use_count() > 1; }
    const void* identity() const { return state_.get(); }
    std::shared_ptr<const Project> snapshot() const {
        return has_value() ? std::shared_ptr<const Project>(state_, &**this) : nullptr;
    }
private:
    std::shared_ptr<std::optional<Project>> state_;
};

struct DirtyState {
    std::size_t differentTasks = 0;
    bool other = true;
    bool aligned = false;
    bool dirty() const { return other || differentTasks; }
};
DirtyState compareSavedInput(const Project&, const ProjectState& saved);
}
