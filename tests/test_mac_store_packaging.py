"""Admission checks before any release credentials or Apple upload are used."""
import copy
from datetime import datetime, timedelta, timezone
import importlib.util
import json
from pathlib import Path
import stat
import tempfile
import unittest
from unittest.mock import call, patch
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('mac_store', ROOT / 'scripts/package-mac-store.py')
mac_store = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mac_store)


class MacStorePackagingTests(unittest.TestCase):
    def setUp(self):
        self.profile = {
            'TeamIdentifier': [mac_store.TEAM],
            'ExpirationDate': datetime.now(timezone.utc) + timedelta(days=2),
            'Entitlements': {'com.apple.application-identifier': mac_store.TEAM + '.' + mac_store.BUNDLE,
                             'com.apple.developer.team-identifier': mac_store.TEAM},
            'DeveloperCertificates': [b'distribution certificate'],
        }

    def test_exact_distribution_profile(self):
        self.assertEqual(len(mac_store.validate_profile(self.profile)), 1)
        self.profile['ExpirationDate'] = self.profile['ExpirationDate'].replace(tzinfo=None)
        mac_store.validate_profile(self.profile)

    def test_preparation_uses_app_status_with_separate_sandbox_evidence(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            bundle = root / 'dist/mac-store-preparation'
            bundle.mkdir(parents=True)
            archive = bundle / 'PPM-unsigned-universal.app.zip'
            archive.write_bytes(b'archive')
            status = {'source_revision': 'app-revision', 'sandbox_container_verified': True}
            (bundle / 'status.json').write_text(json.dumps(status))
            sandbox = root / 'build-mac-store/sandbox-validation'
            sandbox.mkdir(parents=True)
            (sandbox / 'status.json').write_text(json.dumps({'source_revision': 'probe-revision'}))
            self.assertEqual(mac_store.preparation_inputs(root), (status, archive))

    def test_preparation_rejects_unpaired_status_and_duplicate_app_archives(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'status.json').write_text('{}')
            bundle = root / 'app'
            bundle.mkdir()
            (bundle / 'PPM-unsigned-universal.app.zip').write_bytes(b'archive')
            with self.assertRaisesRegex(ValueError, 'paired status'):
                mac_store.preparation_inputs(root)
            (bundle / 'status.json').write_text('{}')
            (root / 'PPM-unsigned-universal.app.zip').write_bytes(b'another archive')
            with self.assertRaisesRegex(ValueError, 'one verified preparation app archive'):
                mac_store.preparation_inputs(root)

    def test_wrong_team_app_wildcard_and_debug_profiles(self):
        for field, value in [('com.apple.application-identifier', mac_store.TEAM + '.*'),
                             ('com.apple.application-identifier', mac_store.TEAM + '.another.app'),
                             ('com.apple.developer.team-identifier', 'OTHER'),
                             ('get-task-allow', True), ('com.apple.security.get-task-allow', True)]:
            with self.subTest(field=field, value=value):
                bad = copy.deepcopy(self.profile)
                bad['Entitlements'][field] = value
                with self.assertRaises(ValueError):
                    mac_store.validate_profile(bad)
        for field, value in [('TeamIdentifier', ['OTHER']), ('ProvisionedDevices', ['a-device']),
                             ('ProvisionsAllDevices', True), ('DeveloperCertificates', [])]:
            bad = copy.deepcopy(self.profile)
            bad[field] = value
            with self.assertRaises(ValueError):
                mac_store.validate_profile(bad)

    def test_expired_and_ambiguous_profile(self):
        self.profile['ExpirationDate'] = datetime.now(timezone.utc) - timedelta(seconds=1)
        with self.assertRaises(ValueError):
            mac_store.validate_profile(self.profile)
        self.setUp()
        self.profile['Entitlements']['application-identifier'] = mac_store.TEAM + '.other'
        with self.assertRaises(ValueError):
            mac_store.validate_profile(self.profile)

    def test_identity_must_be_the_profile_distribution_certificate(self):
        line = '1) ' + 'A' * 40 + f' "Apple Distribution: Publisher ({mac_store.TEAM})"'
        with self.assertRaises(ValueError):
            mac_store.identity(line, ('Apple Distribution:',), {'B' * 40})
        self.assertIn(mac_store.TEAM, mac_store.identity(line, ('Apple Distribution:',), {'A' * 40}))
        with self.assertRaises(ValueError):
            mac_store.identity(line + '\n' + line, ('Apple Distribution:',), {'A' * 40})

    def test_signing_keychain_preserves_and_restores_search_list_on_failure(self):
        previous = '    "/Users/runner/Library/Keychains/login.keychain-db"\n    "/path with spaces/extra.keychain-db"\n'
        original = ['/Users/runner/Library/Keychains/login.keychain-db', '/path with spaces/extra.keychain-db']
        for fails in (False, True):
            with self.subTest(fails=fails), patch.object(mac_store, 'run', side_effect=[previous, '', '']) as run:
                try:
                    with mac_store.signing_keychain_search('/tmp/signing.keychain-db'):
                        if fails:
                            raise ValueError('Signing failed')
                except ValueError:
                    self.assertTrue(fails)
                self.assertEqual(run.call_args_list, [
                    call(['security', 'list-keychains', '-d', 'user']),
                    call(['security', 'list-keychains', '-d', 'user', '-s', '/tmp/signing.keychain-db', *original]),
                    call(['security', 'list-keychains', '-d', 'user', '-s', *original]),
                ])

    def test_upload_requires_exact_source_runtime_and_external_qa(self):
        good = {'source_revision': 'revision', 'qt_appstore_compliant': True, 'sandbox_container_verified': True}
        mac_store.validate_preparation(good, 'revision', False, False)
        mac_store.validate_preparation(good, 'revision', True, True)
        with self.assertRaises(ValueError):
            mac_store.validate_preparation(good, 'revision', True, False)
        for key, value in [('source_revision', 'other'), ('qt_appstore_compliant', False),
                           ('sandbox_container_verified', False)]:
            bad = {**good, key: value}
            with self.assertRaises(ValueError):
                mac_store.validate_preparation(bad, 'revision', True, True)

    def test_archive_rejects_traversal_and_external_symlink(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            for index, name in enumerate(('../outside', '/outside')):
                archive = root / f'bad-{index}.zip'
                with zipfile.ZipFile(archive, 'w') as z:
                    z.writestr(name, b'bad')
                with self.assertRaises(ValueError):
                    mac_store.validate_archive(archive, root)

            for index, target in enumerate(('../../outside', '/outside')):
                archive = root / f'link-{index}.zip'
                link = zipfile.ZipInfo('PPM.app/Contents/link')
                link.create_system = 3
                link.external_attr = (stat.S_IFLNK | 0o777) << 16
                with zipfile.ZipFile(archive, 'w') as z:
                    z.writestr(link, target)
                with self.assertRaises(ValueError):
                    mac_store.validate_archive(archive, root)

    def test_archive_accepts_internal_framework_links(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory).resolve()
            archive = root / 'framework.zip'
            link = zipfile.ZipInfo('PPM.app/Contents/Frameworks/QtCore.framework/Versions/Current')
            link.create_system = 3
            link.external_attr = (stat.S_IFLNK | 0o777) << 16
            with zipfile.ZipFile(archive, 'w') as z:
                z.writestr('PPM.app/Contents/Frameworks/QtCore.framework/Versions/A/QtCore', b'library')
                z.writestr(link, 'A')
            mac_store.validate_archive(archive, root)


if __name__ == '__main__':
    unittest.main()
