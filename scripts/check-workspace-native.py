"""Exercise production workspace windows and capture the native Qt load screen."""
from pathlib import Path
import subprocess
import sys

build = Path(sys.argv[1] if len(sys.argv) > 1 else "build").resolve()
name = "jobs_workspace_ui_tests" + (".exe" if sys.platform == "win32" else "")
binary = next(p for p in (build / name, build / "Release" / name) if p.is_file())
image = build / "workspace-ui.png"
image.unlink(missing_ok=True)
result = subprocess.run([str(binary), "--capture-workspace"], cwd=build, capture_output=True, timeout=120)
(build / "workspace-native.log").write_bytes(result.stdout + result.stderr)
if result.returncode or not image.is_file():
    raise SystemExit("Native workspace acceptance failed; see workspace-native.log")
print("PASS: native multi-project workspace, load analysis, navigation scope, cancellation and reopen")
