"""Measure bounded editing cases; retain raw samples and fail incomplete history checks."""
import argparse
import json
import pathlib
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", default="build")
parser.add_argument("--output-dir")
parser.add_argument("--repeats", type=int, default=7)
parser.add_argument("--tasks", nargs="+", type=int, default=[1000, 10000])
parser.add_argument("--timeout", type=int, default=180)
parser.add_argument("--platform")
parser.add_argument("--allow-short-history", action="store_true", help="Pre-optimization reference only")
args = parser.parse_args()
build = pathlib.Path(args.build_dir).resolve()
name = "jobs_editing_performance_probe" + (".exe" if sys.platform == "win32" else "")
binary = next((p for p in (build / name, build / "Release" / name) if p.is_file()), None)
if binary is None:
    raise SystemExit("Build jobs_editing_performance_probe first")
output = pathlib.Path(args.output_dir).resolve() if args.output_dir else build / "performance" / "editing"
output.mkdir(parents=True, exist_ok=True)
failed = False
for count in args.tasks:
    for baseline_count in (0, 3):
        target = output / f"{count}-{baseline_count}.json"
        target.unlink(missing_ok=True)
        command = [str(binary), "--tasks", str(count), "--baselines", str(baseline_count),
                   "--repeats", str(args.repeats), "--output", str(target)]
        if not args.allow_short_history:
            command += ["--require-100"]
        if args.platform:
            command += ["-platform", args.platform]
        try:
            run = subprocess.run(command, cwd=build, capture_output=True, text=True,
                                 encoding="utf-8", errors="replace", timeout=args.timeout)
            target.with_suffix(".log").write_text(run.stdout + run.stderr, encoding="utf-8")
            if run.returncode:
                raise RuntimeError(f"probe exit {run.returncode}: {run.stderr[-1500:]}")
            data = json.loads(target.read_text(encoding="utf-8"))
            if (data["status"] != "success" or data["task_count"] != count or
                    data["baseline_count"] != baseline_count or
                    (not args.allow_short_history and data["retained_undo_after_100_edits"] != 100) or
                    data["estimated_history_bytes"] > 32 * 1024**2):
                raise RuntimeError("probe did not validate the fixture/history budget")
            measurements = data["measurements"]
            print(f"{count} tasks/{baseline_count} baselines: name UI "
                  f"{measurements['ui_name_commit_events']['median_ms']:.2f} ms; duration UI "
                  f"{measurements['ui_duration_commit_events']['median_ms']:.2f} ms; "
                  f"history {data['estimated_history_bytes']} bytes/{data['retained_undo_after_100_edits']} steps", flush=True)
        except (subprocess.TimeoutExpired, RuntimeError, ValueError, KeyError) as error:
            target.write_text(json.dumps({"status": "timeout" if isinstance(error, subprocess.TimeoutExpired) else "failed",
                                          "task_count": count, "baseline_count": baseline_count, "reason": str(error)},
                                         indent=2), encoding="utf-8")
            print(f"FAIL {count}/{baseline_count}: {error}", flush=True)
            failed = True
raise SystemExit(1 if failed else 0)
