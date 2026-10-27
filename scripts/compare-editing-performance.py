"""Compare paired observations without treating wall-clock differences as a CI gate."""
import json
import pathlib
import sys

before_dir, after_dir, output_path = map(pathlib.Path, sys.argv[1:4])
comparisons = []
for count in (1000, 10000):
    for baselines in (0, 3):
        filename = f"{count}-{baselines}.json"
        before = json.loads((before_dir / filename).read_text(encoding="utf-8"))
        after = json.loads((after_dir / filename).read_text(encoding="utf-8"))
        for data in (before, after):
            assert data["status"] == "success" and data["task_count"] == count
            assert data["baseline_count"] == baselines and data["fixture"] == "jobs-perf-editing-v1"
        for key in ("os", "qt", "platform", "compiler", "build_configuration", "repeats"):
            assert before[key] == after[key], f"Different comparison environment: {key}"
        assert after["retained_undo_after_100_edits"] == 100
        assert after["estimated_history_bytes"] <= 32 * 1024**2
        timings = {}
        for operation, values in before["measurements"].items():
            old = values["median_ms"]
            new = after["measurements"][operation]["median_ms"]
            timings[operation] = {"before_median_ms": old, "after_median_ms": new,
                                  "before_observed_p95_ms": values["observed_p95_ms"],
                                  "after_observed_p95_ms": after["measurements"][operation]["observed_p95_ms"],
                                  "observed_median_change_percent": (new / old - 1) * 100 if old else None}
        comparisons.append({"task_count": count, "baseline_count": baselines,
                            "before_file": filename, "after_file": filename,
                            "before_revision": before["build_source_revision"],
                            "after_revision": after["build_source_revision"],
                            "before_history_steps": before["retained_undo_after_100_edits"],
                            "after_history_steps": after["retained_undo_after_100_edits"],
                            "before_history_bytes": before["estimated_history_bytes"],
                            "after_history_bytes": after["estimated_history_bytes"], "timings": timings})
        print(f"{count}/{baselines}: name {timings['ui_name_commit_events']['before_median_ms']:.2f} -> "
              f"{timings['ui_name_commit_events']['after_median_ms']:.2f} ms; "
              f"undo {before['retained_undo_after_100_edits']} -> 100 steps")
output_path.parent.mkdir(parents=True, exist_ok=True)
output_path.write_text(json.dumps({"status": "success", "semantics": "Paired observations, no universal latency guarantee or timing gate",
                                 "cases": comparisons}, indent=2), encoding="utf-8")
