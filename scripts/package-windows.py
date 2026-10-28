"""Build a per-user NSIS installer from a Release JOBS executable (Windows only)."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import uuid

ROOT = Path(__file__).resolve().parent.parent


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run(command, **kwargs):
    return subprocess.run([str(part) for part in command], check=True, **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    parser.add_argument('--qt-root', type=Path, default=ROOT / '.deps/Qt/6.8.3/mingw_64')
    parser.add_argument('--compiler-bin', type=Path, default=ROOT / '.deps/Qt/Tools/mingw1310_64/bin')
    parser.add_argument('--nsis-root', type=Path, default=ROOT / '.deps/nsis/nsis-3.11')
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'dist')
    args = parser.parse_args()
    if sys.platform != 'win32':
        parser.error('Windows x64 packaging must run on Windows')
    build, qt, compiler, nsis, output = (p.resolve() for p in (
        args.build_dir, args.qt_root, args.compiler_bin, args.nsis_root, args.output_dir))
    environment = os.environ.copy()
    environment['PATH'] = os.pathsep.join((str(qt / 'bin'), str(compiler), environment['PATH']))
    lock = json.loads((ROOT / 'packaging/toolchain-lock.json').read_text())
    if sha256(nsis / 'makensis.exe') != lock['makensis_sha256']:
        raise RuntimeError('NSIS compiler does not match the pinned SHA-256')
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT).decode().strip()
    dirty = subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=normal'], cwd=ROOT)
    if dirty:
        raise RuntimeError('Commit source and packaging changes before producing a release artifact')
    version = re.search(r'project\(JOBS VERSION ([0-9]+\.[0-9]+\.[0-9]+)',
                        (ROOT / 'CMakeLists.txt').read_text())[1]
    tool_version = subprocess.check_output([str(nsis / 'makensis.exe'), '/VERSION']).decode().strip()
    if tool_version != 'v3.11':
        raise RuntimeError('NSIS 3.11 is the pinned packaging compiler')
    deploy_version = subprocess.check_output([str(qt / 'bin/windeployqt.exe'), '--version'], env=environment).decode()
    if '6.8.3' not in deploy_version:
        raise RuntimeError('Qt 6.8.3 is required for this package')
    run(['cmake', '--build', build, '--config', 'Release', '--target', 'JOBS', '--parallel', '3'], env=environment)
    exe = next((p for p in (build / 'Release/JOBS.exe', build / 'JOBS.exe') if p.is_file()), None)
    if exe is None:
        raise RuntimeError('Release JOBS.exe is missing')
    if 'CMAKE_BUILD_TYPE:STRING=Release' not in (build / 'CMakeCache.txt').read_text() and exe.parent.name != 'Release':
        raise RuntimeError('A Release build is required')
    binary = exe.read_bytes()
    pe = struct.unpack_from('<I', binary, 0x3c)[0]
    if binary[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', binary, pe+4)[0] != 0x8664:
        raise RuntimeError('The application must be a Windows x64 executable')
    workspace = build / 'packages' / (revision[:8] + '-' + uuid.uuid4().hex[:8])
    stage = workspace / 'payload'
    stage.mkdir(parents=True)
    output.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(exe, stage / 'JOBS.exe')
    with (workspace / 'windeployqt.log').open('w', encoding='utf-8') as log:
        run([qt / 'bin/windeployqt.exe', '--release', '--compiler-runtime', '--no-translations',
             '--dir', stage, stage / 'JOBS.exe'], env=environment, stdout=log, stderr=subprocess.STDOUT)
    required = ['JOBS.exe', 'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll',
                'platforms/qwindows.dll', 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll']
    for name in required:
        if not (stage / name).is_file():
            raise RuntimeError('Required runtime missing: ' + name)
    (stage / 'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\n', encoding='utf-8')
    for name in ('NOTICE.txt', 'INSTALL-README.txt'):
        shutil.copyfile(ROOT / 'packaging' / name, stage / name)
    shutil.copytree(ROOT / 'packaging/licenses/qtbase', stage / 'licenses/qtbase')
    shutil.copyfile(qt / 'sbom/qtbase-6.8.3.spdx.json', stage / 'licenses/qtbase/qtbase-6.8.3.spdx.json')
    for name in ('gcc', 'mingw-w64', 'winpthreads'):
        shutil.copytree(compiler.parent / 'licenses' / name, stage / 'licenses/runtime' / name)
    (stage / 'licenses/nsis').mkdir(parents=True)
    shutil.copyfile(nsis / 'COPYING', stage / 'licenses/nsis/COPYING.txt')
    (stage / 'install-info.ini').write_text(
        f'[JOBS]\nProductId=ToolMoa.JOBS\nVersion={version}\nSourceRevision={revision}\n', encoding='ascii')
    inventory = [{'path': p.relative_to(stage).as_posix(), 'bytes': p.stat().st_size, 'sha256': sha256(p)}
                 for p in sorted(stage.rglob('*')) if p.is_file()]
    manifest = {'product': 'JOBS', 'version': version, 'channel': 'preview', 'architecture': 'x64',
                'source_revision': revision, 'signed': False, 'qt': '6.8.3', 'nsis': tool_version,
                'application_sha256': sha256(stage / 'JOBS.exe'), 'files': inventory,
                'inventory_excludes': ['package-manifest.json', 'Uninstall.exe (generated at install time)']}
    (stage / 'package-manifest.json').write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')
    source = workspace / 'installer.nsi'
    source.write_text((ROOT / 'packaging/windows-installer.nsi').read_text(encoding='utf-8'), encoding='utf-8-sig')
    installer = output / f'JOBS-{version}-Setup-win64-{revision[:8]}.exe'
    with (workspace / 'nsis.log').open('w', encoding='utf-8') as log:
        run([nsis / 'makensis.exe', '/V3', f'/DAppVersion={version}', f'/DStageDir={stage}',
             f'/DInstallerPath={installer}',
             f'/DInstalledSizeKiB={sum(p.stat().st_size for p in stage.rglob("*") if p.is_file()) // 1024}', source],
            stdout=log, stderr=subprocess.STDOUT)
    summary = {'installer': str(installer), 'bytes': installer.stat().st_size, 'sha256': sha256(installer),
               'source_revision': revision, 'version': version, 'stage': str(stage), 'logs': str(workspace),
               'signed': False}
    installer.with_suffix('.sha256.txt').write_text(summary['sha256']+'  '+installer.name+'\n', encoding='ascii')
    installer.with_suffix('.json').write_text(json.dumps(summary, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    main()
