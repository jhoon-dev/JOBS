"""Disposable-Windows acceptance: owned installation, running-instance rejection, PID handoff and state preservation."""
import argparse
import ctypes
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--installer', type=Path, required=True)
args = parser.parse_args()
state = ROOT / 'build/update-install-state.json'
if state.exists():
    raise ValueError('Use a fresh test state; existing acceptance data is preserved')
spec = importlib.util.spec_from_file_location('installer_acceptance', ROOT / 'scripts/check-windows-installer.py')
helper = importlib.util.module_from_spec(spec)
spec.loader.exec_module(helper)


def lifecycle(mode):
    subprocess.run([sys.executable, str(ROOT / 'scripts/check-windows-installer.py'), mode,
                    '--installer', str(args.installer.resolve()), '--state', str(state)], check=True, timeout=150)


lifecycle('install')  # Refuses pre-existing application registration or user data.
info = json.loads(state.read_text(encoding='utf-8'))
target, case = Path(info['target']), Path(info['case'])
project = case / 'Update-preserved.jobs'
shutil.copyfile(ROOT / 'tests/fixtures/enum-compat/schema-9.jobs', project)
info['project_file'] = str(project)
state.write_text(json.dumps(info, ensure_ascii=False, indent=2), encoding='utf-8')
cache = Path(info['local']) / 'updates'
cache.mkdir(parents=True)
sentinel = cache / 'update-smoke.txt'
sentinel.write_text('settings and recovery remain during update', encoding='utf-8')
project_hash = helper.digest(project)
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.CreateMutexW.argtypes = [ctypes.c_void_p, ctypes.c_bool, ctypes.c_wchar_p]
kernel.CreateMutexW.restype = ctypes.c_void_p
kernel.CloseHandle.argtypes = [ctypes.c_void_p]
marker = kernel.CreateMutexW(None, False, 'Local\\ToolMoa.JOBS.Standalone.Running')
if not marker:
    raise ctypes.WinError(ctypes.get_last_error())
try:
    result = subprocess.run([str(args.installer.resolve()), '/S'], timeout=60)
    assert result.returncode == 4, 'Installer overwrote a running-instance installation'
finally:
    kernel.CloseHandle(marker)

child_code = r'''
import ctypes, sys, time
k=ctypes.WinDLL('kernel32',use_last_error=True)
k.CreateFileW.argtypes=[ctypes.c_wchar_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p,ctypes.c_uint32,ctypes.c_uint32,ctypes.c_void_p]
k.CreateFileW.restype=ctypes.c_void_p
k.CloseHandle.argtypes=[ctypes.c_void_p]
h=k.CreateFileW(sys.argv[1],0x80000000,1,None,3,0x80,None)
if h==ctypes.c_void_p(-1).value:raise ctypes.WinError(ctypes.get_last_error())
print('locked',flush=True)
time.sleep(5)
k.CloseHandle(h)
'''
child = subprocess.Popen([sys.executable, '-u', '-c', child_code, str(target / 'JOBS.exe')],
                         stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True,
                         creationflags=subprocess.CREATE_NO_WINDOW)
try:
    assert child.stdout.readline().strip() == 'locked', 'Owned handoff fixture failed to hold application file'
    start = time.monotonic()
    result = subprocess.run([str(args.installer.resolve()), '/S', f'/WAITPID={child.pid}'], timeout=90)
    elapsed = time.monotonic() - start
    assert result.returncode == 0 and child.poll() is not None and elapsed >= 4, 'Installer failed to wait for old process'
finally:
    if child.poll() is None:
        child.terminate()
    child.wait(timeout=10)
assert helper.digest(project) == project_hash and sentinel.read_text() == 'settings and recovery remain during update'
with helper.winreg.OpenKey(helper.winreg.HKEY_CURRENT_USER, helper.UNINSTALL, 0, helper.ACCESS) as key:
    assert Path(helper.winreg.QueryValueEx(key, 'InstallLocation')[0]) == target, 'Update lost custom installation directory'
manifest = json.loads((target / 'package-manifest.json').read_text())
for entry in manifest['files']:
    assert helper.digest(target / entry['path']) == entry['sha256'], entry['path']
lifecycle('uninstall')
result = {'status': 'success', 'source_revision': manifest['source_revision'], 'scope': 'same-build reinstall exercises updater handoff',
          'running_instance_rejected': True, 'wait_pid_seconds': elapsed, 'custom_install_directory_preserved': True,
          'outside_project_preserved': True, 'app_state_preserved_during_update': True, 'full_uninstall_passed': True}
(ROOT / 'build/update-install-result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
print(json.dumps(result, indent=2))
