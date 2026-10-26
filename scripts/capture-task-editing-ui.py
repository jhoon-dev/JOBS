"""Capture only the application's own native widget, without desktop input."""
import pathlib
import subprocess
import sys

build = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
name = "jobs_task_editing_ui_tests" + (".exe" if sys.platform == "win32" else "")
binary = next((p for p in (build / name, build / "Release" / name) if p.is_file()), None)
if binary is None:
    raise SystemExit("Build the task editing UI fixture first")
target = build / "task-editing-native-smoke.png"
target.unlink(missing_ok=True)
subprocess.run([str(binary), "--capture-only"], cwd=build, timeout=60, check=True)
if not target.is_file() or not target.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"):
    raise SystemExit("The native UI fixture did not produce a PNG")
print(f"PASS: {target.name} ({target.stat().st_size} bytes)")
