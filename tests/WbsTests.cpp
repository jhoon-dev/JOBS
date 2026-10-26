#include "application/Document.h"
#include "domain/Wbs.h"
#include <iostream>
#include <stdexcept>
#include <functional>

namespace {
void check(bool c, const char* message) { if (!c) throw std::runtime_error(message); }
void rejects(const std::function<void()>& fn) {
    bool failed = false; try { fn(); } catch (const std::exception&) { failed = true; }
    check(failed, "expected WBS rejection");
}
class Store final : public jobs::ProjectStore {
    mutable int serial_ = 0;
public:
    jobs::Project stored;
    std::string newId() const override { return "id-" + std::to_string(++serial_); }
    std::string normalizePath(const std::string& p) const override { return p; }
    std::unique_ptr<jobs::WriteLease> lock(const std::string&) override { return nullptr; }
    jobs::StoredProject load(const std::string&) const override { return {stored, "test", false}; }
    std::string save(const std::string&, const jobs::Project&, const std::string&) override { return "test"; }
};
}
int main() {
    try {
        Store store; jobs::Document doc(store);
        doc.create("WBS fixture", "2026-10-26", {"A", "B"});
        doc.setDuration(0, 480); doc.setDuration(1, 960);
        doc.setPredecessors(1, {doc.project()->tasks[0].id});
        const auto firstId = doc.project()->tasks[0].id, secondId = doc.project()->tasks[1].id;
        auto w1 = doc.addWbs("설계"), w2 = doc.addWbs("개발");
        doc.assignWbs(0, w1); doc.assignWbs(1, w2);
        auto before = jobs::calculate(*doc.project()); doc.applyCalculation(before);
        check(jobs::summarizeWbs(*doc.project(), &before, w1).knownDurationMinutes == 480 &&
              jobs::summarizeWbs(*doc.project(), &before, w2).knownDurationMinutes == 960, "F11 initial rollup mismatch");
        doc.assignWbs(1, w1);
        check(doc.calculation()->status == jobs::CalculationStatus::Stale, "WBS move did not stale revision");
        auto after = jobs::calculate(*doc.project());
        check(before.tasks == after.tasks && before.projectFinish == after.projectFinish, "WBS move changed execution schedule");
        check(doc.project()->tasks[0].id == firstId && doc.project()->tasks[1].id == secondId &&
              doc.project()->tasks[1].predecessors == std::vector<std::string>{firstId}, "WBS move changed stable ID / graph");
        const auto sum = jobs::summarizeWbs(*doc.project(), &after, w1);
        check(sum.taskCount == 2 && sum.knownDurationMinutes == 1440 && sum.missingDurations == 0 &&
              sum.start == "2026-10-26 09:00" && sum.finish == "2026-10-28 18:00", "F11 final rollup mismatch");
        check(jobs::summarizeWbs(*doc.project(), &after, w2).taskCount == 0 &&
              !jobs::summarizeWbs(*doc.project(), &after, w2).finish, "empty WBS got dates");
        check(!jobs::summarizeWbs(*doc.project(), &before, w1).finish, "stale rollup got dates");
        auto child = doc.addWbs("詳細", w1); doc.assignWbs(1, child);
        check(jobs::wbsTaskRows(*doc.project(), w1).size() == 2 && jobs::wbsTaskRows(*doc.project(), child).size() == 1,
              "nested WBS filter mismatch");
        auto snapshot = *doc.project();
        rejects([&] { doc.moveWbs(w1, child); }); rejects([&] { doc.moveWbs(w1, w1); });
        rejects([&] { doc.moveWbs(w1, "missing"); }); rejects([&] { doc.assignWbs(0, "missing"); });
        rejects([&] { doc.removeWbs(w1); }); rejects([&] { doc.removeWbs(child); });
        rejects([&] { doc.renameWbs(w1, " "); }); rejects([&] { doc.addWbs(" "); });
        check(*doc.project() == snapshot, "failed WBS edit changed document/revision");
        doc.moveWbs(child, w2);
        auto current = jobs::calculate(*doc.project());
        check(jobs::summarizeWbs(*doc.project(), &current, "").knownDurationMinutes == 1440 &&
              jobs::summarizeWbs(*doc.project(), &current, w1).knownDurationMinutes == 480 &&
              jobs::summarizeWbs(*doc.project(), &current, w2).knownDurationMinutes == 960, "subtree move double-counted/dropped task");
        auto revision = doc.project()->revision; doc.moveWbs(child, w2);
        check(doc.project()->revision == revision, "no-op move changed revision");
        doc.renameWbs(child, "상세 개발");
        doc.assignWbs(1, ""); doc.removeWbs(child); doc.removeWbs(w2);
        check(doc.project()->wbsNodes.size() == 1 && jobs::wbsTaskRows(*doc.project(), "").size() == 2, "empty deletion / ungrouped task mismatch");
        doc.addTask("미정 작업", w1);
        auto incomplete = jobs::summarizeWbs(*doc.project(), nullptr, w1);
        check(incomplete.taskCount == 2 && incomplete.missingDurations == 1 && incomplete.knownDurationMinutes == 480 &&
              !incomplete.finish, "unknown duration counted as zero/valid schedule");
        store.stored = *doc.project(); jobs::Document readOnly(store); readOnly.open("fixture", true);
        rejects([&] { readOnly.addWbs("blocked"); }); rejects([&] { readOnly.assignWbs(0, ""); });
        rejects([&] { readOnly.moveWbs(w1, ""); }); rejects([&] { readOnly.renameWbs(w1, "blocked"); });
        auto malformed = *doc.project(); malformed.wbsNodes.push_back(malformed.wbsNodes.front());
        rejects([&] { jobs::validate(malformed); });
        malformed = *doc.project(); malformed.tasks[0].wbsId = "missing";
        rejects([&] { jobs::validate(malformed); });
        // Validate a deep hierarchy iteratively; no recursion-based depth limit.
        malformed = *doc.project(); malformed.wbsNodes.clear(); for (auto& t : malformed.tasks) t.wbsId.clear();
        for (int i = 0; i < 2000; ++i) malformed.wbsNodes.push_back({"deep" + std::to_string(i), "node", i ? "deep" + std::to_string(i - 1) : ""});
        jobs::validate(malformed); check(jobs::wbsSubtree(malformed, "deep0").size() == 2000, "deep WBS traversal failed");
        malformed.wbsNodes.front().parentId = "deep1999"; rejects([&] { jobs::validate(malformed); });
        std::cout << "PASS: F11 rollups, stable IDs/graph/dates, nested filter/move, unknowns, stale dates, cycles/dangling refs, atomic rejection, read-only, deep tree\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
