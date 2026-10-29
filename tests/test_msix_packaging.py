"""Independent packaging guard regressions; run with python -m unittest discover -s tests -p test_msix_packaging.py."""
import hashlib
import json
from pathlib import Path
import sys
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from msix_package import (DEV_OID, FOUNDATION, RESCAP, identity, manifest, validate_assets,
                          validate_payload, verify_archive, version)
from windows_payload import inventory


class PackagingContract(unittest.TestCase):
    def setUp(self):
        self.config = {'IdentityName': '12345.AccountJOBS', 'Publisher': 'CN=12345678-1234-1234-1234-123456789012',
                       'PublisherDisplayName': '검증 전용 & Test', 'DisplayName': 'JOBS'}

    def test_store_requires_exact_nonempty_identity(self):
        for config in (None, {}, {**self.config, 'Publisher': ''}, {**self.config, 'unexpected': 'x'},
                       {**self.config, 'IdentityName': 'placeholder'},
                       {**self.config, 'Publisher': 'CN=Test, ' + DEV_OID},
                       {**self.config, 'IdentityName': ' name'}, {**self.config, 'Publisher': 'CN='}):
            with self.subTest(config=config), self.assertRaises(ValueError):
                identity('store', config)
        self.assertEqual(identity('store', self.config), self.config)

    def test_development_identity_separate_and_fixed(self):
        dev = identity('development')
        self.assertIn(DEV_OID, dev['Publisher'])
        self.assertNotEqual(dev['IdentityName'], self.config['IdentityName'])
        with self.assertRaises(ValueError):
            identity('development', self.config)

    def test_version_bounds(self):
        for value in ('1.0', '-1.0.0.0', '1.0.0.1', '1.65536.0.0', '1.0.0.0\n', '1.0.0.x'):
            with self.subTest(value=value), self.assertRaises(ValueError):
                version(value)
        self.assertEqual(version('1.65535.65535.0'), '1.65535.65535.0')

    def test_manifest_identity_xml_and_capability(self):
        root = ET.fromstring(manifest(self.config, '1.0.0.0'))
        ns = {'f': FOUNDATION, 'r': RESCAP}
        self.assertEqual(root.find('f:Identity', ns).get('Publisher'), self.config['Publisher'])
        self.assertEqual(root.find('f:Properties/f:PublisherDisplayName', ns).text,
                         self.config['PublisherDisplayName'])
        self.assertEqual([c.get('Name') for c in root.find('f:Capabilities', ns)], ['runFullTrust'])
        self.assertEqual(len(root.find('f:Applications', ns)), 1)
        dev = ET.fromstring(manifest(identity('development'), '1.0.0.0', True))
        self.assertEqual(len(dev.find('f:Applications', ns)), 2)

    def test_assets_match_vector_inventory(self):
        validate_assets(ROOT / 'packaging/msix/Assets')

    def test_payload_rejects_secrets_installer_and_store_probe(self):
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            (folder / 'JOBS.exe').write_bytes(b'executable')
            (folder / 'AppxManifest.xml').write_bytes(b'manifest')
            validate_payload(folder, 'store')
            for name in ('key.pfx', 'Uninstall.exe', 'app-Setup.exe', 'jobs_msix_probe.exe', 'run.ps1'):
                with self.subTest(name=name):
                    path = folder / name
                    path.write_bytes(b'bad')
                    with self.assertRaises(ValueError):
                        validate_payload(folder, 'store')
                    path.unlink()
            with self.assertRaises(ValueError):
                validate_payload(folder, 'store', probe=True)

    def test_archive_roundtrip_detects_corruption_and_unexpected_content(self):
        xml = manifest(identity('development'), '1.0.0.0')
        with tempfile.TemporaryDirectory() as temporary:
            folder = Path(temporary)
            payload = folder / 'payload'
            payload.mkdir()
            (payload / 'JOBS.exe').write_bytes(b'executable')
            (payload / 'AppxManifest.xml').write_bytes(xml)
            files = inventory(payload)
            for modification in ('none', 'corrupt', 'extra', 'traversal', 'duplicate'):
                with self.subTest(modification=modification):
                    package = folder / (modification + '.msix')
                    with zipfile.ZipFile(package, 'w') as archive:
                        archive.writestr('AppxManifest.xml', xml)
                        archive.writestr('JOBS.exe', b'bad' if modification == 'corrupt' else b'executable')
                        archive.writestr('AppxBlockMap.xml', b'footprint')
                        if modification == 'extra':
                            archive.writestr('unlisted.dll', b'bad')
                        if modification == 'traversal':
                            archive.writestr('../outside', b'bad')
                        if modification == 'duplicate':
                            archive.writestr('jobs.exe', b'bad')
                    if modification == 'none':
                        verify_archive(package, files, xml)
                    else:
                        with self.assertRaises(ValueError):
                            verify_archive(package, files, xml)


if __name__ == '__main__':
    unittest.main()
