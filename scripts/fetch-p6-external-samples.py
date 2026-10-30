"""Fetch checksum-pinned public samples into an ignored validation directory only."""
import argparse
import hashlib
import json
from pathlib import Path
from urllib.request import urlopen

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output', type=Path, required=True)
args = parser.parse_args()
output = args.output.resolve()
workspace = Path(__file__).resolve().parents[1]
if not output.is_relative_to(workspace/'build'):
    raise SystemExit('Use a fresh directory below the ignored build directory')
manifest = json.loads((workspace/'tests/fixtures/p6-public/external-manifest.json').read_text(encoding='utf-8'))
output.mkdir(parents=True, exist_ok=False)
for entry in manifest['files']:
    if Path(entry['file']).name != entry['file']:
        raise SystemExit('Invalid manifest filename')
    with urlopen(entry['url'], timeout=30) as response:
        data = response.read(16*1024*1024+1)
    if len(data) != entry['bytes'] or hashlib.sha256(data).hexdigest() != entry['sha256']:
        raise SystemExit('Public sample size or checksum changed: '+entry['file'])
    (output/entry['file']).write_bytes(data)
(output/'manifest.json').write_text(json.dumps(manifest, indent=2)+'\n', encoding='utf-8')
print('Fetched two unmodified pinned samples; raw files are not release assets.')
