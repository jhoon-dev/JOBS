"""Paired observations of prepared filtered/sorted/WBS view publication."""
import argparse
import json
import pathlib
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", default="build")
parser.add_argument("--output-dir")
parser.add_argument("--reference")
args = parser.parse_args()
build = pathlib.Path(args.build_dir).resolve()
name = "jobs_projected_view_probe" + (".exe" if sys.platform == "win32" else "")
binary = next(p for p in (build / name, build / "Release" / name) if p.is_file())
output = pathlib.Path(args.output_dir).resolve() if args.output_dir else build / "performance" / "projected-views"
output.mkdir(parents=True, exist_ok=True)
comparison = []
for mode in ("search", "sort", "wbs", "combined"):
    for tasks in (1000, 10000):
        for baselines in (0, 3):
            target = output / f"{mode}-{tasks}-{baselines}.json"
            target.unlink(missing_ok=True)
            run = subprocess.run([str(binary), "--view", mode, "--tasks", str(tasks), "--baselines", str(baselines), "--output", str(target)],
                                 cwd=build, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180)
            target.with_suffix(".log").write_text(run.stdout + run.stderr, encoding="utf-8")
            if run.returncode:
                print(run.stdout + run.stderr, file=sys.stderr, flush=True)
            run.check_returncode()
            data = json.loads(target.read_text(encoding="utf-8"))
            assert data["status"] == "success" and data["view"] == mode and data["task_count"] == tasks
            assert data["baseline_count"] == baselines and len(data["operations"]) == 8
            print(f"PASS: {mode} {tasks}/{baselines}", flush=True)
            if args.reference:
                reference = json.loads((pathlib.Path(args.reference) / target.name).read_text(encoding="utf-8"))
                assert reference["source_revision"] != data["source_revision"]
                assert reference["built_product_source_sha256"] != data["built_product_source_sha256"]
                assert reference["platform"] == data["platform"]
                for before, after in zip(reference["operations"], data["operations"], strict=True):
                    assert before["operation"] == after["operation"]
                    comparison.append({"view": mode, "tasks": tasks, "baselines": baselines,
                                       "operation": after["operation"], "before": before, "after": after})
if comparison:
    (output / "comparison.json").write_text(json.dumps({"scope": "Single observations, no timing gate; setup readiness excluded, modals included", "operations": comparison}, indent=2), encoding="utf-8")
