"""Read-only quality gates; format --fix is the only mutating operation."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def git(*args):
    return subprocess.check_output(["git", *args], cwd=ROOT)


def cpp_path(name):
    path = (ROOT / name).resolve()
    relative = path.relative_to(ROOT)
    if relative.parts[0] not in ("src", "tests") or path.suffix not in (".h", ".cpp"):
        raise ValueError(f"not a repository C++ source: {name}")
    return path


def format_paths(base=None):
    names = {line.strip() for line in (ROOT / ".clang-format-files").read_text().splitlines()
             if line.strip() and not line.lstrip().startswith("#")}
    changes = git("diff", "--name-only", "--diff-filter=ACMR", "-z", "HEAD", "--", "src", "tests")
    changes += git("ls-files", "--others", "--exclude-standard", "-z", "--", "src", "tests")
    if base:
        revision = git("rev-parse", "--verify", "--end-of-options", f"{base}^{{commit}}").decode().strip()
        changes += git("diff", "--name-only", "--diff-filter=ACMR", "-z", revision, "HEAD", "--", "src", "tests")
    for name in changes.decode("utf-8").split("\0"):
        if name and Path(name).suffix in (".cpp", ".h"):
            names.add(name)
    # Removed enrolled files are configuration errors, not silently skipped checks.
    paths = [cpp_path(name) for name in sorted(names)]
    for path in paths:
        if not path.is_file():
            raise ValueError(f"missing enrolled source: {path}")
    return paths


def version(binary, expected):
    text = subprocess.check_output([binary, "--version"], text=True)
    if not re.search(expected, text):
        raise ValueError(f"unexpected tool version: {text.strip()}")
    return text.strip()


def format_check(args):
    tool_version = version(args.tool or "clang-format", r"version 18\.1\.8\b")
    paths = format_paths(args.base)
    changed = []
    for path in paths:
        formatted = subprocess.check_output([args.tool or "clang-format", "--style=file", str(path)], cwd=ROOT)
        if formatted != path.read_bytes():
            changed.append(path.relative_to(ROOT).as_posix())
            if args.fix:
                path.write_bytes(formatted)
    result = {"status": "success" if not changed or args.fix else "failed", "tool": tool_version,
              "base": args.base, "files": [p.relative_to(ROOT).as_posix() for p in paths],
              "format_changes": changed, "fix": args.fix}
    print(json.dumps(result, indent=2))
    return result


def tidy_check(args):
    if args.jobs < 1 or args.jobs > 4:
        raise ValueError("--jobs must be between 1 and 4")
    tool = args.tool or "clang-tidy-18"
    tool_version = version(tool, r"version 18\.")
    build = Path(args.build_dir).resolve()
    database = json.loads((build / "compile_commands.json").read_text())
    sources = sorted({Path(e["file"]).resolve() for e in database
                      if Path(e["file"]).resolve().is_relative_to(ROOT / "src")})
    expected = set((ROOT / "src").rglob("*.cpp"))
    if set(sources) != expected:
        raise ValueError(f"compile database does not cover all src C++ files: {sorted(map(str, expected - set(sources)))}")
    output = build / "cpp-quality"
    output.mkdir(parents=True, exist_ok=True)
    result = {"status": "running", "tool": tool_version, "source_revision": git("rev-parse", "HEAD").decode().strip(),
              "database": str(build / "compile_commands.json"), "translation_units": len(sources), "commands": []}
    # Save before analysis so interrupted runs retain the intended source list.
    for source in sources:
        result["commands"].append({"file": source.relative_to(ROOT).as_posix(),
            "command": [tool, "-p", str(build), "--quiet", str(source)], "timeout_seconds": 180})
    manifest = output / "tidy.json"
    manifest.write_text(json.dumps(result, indent=2), encoding="utf-8")

    def analyze(entry):
        log = output / (entry["file"].replace("/", "_") + ".log")
        try:
            run = subprocess.run(entry["command"], cwd=ROOT, capture_output=True, timeout=entry["timeout_seconds"])
            log.write_bytes(run.stdout + run.stderr)
            entry["returncode"] = run.returncode
        except subprocess.TimeoutExpired as error:
            log.write_bytes((error.stdout or b"") + (error.stderr or b""))
            entry["timed_out"] = True
        print(f"{'PASS' if entry.get('returncode') == 0 else 'FAIL'}: {entry['file']}", flush=True)

    try:
        with ThreadPoolExecutor(max_workers=args.jobs) as pool:
            list(pool.map(analyze, result["commands"]))
        result["status"] = "success" if all(e.get("returncode") == 0 for e in result["commands"]) else "failed"
    finally:
        manifest.write_text(json.dumps(result, indent=2), encoding="utf-8")
    return result


def main():
    parser = argparse.ArgumentParser()
    modes = parser.add_subparsers(dest="mode", required=True)
    fmt = modes.add_parser("format")
    fmt.add_argument("--base")
    fmt.add_argument("--fix", action="store_true")
    fmt.add_argument("--tool")
    tidy = modes.add_parser("tidy")
    tidy.add_argument("--build-dir", default="build-quality")
    tidy.add_argument("--tool")
    tidy.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    try:
        result = format_check(args) if args.mode == "format" else tidy_check(args)
        return 0 if result["status"] == "success" else 1
    except (ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"FAIL: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
