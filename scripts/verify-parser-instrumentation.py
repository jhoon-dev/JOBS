"""Fail if a purported fuzz build leaves JOBS parser/dependency code uninstrumented."""
import json
from pathlib import Path
import shlex
import sys

build = Path(sys.argv[1] if len(sys.argv) > 1 else "build-sanitized")
commands = json.loads((build / "compile_commands.json").read_text())
required_coverage = {"jobs_domain", "jobs_application", "jobs_calculation", "jobs_persistence", "jobs_parser_harness"}
seen = set()
fuzz_entries = 0
checked = []
for entry in commands:
    source = Path(entry["file"])
    args = entry.get("arguments") or shlex.split(entry["command"])
    command = " ".join(args)
    if "/src/" in source.as_posix() or source.name in ("ParserHarness.cpp", "FuzzerEntry.cpp"):
        assert "-fsanitize=address,undefined" in args, source
        assert "-fno-sanitize-recover=all" in args, source
        assert "-fno-omit-frame-pointer" in args, source
        for target in required_coverage:
            if f"{target}.dir/" in command:
                assert "-fsanitize=fuzzer-no-link" in args, source
                seen.add(target)
        checked.append(source.name)
    if source.name == "FuzzerEntry.cpp":
        assert "-fsanitize=fuzzer" in args, source
        fuzz_entries += 1
assert seen == required_coverage and fuzz_entries == 3, (seen, fuzz_entries)
(build / "instrumentation.json").write_text(json.dumps({"status": "success", "coverage_targets": sorted(seen),
    "fuzz_entries": fuzz_entries, "checked_translation_units": checked}, indent=2), encoding="utf-8")
print(f"PASS: ASan/UBSan in {len(checked)} translation units; coverage in all five parser/dependency libraries")
