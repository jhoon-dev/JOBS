"""Bounded paired UI observations; no machine-specific timing gate."""
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
name = "jobs_bulk_performance_probe" + (".exe" if sys.platform == "win32" else "")
binary = next(p for p in (build / name, build / "Release" / name) if p.is_file())
output = pathlib.Path(args.output_dir).resolve() if args.output_dir else build / "performance" / "bulk"
output.mkdir(parents=True, exist_ok=True)
comparison = []
for tasks in (1000, 10000):
    for baselines in (0, 3):
        target = output / f"{tasks}-{baselines}.json"
        target.unlink(missing_ok=True)
        run = subprocess.run([str(binary), "--tasks", str(tasks), "--baselines", str(baselines), "--output", str(target)],
                             cwd=build, capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180)
        target.with_suffix(".log").write_text(run.stdout + run.stderr, encoding="utf-8")
        if run.returncode:
            print(run.stdout + run.stderr, file=sys.stderr, flush=True)
        run.check_returncode()
        data = json.loads(target.read_text(encoding="utf-8"))
        assert data["status"] == "success" and data["task_count"] == tasks and data["baseline_count"] == baselines
        assert len(data["operations"]) == 8
        print(f"PASS: bulk UI {tasks}/{baselines}", flush=True)
        if args.reference:
            reference = json.loads((pathlib.Path(args.reference) / target.name).read_text(encoding="utf-8"))
            assert reference["source_revision"] != data["source_revision"]
            assert reference["built_product_source_sha256"] != data["built_product_source_sha256"]
            assert reference["platform"] == data["platform"]
            for before, after in zip(reference["operations"], data["operations"], strict=True):
                assert before["operation"] == after["operation"]
                comparison.append({"tasks": tasks, "baselines": baselines, "operation": after["operation"],
                                   "before": before, "after": after})
if comparison:
    (output / "comparison.json").write_text(json.dumps({"scope": "Single observations, includes automated modal delays; no timing gate", "operations": comparison}, indent=2), encoding="utf-8")
