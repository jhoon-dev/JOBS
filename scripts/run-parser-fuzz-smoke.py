"""Bounded libFuzzer runs. Preserve commands, logs, corpus and crash artifacts."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument("--build-dir", default="build-sanitized")
parser.add_argument("--runs", type=int, default=5000)
args = parser.parse_args()
if args.runs < 1 or args.runs > 1000000:
    parser.error("--runs must be between 1 and 1000000")
build = Path(args.build_dir).resolve()
output = build / "parser-fuzz"
output.mkdir(parents=True, exist_ok=True)
summary = {"source_revision": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
           "scope": "finite parser smoke, not a security or compatibility guarantee",
           "seed": 20261028, "runs_requested_per_target": args.runs,
           "environment": {k: os.environ.get(k) for k in ("ASAN_OPTIONS", "UBSAN_OPTIONS", "CXX")},
           "commands": [], "status": "running"}
manifest = output / "run.json"


def record(command, name, timeout):
    entry = {"command": list(map(str, command)), "timeout_seconds": timeout}
    summary["commands"].append(entry)
    manifest.write_text(json.dumps(summary, indent=2), encoding="utf-8")
    try:
        result = subprocess.run(command, capture_output=True, timeout=timeout)
        entry["returncode"] = result.returncode
        (output / f"{name}.log").write_bytes(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(f"{name} failed ({result.returncode}); see {output}")
    except subprocess.TimeoutExpired as error:
        entry["timed_out"] = True
        (output / f"{name}.log").write_bytes((error.stdout or b"") + (error.stderr or b""))
        raise
    finally:
        manifest.write_text(json.dumps(summary, indent=2), encoding="utf-8")


try:
    record([build / "jobs_parser_seed_tests", "--write-corpus", output / "corpus"], "seeds", 120)
    for kind in ("jobs", "xml", "xer"):
        artifacts = output / "artifacts" / kind
        artifacts.mkdir(parents=True, exist_ok=True)
        binary = build / f"jobs_fuzz_{kind}"
        corpus = output / "corpus" / kind
        limits = ["-max_len=65536", "-timeout=10", "-rss_limit_mb=2048", "-malloc_limit_mb=1024",
                  f"-artifact_prefix={artifacts}{os.sep}", "-print_final_stats=1"]
        # File arguments replay every seed once without mutating the initial corpus.
        record([binary, *sorted(corpus.iterdir()), "-runs=1", *limits], f"{kind}-replay", 120)
        record([binary, corpus, f"-runs={args.runs}", "-seed=20261028", *limits], f"{kind}-fuzz", 180)
        print(f"PASS: {kind} replay + {args.runs} bounded fuzz runs", flush=True)
    summary["status"] = "success"
except Exception as error:
    summary["status"] = "failed"
    summary["error"] = str(error)
    print(error, file=sys.stderr)
finally:
    manifest.write_text(json.dumps(summary, indent=2), encoding="utf-8")
sys.exit(0 if summary["status"] == "success" else 1)
