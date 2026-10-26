"""Run each deterministic performance case in its own bounded process."""
import argparse
import json
import pathlib
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", default="build")
parser.add_argument("--repeats", type=int, default=3)
parser.add_argument("--timeout", type=int, default=180)
parser.add_argument("--tasks", nargs="+", type=int, default=[1000, 10000])
parser.add_argument("--platform", default=None)
args = parser.parse_args()
build = pathlib.Path(args.build_dir).resolve()
name = "jobs_performance_probe" + (".exe" if sys.platform == "win32" else "")
binary = next((p for p in [build / name, build / "Release" / name] if p.is_file()), None)
if binary is None:
    raise SystemExit("Performance probe executable is missing")
output = build / "performance"
output.mkdir(exist_ok=True)
failed = False
for profile in ["legacy", "effort"]:
    for count in args.tasks:
        target = output / f"{profile}-{count}.json"
        target.unlink(missing_ok=True)  # A failed run cannot leave an older success.
        command = [str(binary), "--tasks", str(count), "--profile", profile,
                   "--repeats", str(args.repeats), "--output", str(target)]
        if args.platform:
            command += ["-platform", args.platform]
        try:
            run = subprocess.run(command, cwd=build, capture_output=True, text=True,
                                 encoding="utf-8", errors="replace", timeout=args.timeout)
            target.with_suffix(".log").write_text(run.stdout + run.stderr, encoding="utf-8")
            if run.returncode:
                raise RuntimeError(f"probe exit {run.returncode}: {run.stderr[-1500:]}")
            data = json.loads(target.read_text(encoding="utf-8"))
            if data["status"] != "success" or data["task_count"] != count:
                raise RuntimeError("probe did not validate the complete fixture")
            print(f"{profile}/{count}: calculation median {data['calculation']['median_ms']:.2f} ms; "
                  f"UI {data['ui_preparation']['median_ms']:.2f} ms; "
                  f"peak {data['peak_process_resident_bytes'] / 1024**2:.1f} MiB", flush=True)
        except (subprocess.TimeoutExpired, RuntimeError, ValueError, KeyError) as error:
            target.write_text(json.dumps({"status": "timeout" if isinstance(error, subprocess.TimeoutExpired)
                                         else "failed", "task_count": count, "profile": profile,
                                         "reason": str(error)}, indent=2), encoding="utf-8")
            print(f"FAIL {profile}/{count}: {error}", flush=True)
            failed = True
raise SystemExit(1 if failed else 0)
