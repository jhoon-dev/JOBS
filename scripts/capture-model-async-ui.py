"""Exercise the application's native model/async editing fixture and preserve its evidence."""
import json
import pathlib
import subprocess
import sys

build = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
name = "jobs_async_editing_ui_tests" + (".exe" if sys.platform == "win32" else "")
binary = next((p for p in (build / name, build / "Release" / name) if p.is_file()), None)
if binary is None:
    raise SystemExit("Build jobs_async_editing_ui_tests first")
for filename in ("async-editing-ui.png", "async-editing-ui.json"):
    (build / filename).unlink(missing_ok=True)
subprocess.run([str(binary)], cwd=build, timeout=180, check=True)
image = build / "async-editing-ui.png"
assert image.read_bytes().startswith(b"\x89PNG\r\n\x1a\n")
metrics = json.loads((build / "async-editing-ui.json").read_text(encoding="utf-8"))
assert metrics["status"] == "success" and metrics["task_count"] == 10000
assert metrics["heartbeat_count_while_editing"] > 0
assert metrics["platform"] != "offscreen"
print(f"PASS: native model/view + cancellable edits; GUI enqueue observation {metrics['enqueue_gui_ms']:.2f} ms")
