"""Run a separate ad-hoc-signed sandbox fixture; never include it in the Store app."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import uuid

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('build', type=Path)
parser.add_argument('--qt-root', type=Path, required=True)
args = parser.parse_args()
if sys.platform != 'darwin':
    parser.error('Actual macOS sandbox enforcement is required')
root = Path(__file__).resolve().parents[1]
build = args.build.resolve()
app = build / 'PPMSandboxValidation.app'
subprocess.run([str(args.qt_root / 'bin/macdeployqt'), str(app), '-appstore-compliant', '-codesign=-'], check=True)
subprocess.run(['codesign', '--force', '--sign', '-', '--entitlements',
                str(root / 'packaging/macos/PPM.entitlements'), str(app)], check=True)
subprocess.run(['codesign', '--verify', '--deep', '--strict', str(app)], check=True)
output = build / 'sandbox-validation'
output.mkdir(exist_ok=True)
outside = Path(os.environ['RUNNER_TEMP']) / ('ppm-sandbox-denied-' + uuid.uuid4().hex)
outside.write_text('Unselected external file must remain unchanged.', encoding='utf-8')
executable = app / 'Contents/MacOS/PPMSandboxValidation'
help_result = subprocess.run(['open', '-h'], capture_output=True, text=True)
help_text = help_result.stdout + help_result.stderr
if '--stdout' not in help_text or '--stderr' not in help_text:
    raise SystemExit('LaunchServices open tool does not support captured application streams')
for phase in ('write', 'read'):
    stdout, stderr = output / (phase + '.stdout.log'), output / (phase + '.stderr.log')
    # LaunchServices initializes the app container before Qt reads its home/data paths.
    run = subprocess.run(['open', '-W', '-n', '--stdout', str(stdout), '--stderr', str(stderr),
                          str(app), '--args', phase, str(outside)], capture_output=True, text=True, timeout=120)
    text = ''.join(path.read_text(errors='replace') for path in (stdout, stderr) if path.exists())
    (output / (phase + '.log')).write_text(text + run.stdout + run.stderr, encoding='utf-8')
    if run.returncode or 'PASS: active sandbox,' not in text:
        raise SystemExit(f'Sandbox {phase} failed; inspect {output / (phase + ".log")}')
    print(text.strip())
assert outside.read_text(encoding='utf-8') == 'Unselected external file must remain unchanged.'
(output / 'status.json').write_text(json.dumps({
    'sandbox_enforced': True, 'container_restart_lifecycle': True, 'unselected_external_write_denied': True,
    'external_folder_selection_verified': False,
    'scope': 'Separate signed fixture. Real external-folder grants, cancellation and bookmark restart require verification before upload.',
}, indent=2) + '\n', encoding='utf-8')
