"""Exercise editor drafts and navigation with native Qt at process-local scales."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--build-dir', type=Path, default=Path('build'))
args = parser.parse_args()
build = args.build_dir.resolve()
name = 'jobs_editing_polish_tests'+('.exe' if sys.platform == 'win32' else '')
binary = next(p for p in (build/name, build/'Release'/name) if p.is_file())
for scale in ('1', '1.25', '1.5', '2'):
    folder = build/'editing-polish-native'/scale
    folder.mkdir(parents=True, exist_ok=False)
    environment = os.environ.copy()
    environment.pop('QT_QPA_PLATFORM', None)
    environment['QT_SCALE_FACTOR'] = scale
    environment['JOBS_EDITOR_CAPTURE_DIR'] = str(folder)
    run = subprocess.run([str(binary), '-o', str(folder/'qtest.log')+',txt'],
                         env=environment, cwd=build, capture_output=True, timeout=90)
    if run.returncode:
        raise SystemExit((folder/'qtest.log').read_text(encoding='utf-8', errors='replace'))
    print('PASS: native editor/navigation at scale '+scale, flush=True)
