"""Rasterize the repository's simple vector icon for required MSIX/Store sizes."""
from pathlib import Path
import hashlib
import json
import xml.etree.ElementTree as ET
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
folder = ROOT / 'packaging/msix/Assets'
svg = ET.fromstring((folder / 'JOBS.svg').read_bytes())
files = {}
for name, size in [('Square44x44Logo.png', 44), ('Square150x150Logo.png', 150),
                   ('StoreLogo.png', 50), ('StoreListing300.png', 300)]:
    scale = size * 4 / 256
    canvas = Image.new('RGBA', (size * 4, size * 4))
    draw = ImageDraw.Draw(canvas)
    for rect in svg:
        if rect.tag.rsplit('}', 1)[-1] != 'rect':
            raise ValueError('Unsupported vector primitive')
        x, y, width, height = (float(rect.attrib[key]) * scale for key in ('x', 'y', 'width', 'height'))
        draw.rounded_rectangle((x, y, x + width, y + height), radius=float(rect.attrib['rx']) * scale,
                               fill=rect.attrib['fill'])
    canvas = canvas.resize((size, size), Image.Resampling.LANCZOS)
    path = folder / name
    canvas.save(path, optimize=False)
    files[name] = {'width': size, 'height': size, 'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}
(folder / 'assets.json').write_text(json.dumps({'source': 'JOBS.svg', 'generator': 'Pillow 11.3.0', 'files': files}, indent=2) + '\n')
