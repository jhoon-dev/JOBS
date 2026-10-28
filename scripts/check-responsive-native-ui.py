"""Run bounded incremental-view assertions on the native platform."""
import pathlib
import subprocess
import sys

build = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
name = "jobs_responsive_ui_tests" + (".exe" if sys.platform == "win32" else "")
binary = next(p for p in (build / name, build / "Release" / name) if p.is_file())
subprocess.run([str(binary)], cwd=build, check=True, timeout=180)
print("PASS: native 10k/3-baseline incremental model, WBS, selection/scroll and full-refresh fallbacks")
