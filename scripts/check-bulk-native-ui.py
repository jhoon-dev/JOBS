"""Exercise row notifications, async commands, selection and fallbacks natively."""
import pathlib
import subprocess
import sys

build = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
name = "jobs_bulk_editing_tests" + (".exe" if sys.platform == "win32" else "")
binary = next(p for p in (build / name, build / "Release" / name) if p.is_file())
subprocess.run([str(binary), "--native"], cwd=build, check=True, timeout=180)
