"""Render the existing JOBS vector brand as native desktop icon containers."""
from pathlib import Path
import hashlib
import json
import xml.etree.ElementTree as ET
from PIL import Image, ImageDraw
from PIL import __version__

if __version__ != '11.3.0':
    raise RuntimeError('Use Pillow 11.3.0 from scripts/msix-asset-requirements.txt')

root = Path(__file__).resolve().parents[1]
source = root / 'packaging/msix/Assets/JOBS.svg'
folder = root / 'packaging/icons'
folder.mkdir(exist_ok=True)
svg = ET.fromstring(source.read_bytes())
image = Image.new('RGBA', (4096, 4096))
draw = ImageDraw.Draw(image)
scale = 4096 / 256
for item in svg:
    if item.tag.rsplit('}', 1)[-1] != 'rect':
        raise ValueError('Unsupported vector primitive')
    x, y, width, height = [float(item.attrib[key])*scale for key in ('x', 'y', 'width', 'height')]
    draw.rounded_rectangle((x, y, x+width, y+height), radius=float(item.attrib['rx'])*scale,
                           fill=item.attrib['fill'])
image = image.resize((1024, 1024), Image.Resampling.LANCZOS)
image.save(folder / 'JOBS.png')
image.resize((256, 256), Image.Resampling.LANCZOS).save(folder / 'JOBS.ico',
    bitmap_format='bmp',
    sizes=[(size, size) for size in (16, 20, 24, 32, 40, 48, 64, 96, 128, 256)])
image.save(folder / 'JOBS.icns')
(folder / 'icons.json').write_text(json.dumps({
    'source': source.relative_to(root).as_posix(),
    'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
    'generator': 'Pillow 11.3.0',
    'files': {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
              for path in sorted(folder.iterdir()) if path.suffix in ('.png', '.ico', '.icns')}
}, indent=2)+'\n', encoding='utf-8')
print('Generated JOBS PNG, multi-resolution ICO and ICNS from existing SVG')
