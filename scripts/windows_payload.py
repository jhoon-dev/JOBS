"""Shared self-contained Windows runtime payload; no installer or signing policy."""
import hashlib
from pathlib import Path
import shutil
import subprocess


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def deploy(root, stage, executable, qt, compiler, environment, log):
    stage.mkdir(parents=True, exist_ok=False)
    shutil.copyfile(executable, stage / 'JOBS.exe')
    with log.open('w', encoding='utf-8') as output:
        subprocess.run([str(qt / 'bin/windeployqt.exe'), '--release', '--compiler-runtime', '--no-translations',
                        '--dir', str(stage), str(stage / 'JOBS.exe')], env=environment,
                       stdout=output, stderr=subprocess.STDOUT, check=True)
    for name in ['JOBS.exe', 'Qt6Core.dll', 'Qt6Gui.dll', 'Qt6Widgets.dll', 'Qt6Network.dll', 'platforms/qwindows.dll',
                 'libgcc_s_seh-1.dll', 'libstdc++-6.dll', 'libwinpthread-1.dll']:
        if not (stage / name).is_file():
            raise RuntimeError('Required MinGW runtime missing: ' + name)
    (stage / 'qt.conf').write_text('[Paths]\nPrefix=.\nPlugins=.\n', encoding='utf-8')
    shutil.copyfile(root / 'packaging/NOTICE.txt', stage / 'NOTICE.txt')
    shutil.copytree(root / 'packaging/licenses/qtbase', stage / 'licenses/qtbase')
    shutil.copyfile(qt / 'sbom/qtbase-6.8.3.spdx.json', stage / 'licenses/qtbase/qtbase-6.8.3.spdx.json')
    for name in ('gcc', 'mingw-w64', 'winpthreads'):
        shutil.copytree(compiler.parent / 'licenses' / name, stage / 'licenses/runtime' / name)


def inventory(stage):
    return [{'path': p.relative_to(stage).as_posix(), 'bytes': p.stat().st_size, 'sha256': sha256(p)}
            for p in sorted(stage.rglob('*')) if p.is_file()]
