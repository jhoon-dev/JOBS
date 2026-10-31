"""Capture the application's native Qt widgets without desktop input."""
import argparse
import os
from pathlib import Path
import subprocess
import struct
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', nargs='?', default='build')
parser.add_argument('--store', action='store_true', help='Require an Apple-supported native screenshot size')
args = parser.parse_args()
build = Path(args.build).resolve()
name = 'jobs_workspace_presentation_tests' + ('.exe' if sys.platform == 'win32' else '')
binary = next((path for path in (build / name, build / 'Release' / name) if path.is_file()), None)
if binary is None:
    raise SystemExit('Build the workspace presentation fixture first')
output = build / 'workspace-ui'
environment = os.environ.copy()
environment['JOBS_UI_SCREENSHOT_DIR'] = str(output)
if args.store:
    environment['JOBS_STORE_SCREENSHOT'] = '1'
subprocess.run([str(binary), 'calculatedWorkspaceAndNativeCapture', '-o', '-,txt'],
               cwd=build, env=environment, timeout=60, check=True)
for name in ('workspace-1440.png', 'workspace-1024.png'):
    path = output / name
    if not path.is_file() or not path.read_bytes().startswith(b'\x89PNG\r\n\x1a\n'):
        raise SystemExit('Native screenshot missing: ' + name)
    if args.store and name == 'workspace-1440.png':
        dimensions = struct.unpack('>II', path.read_bytes()[16:24])
        if dimensions not in ((1440, 900), (2880, 1800)):
            raise SystemExit('Native screenshot has unsupported Apple dimensions: ' + str(dimensions))
    print('PASS:', path)
