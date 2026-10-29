"""Download pinned Microsoft SDK build tools without installing or changing Windows policy."""
import argparse
import base64
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import shutil
import sys
import urllib.request
import uuid
import zipfile

ROOT = Path(__file__).resolve().parents[1]


def microsoft_signature(path):
    command = "$ErrorActionPreference='Stop'; $s=Get-AuthenticodeSignature -LiteralPath $env:JOBS_SIGNED_TOOL; $s | Select-Object Status,@{Name='Subject';Expression={$_.SignerCertificate.Subject}} | ConvertTo-Json -Compress"
    import os
    env = os.environ.copy()
    # Windows PowerShell must not inherit PowerShell 7's incompatible module search path.
    env.pop('PSModulePath', None)
    env['JOBS_SIGNED_TOOL'] = str(path)
    shell = shutil.which('pwsh.exe') or 'powershell.exe'
    if shell == 'powershell.exe':
        env['PSModulePath'] = str(Path(env['SystemRoot']) / 'System32/WindowsPowerShell/v1.0/Modules')
    result = subprocess.run([shell, '-NoProfile', '-NonInteractive', '-Command', command],
                            capture_output=True, text=True, env=env, timeout=30)
    if result.returncode:
        raise RuntimeError(result.stderr)
    signature = json.loads(result.stdout)
    # PowerShell serializes SignatureStatus.Valid as integer 0.
    if signature['Status'] != 0 or 'O=Microsoft Corporation' not in signature.get('Subject', ''):
        raise RuntimeError(f'Microsoft Authenticode validation failed: {path}: {signature}')
    return signature


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=ROOT / '.deps/msix-sdk')
    args = parser.parse_args()
    if sys.platform != 'win32':
        parser.error('Microsoft SDK executable tools require Windows')
    lock = json.loads((ROOT / 'packaging/msix-toolchain-lock.json').read_text())
    destination = args.output_dir.resolve() / lock['version']
    metadata = destination / 'tool-info.json'
    if metadata.is_file():
        info = json.loads(metadata.read_text())
        tool = destination / info['makeappx_relative']
        if hashlib.sha256(tool.read_bytes()).hexdigest() != lock['makeappx_sha256']:
            raise RuntimeError('Existing SDK tool differs from its verified inventory')
        microsoft_signature(tool)
        print(str(tool))
        return
    if destination.exists():
        raise RuntimeError('Incomplete SDK destination exists; select a fresh --output-dir')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    archive = args.output_dir / f"sdk-{lock['version']}-{uuid.uuid4().hex[:8]}.nupkg"
    digest = hashlib.sha512()
    size = 0
    with urllib.request.urlopen(lock['url'], timeout=90) as response, archive.open('xb') as output:
        while chunk := response.read(1024 * 1024):
            size += len(chunk)
            if size > lock['max_archive_bytes']:
                raise RuntimeError('SDK archive exceeds pinned size bound')
            digest.update(chunk)
            output.write(chunk)
    if base64.b64encode(digest.digest()).decode() != lock['sha512_base64']:
        raise RuntimeError('SDK archive differs from published NuGet SHA-512')
    with zipfile.ZipFile(archive) as package:
        entries = package.infolist()
        if len(entries) > 20000 or sum(item.file_size for item in entries) > lock['max_unpacked_bytes']:
            raise RuntimeError('SDK extraction limits exceeded')
        for item in entries:
            path = PurePosixPath(item.filename.replace('\\', '/'))
            if path.is_absolute() or '..' in path.parts or any(':' in part for part in path.parts):
                raise RuntimeError('Unsafe SDK archive path')
        package.extractall(destination)
    matches = list(destination.glob('bin/*/x64/makeappx.exe'))
    if len(matches) != 1:
        raise RuntimeError(f'Expected one x64 MakeAppx, found {matches}')
    tool = matches[0]
    if hashlib.sha256(tool.read_bytes()).hexdigest() != lock['makeappx_sha256']:
        raise RuntimeError('MakeAppx differs from the pinned tool hash')
    signature = microsoft_signature(tool)
    info = {'package_version': lock['version'], 'archive_url': lock['url'],
            'archive_sha512_base64': lock['sha512_base64'], 'archive_bytes': size,
            'makeappx_relative': tool.relative_to(destination).as_posix(),
            'makeappx_sha256': hashlib.sha256(tool.read_bytes()).hexdigest(),
            'authenticode': signature}
    metadata.write_text(json.dumps(info, indent=2) + '\n', encoding='utf-8')
    print(str(tool))


if __name__ == '__main__':
    main()
