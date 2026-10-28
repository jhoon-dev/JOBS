"""Retain independent request/worker/commit observations on a bounded fixture."""
import argparse
import json
import pathlib
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", default="build")
parser.add_argument("--output-dir")
parser.add_argument("--repeats", type=int, default=3)
args = parser.parse_args()
build = pathlib.Path(args.build_dir).resolve()
name = "jobs_responsive_performance_probe" + (".exe" if sys.platform == "win32" else "")
binary = next(p for p in (build / name, build / "Release" / name) if p.is_file())
output = pathlib.Path(args.output_dir).resolve() if args.output_dir else build / "performance" / "responsive"
output.mkdir(parents=True, exist_ok=True)
for tasks in (1000, 10000):
    for baselines in (0, 3):
        target = output / f"{tasks}-{baselines}.json"
        target.unlink(missing_ok=True)
        run = subprocess.run([str(binary), "--tasks", str(tasks), "--baselines", str(baselines),
                              "--repeats", str(args.repeats), "--output", str(target)], cwd=build,
                             capture_output=True, text=True, encoding="utf-8", errors="replace", timeout=180)
        target.with_suffix(".log").write_text(run.stdout + run.stderr, encoding="utf-8")
        run.check_returncode()
        data = json.loads(target.read_text(encoding="utf-8"))
        assert data["status"] == "success" and data["task_count"] == tasks and data["baseline_count"] == baselines
        print(f"{tasks}/{baselines}: request {data['request_gui']['median_ms']:.3f} ms; "
              f"commit {data['apply_gui']['median_ms']:.3f} ms; worker {data['prepare_worker']['median_ms']:.2f} ms", flush=True)
