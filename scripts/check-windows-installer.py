"""Manual native acceptance helper; refuses to touch existing JOBS user data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
import uuid
import winreg

ROOT = Path(__file__).resolve().parent.parent
UNINSTALL = r'Software\Microsoft\Windows\CurrentVersion\Uninstall\ToolMoa.JOBS'
SETTINGS = r'Software\ToolMoa\JOBS'
ACCESS = winreg.KEY_READ | winreg.KEY_WOW64_64KEY


def exists(key):
    try:
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, key, 0, ACCESS):
            return True
    except FileNotFoundError:
        return False


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def installer_command(executable, target):
    # NSIS /D= consumes the final, unquoted remainder. No shell is involved.
    assert '"' not in str(executable) + str(target)
    return f'"{executable}" /S /D={target}'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=('install', 'launch', 'uninstall'))
    parser.add_argument('--installer', type=Path)
    parser.add_argument('--state', type=Path, default=ROOT / 'build/windows-installer-smoke.json')
    args = parser.parse_args()
    if args.mode == 'install':
        assert args.installer and args.installer.is_file()
        assert not exists(UNINSTALL), 'An installed JOBS already exists; use a clean test account'
        settings_existed = exists(SETTINGS)
        if settings_existed:
            with winreg.OpenKey(winreg.HKEY_CURRENT_USER, SETTINGS, 0, ACCESS) as key:
                children, values, _ = winreg.QueryInfoKey(key)
                assert children == values == 0, 'Existing JOBS settings must be preserved; use a clean test account'
        local = Path(os.environ['LOCALAPPDATA']) / 'ToolMoa/JOBS'
        roaming = Path(os.environ['APPDATA']) / 'ToolMoa/JOBS'
        assert not local.exists() and not roaming.exists(), 'Existing JOBS data must be preserved; use a clean test account'
        case = ROOT / 'build' / ('installer-smoke-' + uuid.uuid4().hex[:8])
        protected = case / 'protected/JOBS'
        protected.mkdir(parents=True)
        keep = protected / 'keep.txt'; keep.write_text('unrelated data', encoding='utf-8')
        result = subprocess.run(installer_command(args.installer.resolve(), protected), shell=False, timeout=90)
        assert result.returncode == 3 and keep.read_text() == 'unrelated data'
        assert not (protected / 'JOBS.exe').exists() and not exists(UNINSTALL)
        target = case / '설치 확인/JOBS'
        result = subprocess.run(installer_command(args.installer.resolve(), target), shell=False, timeout=120)
        assert result.returncode == 0 and (target / 'JOBS.exe').is_file()
        manifest = json.loads((target / 'package-manifest.json').read_text(encoding='utf-8'))
        for entry in manifest['files']:
            assert digest(target / entry['path']) == entry['sha256'], entry['path']
        with winreg.OpenKey(winreg.HKEY_CURRENT_USER, UNINSTALL, 0, ACCESS) as key:
            assert Path(winreg.QueryValueEx(key, 'InstallLocation')[0]) == target
            assert winreg.QueryValueEx(key, 'UninstallString')[0] == f'"{target}\\Uninstall.exe"'
        state = {'case': str(case), 'target': str(target), 'local': str(local), 'roaming': str(roaming),
                 'installer': str(args.installer.resolve()), 'installer_sha256': digest(args.installer),
                 'settings_key_existed_empty': settings_existed, 'inventory_files_verified': len(manifest['files']),
                 'source_revision': manifest['source_revision'], 'protected_nonempty_directory_rejected': True}
    else:
        state = json.loads(args.state.read_text(encoding='utf-8'))
        target, case = Path(state['target']), Path(state['case'])
        assert target.resolve().is_relative_to((ROOT / 'build').resolve()) and target.name == 'JOBS'
        if args.mode == 'launch':
            environment = os.environ.copy()
            environment['PATH'] = os.pathsep.join((str(Path(environment['SystemRoot']) / 'System32'), environment['SystemRoot']))
            for name in list(environment):
                if name.startswith(('QT_', 'QML_')) or name in ('QML2_IMPORT_PATH',):
                    environment.pop(name)
            with (case / 'application.log').open('wb') as log:
                process = subprocess.Popen([str(target / 'JOBS.exe')], cwd=target, env=environment,
                                           stdout=log, stderr=subprocess.STDOUT)
            time.sleep(2)
            assert process.poll() is None, (case / 'application.log').read_text(errors='replace')
            state.update({'application_pid': process.pid, 'development_path_removed': True,
                          'project_file': str(case / '설치 확인 프로젝트.jobs')})
        else:
            project = Path(state['project_file'])
            assert project.is_file(), 'Create/save/reopen the project in the native app before uninstall'
            project_hash = digest(project)
            # The test owns all these paths: data roots were absent before launch.
            for name in ('local', 'roaming'):
                data = Path(state[name]); assert data == Path(os.environ['LOCALAPPDATA' if name == 'local' else 'APPDATA']) / 'ToolMoa/JOBS'
                for child in ('cache', 'recovery'):
                    (data / child).mkdir(parents=True, exist_ok=True)
                    (data / child / 'installer-smoke.txt').write_text('synthetic data', encoding='utf-8')
            sibling = Path(state['local']).parent / ('InstallerSibling-' + uuid.uuid4().hex[:8])
            sibling.mkdir(); (sibling / 'keep.txt').write_text('other application data', encoding='utf-8')
            (target / 'extra-user-file.txt').write_text('full cleanup test', encoding='utf-8')
            with winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, SETTINGS, 0, winreg.KEY_WRITE | winreg.KEY_WOW64_64KEY) as key:
                winreg.SetValueEx(key, 'InstallerSmokeOnly', 0, winreg.REG_SZ, 'synthetic setting')
            result = subprocess.run([str(target / 'Uninstall.exe'), '/S'], timeout=90)
            assert result.returncode == 0
            for _ in range(100):
                if not target.exists() and not exists(UNINSTALL):
                    break
                time.sleep(0.2)
            assert not target.exists(), 'Installation directory was not fully removed'
            assert not Path(state['local']).exists() and not Path(state['roaming']).exists()
            assert not exists(SETTINGS) and not exists(UNINSTALL)
            assert digest(project) == project_hash
            assert (sibling / 'keep.txt').read_text() == 'other application data'
            # Remove only the synthetic sibling; restore the pre-test empty settings key.
            assert sibling.parent == Path(state['local']).parent and sibling.name.startswith('InstallerSibling-')
            (sibling / 'keep.txt').unlink(); sibling.rmdir()
            if state['settings_key_existed_empty']:
                winreg.CreateKeyEx(winreg.HKEY_CURRENT_USER, SETTINGS, 0,
                                   winreg.KEY_WRITE | winreg.KEY_WOW64_64KEY).Close()
            state.update({'full_install_directory_removed': True, 'local_roaming_cache_recovery_removed': True,
                          'settings_and_uninstall_registration_removed': True, 'outside_project_sha256': project_hash,
                          'outside_project_preserved': True, 'other_app_data_preserved': True,
                          'pretest_empty_settings_key_restored': state['settings_key_existed_empty']})
    args.state.write_text(json.dumps(state, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps(state, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
