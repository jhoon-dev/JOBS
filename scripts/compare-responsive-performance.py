"""Compare phase observations with no wall-clock performance gate."""
import json
import pathlib
import sys

before_dir, after_dir, target = map(pathlib.Path, sys.argv[1:4])
cases = []
for tasks in (1000, 10000):
    for baselines in (0, 3):
        name = f"{tasks}-{baselines}.json"
        before = json.loads((before_dir / name).read_text(encoding="utf-8"))
        after = json.loads((after_dir / name).read_text(encoding="utf-8"))
        for key in ("status", "fixture", "os", "platform", "qt", "task_count", "baseline_count", "repeats"):
            assert before[key] == after[key], key
        assert after["status"] == "success" and after["fixture"] == "jobs-responsive-phases-v1"
        phases = {}
        for key in ("request_gui", "prepare_worker", "apply_gui"):
            phases[key] = {"before_median_ms": before[key]["median_ms"], "after_median_ms": after[key]["median_ms"],
                           "before_observed_p95_ms": before[key]["observed_p95_ms"], "after_observed_p95_ms": after[key]["observed_p95_ms"]}
        cases.append({"tasks": tasks, "baselines": baselines, "before_revision": before["source_revision"],
                      "after_revision": after["source_revision"], "phases": phases})
        print(f"{tasks}/{baselines}: request {phases['request_gui']['before_median_ms']:.3f} -> "
              f"{phases['request_gui']['after_median_ms']:.3f} ms; commit "
              f"{phases['apply_gui']['before_median_ms']:.3f} -> {phases['apply_gui']['after_median_ms']:.3f} ms")
target.parent.mkdir(parents=True, exist_ok=True)
target.write_text(json.dumps({"status": "success", "semantics": "Paired phase observations; same CI toolchain, no universal latency guarantee", "cases": cases}, indent=2), encoding="utf-8")
