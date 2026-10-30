"""Check the native Qt workspace at process-local scale factors; OS settings remain unchanged."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path('build'))
args = parser.parse_args()
build = args.build_dir.resolve()
name = 'jobs_workspace_presentation_tests' + ('.exe' if sys.platform == 'win32' else '')
binary = next((path for path in (build/name, build/'Release'/name) if path.is_file()), None)
if binary is None:
    raise SystemExit('Build the workspace presentation tests first')
results = []
for scale in ('1', '1.25', '1.5', '2'):
    folder = build/'workspace-scaling'/scale
    folder.mkdir(parents=True, exist_ok=False)
    environment = os.environ.copy()
    environment['QT_SCALE_FACTOR'] = scale
    environment['JOBS_LAYOUT_OUTPUT_DIR'] = str(folder)
    run = subprocess.run([str(binary), 'componentHeightsAndApplicationIcon', '-o', str(folder/'qtest.log')+',txt'],
                         cwd=build, env=environment, capture_output=True, timeout=60)
    if run.returncode:
        raise SystemExit((folder/'qtest.log').read_text(encoding='utf-8', errors='replace'))
    data = json.loads((folder/'geometry.json').read_text())
    results.append({'scale_factor': scale, 'checks': data})
    print('PASS: scale', scale, 'at three window sizes', flush=True)
(build/'workspace-scaling/results.json').write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
