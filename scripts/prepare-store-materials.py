"""Collect reviewed Store documents, icons and real packaged UI evidence, without fabricating a submission identity."""
import argparse
import json
from pathlib import Path
import shutil
import struct
import zipfile
from windows_payload import inventory, sha256

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--evidence', type=Path, required=True, help='Directory containing lifecycle.json and probe screenshots')
    parser.add_argument('--screenshot', action='append', choices=('01-schedule.png', '02-shared-resources.png'),
                        help='Select desktop-size real screenshots; default: 01-schedule.png')
    parser.add_argument('--output-dir', type=Path, default=ROOT / 'dist/msix/store-preparation')
    args = parser.parse_args()
    evidence = args.evidence.resolve()
    report = json.loads((evidence / 'lifecycle.json').read_text(encoding='utf-8-sig'))
    if report['status'] != 'success' or not report['initial']['packaged'] or not report['upgrade']['packaged']:
        raise ValueError('Successful actual packaged-identity evidence is required')
    output = args.output_dir.resolve()
    output.mkdir(parents=True, exist_ok=False)
    for name in ('listing.ko.md', 'privacy-policy.ko.md', 'submission-checklist.md'):
        shutil.copyfile(ROOT / 'docs/store' / name, output / name)
    shutil.copyfile(ROOT / 'packaging/msix/store-identity.template.json', output / 'store-identity.template.json')
    shutil.copytree(ROOT / 'packaging/msix/Assets', output / 'Assets')
    for name in args.screenshot or ['01-schedule.png']:
        matches = list(evidence.rglob(name))
        if len(matches) != 1:
            raise ValueError('Expected one real packaged screenshot: ' + name)
        png = matches[0].read_bytes()
        width, height = struct.unpack('>II', png[16:24])
        if png[:8] != b'\x89PNG\r\n\x1a\n' or width < 1366 or height < 768 or len(png) > 50 * 1024 * 1024:
            raise ValueError('Screenshot does not meet desktop Store size limits')
        shutil.copyfile(matches[0], output / name)
    shutil.copyfile(evidence / 'lifecycle.json', output / 'development-validation.json')
    status = {'source_revision': report['source_revision'], 'store_identity_supplied': False,
              'store_package_created': False, 'submitted': False, 'store_approved': False, 'wack': 'not run',
              'pending': ['Partner Center registration/name reservation/real identity', 'final Store package',
                          'WACK and target-device validation', 'pricing/markets/ratings/publisher review', 'submission/certification'],
              'files': inventory(output)}
    (output / 'status.json').write_text(json.dumps(status, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    archive = output.with_suffix('.zip')
    if archive.exists():
        raise ValueError('Materials archive already exists')
    with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED) as bundle:
        for file in output.rglob('*'):
            if file.is_file():
                bundle.write(file, file.relative_to(output))
    archive.with_suffix('.sha256.txt').write_text(sha256(archive) + '  ' + archive.name + '\n', encoding='ascii')
    print(json.dumps({'archive': str(archive), 'sha256': sha256(archive), 'status': 'preparation only'}, indent=2))


if __name__ == '__main__':
    main()
