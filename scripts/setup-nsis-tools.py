"""Install the repository's pinned NSIS compiler into a private tools folder."""
import hashlib
import json
from pathlib import Path, PurePosixPath
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
lock = json.loads((ROOT / 'packaging/toolchain-lock.json').read_text())
folder = ROOT / '.deps/nsis'
tool = folder / ('nsis-' + lock['nsis_version']) / 'makensis.exe'
if not tool.exists():
    folder.mkdir(parents=True, exist_ok=True)
    archive = folder / ('nsis-' + lock['nsis_version'] + '.zip')
    if not archive.exists():
        with urllib.request.urlopen(lock['download_mirror'], timeout=60) as response, archive.open('xb') as output:
            size = 0
            while chunk := response.read(1024 * 1024):
                size += len(chunk)
                if size > 30 * 1024 * 1024:
                    raise ValueError('NSIS archive exceeds size bound')
                output.write(chunk)
    if hashlib.sha256(archive.read_bytes()).hexdigest() != lock['archive_sha256']:
        raise ValueError('NSIS archive hash differs from pinned supplier checksum')
    if tool.parent.exists():
        raise ValueError('Incomplete NSIS destination exists')
    with zipfile.ZipFile(archive) as package:
        entries = package.infolist()
        if len(entries) > 20000 or sum(x.file_size for x in entries) > 150 * 1024 * 1024:
            raise ValueError('NSIS extraction limits exceeded')
        for entry in entries:
            path = PurePosixPath(entry.filename.replace('\\', '/'))
            if path.is_absolute() or '..' in path.parts or ':' in entry.filename or path.parts[0] != tool.parent.name:
                raise ValueError('Unsafe NSIS archive path')
        package.extractall(folder)
if hashlib.sha256(tool.read_bytes()).hexdigest() != lock['makensis_sha256']:
    raise ValueError('NSIS compiler hash differs from lock')
print(tool)
