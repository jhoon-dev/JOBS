"""Exercise the native XML dialogs and capture the application's own preview."""
import pathlib
import subprocess
import sys

build = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
name = "jobs_project_xml_ui_tests" + (".exe" if sys.platform == "win32" else "")
binary = next((p for p in (build / name, build / "Release" / name) if p.is_file()), None)
if binary is None:
    raise SystemExit("Build the Project XML UI tests first")
target = build / "project-xml-preview-native.png"
target.unlink(missing_ok=True)
subprocess.run([str(binary)], cwd=build, timeout=60, check=True)
if not target.is_file() or not target.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"):
    raise SystemExit("The native Project XML preview did not produce a PNG")
print(f"PASS: {target.name} ({target.stat().st_size} bytes)")
