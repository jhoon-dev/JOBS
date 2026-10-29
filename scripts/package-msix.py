"""Create a development MSIX, or an unsigned Store package using explicit Partner Center identity."""
import argparse
import json
import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import sys
import uuid
from windows_payload import deploy, inventory, sha256
from msix_package import ASSETS, identity, manifest, validate_assets, validate_payload, verify_archive, version

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--mode', choices=('development', 'store'), default='development')
    parser.add_argument('--identity', type=Path)
    parser.add_argument('--version')
    parser.add_argument('--include-probe', action='store_true')
    parser.add_argument('--build-dir', type=Path, default=ROOT / 'build')
    parser.add_argument('--qt-root', type=Path, default=ROOT / '.deps/Qt/6.8.3/mingw_64')
    parser.add_argument('--compiler-bin', type=Path, default=ROOT / '.deps/Qt/Tools/mingw1310_64/bin')
    parser.add_argument('--sdk-root', type=Path, default=ROOT / '.deps/msix-sdk')
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'dist/msix')
    args = parser.parse_args()
    config = identity(args.mode, json.loads(args.identity.read_text(encoding='utf-8-sig')) if args.identity else None)
    if args.include_probe and args.mode != 'development':
        parser.error('--include-probe is allowed only in development packages')
    package_version = version(args.version or '1.0.0.0')
    if args.mode == 'store' and int(package_version.split('.')[0]) == 0:
        parser.error('Store package major version must be positive')
    if sys.platform != 'win32':
        parser.error('MakeAppx requires Windows')
    build, qt, compiler, output = (p.resolve() for p in (args.build_dir, args.qt_root, args.compiler_bin, args.output_dir))
    lock = json.loads((ROOT / 'packaging/msix-toolchain-lock.json').read_text())
    sdk = args.sdk_root.resolve() / lock['version']
    tools = json.loads((sdk / 'tool-info.json').read_text())
    makeappx = sdk / tools['makeappx_relative']
    if sha256(makeappx) != lock['makeappx_sha256']:
        raise RuntimeError('MakeAppx differs from pinned Microsoft SDK tool')
    if tools['archive_sha512_base64'] != lock['sha512_base64']:
        raise RuntimeError('SDK archive identity differs from lock')
    revision = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    if subprocess.check_output(['git', 'status', '--porcelain', '--untracked-files=normal'], cwd=ROOT):
        raise RuntimeError('Commit source before producing a package')
    environment = os.environ.copy()
    environment['PATH'] = os.pathsep.join((str(qt / 'bin'), str(compiler), environment['PATH']))
    target = ['JOBS'] + (['jobs_msix_probe'] if args.include_probe else [])
    subprocess.run(['cmake', '--build', str(build), '--config', 'Release', '--parallel', '3', '--target', *target],
                   check=True, env=environment)
    if 'CMAKE_BUILD_TYPE:STRING=Release' not in (build / 'CMakeCache.txt').read_text():
        raise RuntimeError('This package pipeline requires a Release MinGW/Ninja build')
    for executable in ['JOBS.exe'] + (['jobs_msix_probe.exe'] if args.include_probe else []):
        binary = (build / executable).read_bytes()
        pe = struct.unpack_from('<I', binary, 0x3c)[0]
        if binary[pe:pe+4] != b'PE\0\0' or struct.unpack_from('<H', binary, pe+4)[0] != 0x8664:
            raise RuntimeError('The application must be a Windows x64 executable')
    deploy_version = subprocess.check_output([str(qt / 'bin/windeployqt.exe'), '--version'], env=environment).decode()
    if '6.8.3' not in deploy_version:
        raise RuntimeError('Qt 6.8.3 required')
    workspace = build / 'packages' / ('msix-' + revision[:8] + '-' + uuid.uuid4().hex[:8])
    stage = workspace / 'payload'
    deploy(ROOT, stage, build / 'JOBS.exe', qt, compiler, environment, workspace / 'windeployqt.log')
    shutil.copyfile(ROOT / 'packaging/msix/MSIX-README.txt', stage / 'INSTALL-README.txt')
    validate_assets(ROOT / 'packaging/msix/Assets')
    (stage / 'Assets').mkdir()
    for name in ASSETS:
        shutil.copyfile(ROOT / 'packaging/msix/Assets' / name, stage / 'Assets' / name)
    if args.include_probe:
        shutil.copyfile(build / 'jobs_msix_probe.exe', stage / 'jobs_msix_probe.exe')
        shutil.copyfile(qt / 'bin/Qt6Test.dll', stage / 'Qt6Test.dll')
    xml = manifest(config, package_version, args.include_probe)
    (stage / 'AppxManifest.xml').write_bytes(xml)
    validate_payload(stage, args.mode, args.include_probe)
    files = inventory(stage)
    output.mkdir(parents=True, exist_ok=True)
    name = f"JOBS-{args.mode}-{package_version}-x64-{revision[:8]}" + ('-probe' if args.include_probe else '')
    package = output / (name + '.msix')
    if package.exists():
        raise RuntimeError('Package already exists; select a new --output-dir or version')
    with (workspace / 'makeappx.log').open('w', encoding='utf-8') as log:
        subprocess.run([str(makeappx), 'pack', '/d', str(stage), '/p', str(package), '/v'],
                       check=True, stdout=log, stderr=subprocess.STDOUT)
    verify_archive(package, files, xml)
    result = {'product': 'JOBS', 'source_revision': revision, 'mode': args.mode, 'version': package_version,
              'architecture': 'x64', 'identity': config, 'signed': False, 'submitted': False, 'store_approved': False,
              'store_identity_supplied': args.mode == 'store', 'test_probe_included': args.include_probe,
              'package': str(package), 'bytes': package.stat().st_size, 'sha256': sha256(package),
              'stage': str(stage), 'logs': str(workspace), 'sdk': tools, 'files': files,
              'verification': ['MakeAppx schema/semantic validation', 'independent ZIP/manifest/file SHA roundtrip'],
              'next_step': 'Partner Center certification' if args.mode == 'store' else 'Development validation only; reserve Store identity before submission'}
    package.with_suffix('.json').write_text(json.dumps(result, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    package.with_suffix('.sha256.txt').write_text(result['sha256'] + '  ' + package.name + '\n', encoding='ascii')
    print(json.dumps({k: result[k] for k in ['package', 'mode', 'version', 'bytes', 'sha256', 'source_revision', 'next_step']}, indent=2))


if __name__ == '__main__':
    main()
