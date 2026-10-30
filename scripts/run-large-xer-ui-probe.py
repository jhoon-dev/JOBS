"""Run one bounded native XER measurement and preserve raw timing samples."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path('build'))
parser.add_argument('--input', type=Path, required=True)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
build, source, output = args.build_dir.resolve(), args.input.resolve(), args.output.resolve()
name = 'jobs_large_xer_ui_probe' + ('.exe' if sys.platform == 'win32' else '')
binary = next((path for path in (build/name, build/'Release'/name) if path.is_file()), None)
if binary is None or output.exists():
    raise SystemExit('Probe missing or output already exists')
output.parent.mkdir(parents=True, exist_ok=True)
with output.with_suffix('.log').open('wb') as log:
    try:
        result = subprocess.run([str(binary), str(source), str(output)], cwd=build,
                                stdout=log, stderr=subprocess.STDOUT, timeout=300)
    except subprocess.TimeoutExpired:
        output.write_text(json.dumps({'status': 'timeout', 'limit_seconds': 300})+'\n', encoding='utf-8')
        raise SystemExit('Native probe timed out')
if result.returncode:
    raise SystemExit('Native probe failed: ' + str(output.with_suffix('.log')))
data = json.loads(output.read_text())
if data['status'] != 'passed' or data['platform'] not in ('windows', 'cocoa'):
    raise SystemExit('Native desktop acceptance not confirmed')
provenance = {'revision_at_run': subprocess.check_output(['git', 'rev-parse', 'HEAD'], text=True).strip(),
              'dirty_at_run': bool(subprocess.check_output(['git', 'status', '--porcelain'])),
              'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
              'input_sha256': hashlib.sha256(source.read_bytes()).hexdigest()}
output.with_suffix('.provenance.json').write_text(json.dumps(provenance, indent=2)+'\n', encoding='utf-8')
print('PASS:', data['tasks'], 'tasks;', round(data['f9_calculation_and_commit_ms'], 2), 'ms calculate/commit')
