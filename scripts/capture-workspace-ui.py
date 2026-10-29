"""Capture the application's native Qt widgets without desktop input."""
import os
from pathlib import Path
import subprocess
import sys

build = Path(sys.argv[1] if len(sys.argv) > 1 else 'build').resolve()
name = 'jobs_workspace_presentation_tests' + ('.exe' if sys.platform == 'win32' else '')
binary = next((path for path in (build / name, build / 'Release' / name) if path.is_file()), None)
if binary is None:
    raise SystemExit('Build the workspace presentation fixture first')
output = build / 'workspace-ui'
environment = os.environ.copy()
environment['JOBS_UI_SCREENSHOT_DIR'] = str(output)
subprocess.run([str(binary), 'calculatedWorkspaceAndNativeCapture', '-o', '-,txt'],
               cwd=build, env=environment, timeout=60, check=True)
for name in ('workspace-1440.png', 'workspace-1024.png'):
    path = output / name
    if not path.is_file() or not path.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'):
        raise SystemExit('Native screenshot missing: ' + name)
    print('PASS:', path)
