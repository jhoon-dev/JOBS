"""Sign a verified preparation artifact; upload only after external-folder QA."""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import plistlib
import re
import secrets
import stat
import subprocess
import sys
import tempfile
import uuid
import zipfile

ROOT = Path(__file__).resolve().parents[1]
TEAM = 'V6445K474Q'
BUNDLE = 'com.toolmoa.jobs'
REQUIRED = ('MAC_APP_STORE_CERTIFICATES_P12_BASE64', 'MAC_APP_STORE_CERTIFICATES_PASSWORD',
            'MAC_APP_STORE_PROVISION_PROFILE_BASE64', 'APP_STORE_CONNECT_API_KEY_ID',
            'APP_STORE_CONNECT_API_ISSUER_ID', 'APP_STORE_CONNECT_API_PRIVATE_KEY')
LOCAL_SECRETS = []


def validate_preparation(status, revision, upload, external_verified):
    if status.get('source_revision') != revision or not status.get('qt_appstore_compliant'):
        raise ValueError('Preparation source or Qt distribution differs from this release')
    if not status.get('sandbox_container_verified'):
        raise ValueError('Signed sandbox container lifecycle has not passed')
    if upload and not external_verified:
        raise ValueError('Verify external folder grant/cancel/bookmark restart before upload')


def validate_profile(profile, now=None):
    """Reject another app/team, wildcard, expired and development profiles."""
    now = now or datetime.now(timezone.utc)
    expiration = profile.get('ExpirationDate')
    if not isinstance(expiration, datetime):
        raise ValueError('Provisioning profile has no expiration date')
    if expiration.replace(tzinfo=expiration.tzinfo or timezone.utc) <= now:
        raise ValueError('Provisioning profile has expired')
    if profile.get('TeamIdentifier') != [TEAM] or profile.get('ProvisionedDevices') or profile.get('ProvisionsAllDevices'):
        raise ValueError('Expected this team and a Mac App Store distribution profile')
    entitlements = profile.get('Entitlements', {})
    identifiers = [entitlements[k] for k in ('com.apple.application-identifier', 'application-identifier')
                   if k in entitlements]
    if not identifiers or any(value != TEAM + '.' + BUNDLE for value in identifiers):
        raise ValueError('Provisioning profile does not match the exact PPM bundle ID')
    if entitlements.get('com.apple.developer.team-identifier') != TEAM:
        raise ValueError('Provisioning entitlement team differs')
    if entitlements.get('get-task-allow') or entitlements.get('com.apple.security.get-task-allow'):
        raise ValueError('Development/debugging profile cannot be submitted')
    certificates = profile.get('DeveloperCertificates')
    if not isinstance(certificates, list) or not certificates or not all(isinstance(c, bytes) for c in certificates):
        raise ValueError('Provisioning profile contains no distribution certificates')
    return {hashlib.sha1(c).hexdigest().upper() for c in certificates}


def validate_archive(path, destination):
    with zipfile.ZipFile(path) as archive:
        for item in archive.infolist():
            target = (destination / item.filename).resolve()
            if not target.is_relative_to(destination) or Path(item.filename).is_absolute():
                raise ValueError('App archive escapes extraction directory')
            if stat.S_ISLNK(item.external_attr >> 16):
                link = archive.read(item).decode('utf-8')
                if (Path(link).is_absolute() or '..' in Path(link).parts or
                        not (target.parent / link).resolve().is_relative_to(destination)):
                    raise ValueError('App archive symlink escapes extraction directory')


def run(command, *, cwd=None, environment=None):
    result = subprocess.run(list(map(str, command)), cwd=cwd, env=environment, capture_output=True, text=True)
    output = result.stdout + result.stderr
    for name in REQUIRED:
        value = os.environ.get(name)
        if value:
            output = output.replace(value, '[REDACTED]')
    for value in LOCAL_SECRETS:
        output = output.replace(value, '[REDACTED]')
    if result.returncode:
        raise RuntimeError(f'{Path(str(command[0])).name} failed (exit {result.returncode}): {output[-2500:]}')
    return output


def secret_file(name, path, *, encoded=True):
    value = os.environ[name]
    data = base64.b64decode(''.join(value.split()), validate=True) if encoded else value.encode('utf-8')
    with path.open('xb') as stream:
        os.chmod(path, 0o600)
        stream.write(data)


def identity(lines, prefixes, fingerprints=None):
    choices = []
    for fingerprint, name in re.findall(r'\b([0-9A-F]{40})\s+"([^"]+)"', lines):
        if any(name.startswith(prefix) for prefix in prefixes) and f'({TEAM})' in name:
            if fingerprints is None or fingerprint in fingerprints:
                choices.append((fingerprint, name))
    if len(choices) != 1:
        raise ValueError('Expected exactly one matching Apple distribution identity in the temporary keychain')
    return choices[0][1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--preparation', type=Path, required=True)
    parser.add_argument('--upload', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('Apple signing and upload tools require macOS')
    missing = [name for name in REQUIRED if not os.environ.get(name)]
    if missing:
        raise ValueError('Configure repository Secrets: ' + ', '.join(missing))
    key_id = os.environ['APP_STORE_CONNECT_API_KEY_ID']
    if not re.fullmatch(r'[A-Z0-9]{10}', key_id):
        raise ValueError('Invalid App Store Connect key ID')
    uuid.UUID(os.environ['APP_STORE_CONNECT_API_ISSUER_ID'])
    preparation = args.preparation.resolve()
    records = list(preparation.rglob('status.json'))
    archives = list(preparation.rglob('PPM-unsigned-universal.app.zip'))
    if len(records) != 1 or len(archives) != 1:
        raise ValueError('Expected one verified preparation status and app archive')
    status = json.loads(records[0].read_text())
    revision = run(['git', 'rev-parse', 'HEAD']).strip()
    external_verified = os.environ.get('MAC_SANDBOX_EXTERNAL_VERIFIED') == 'true'
    validate_preparation(status, revision, args.upload, external_verified)
    output = ROOT / 'dist/mac-app-store'
    output.mkdir(parents=True, exist_ok=True)
    temporary_root = Path(os.environ['RUNNER_TEMP']).resolve()
    with tempfile.TemporaryDirectory(prefix='ppm-sign-', dir=temporary_root) as directory:
        work = Path(directory).resolve()
        if not work.is_relative_to(temporary_root):
            raise ValueError('Signing workspace escapes runner temporary directory')
        os.chmod(work, 0o700)
        validate_archive(archives[0], work)
        run(['ditto', '-xk', archives[0], work])
        app = work / 'PPM.app'
        info = plistlib.loads((app / 'Contents/Info.plist').read_bytes())
        if info.get('CFBundleIdentifier') != BUNDLE or info.get('PPMFileAccessPolicy') != 'app-scoped-folders-v1':
            raise ValueError('App identity or sandbox file-access implementation differs')
        if info.get('ITSAppUsesNonExemptEncryption') is not False:
            raise ValueError('Encryption declaration is missing')
        if sorted(p.name for p in (app / 'Contents/MacOS').iterdir()) != ['PPM']:
            raise ValueError('Store app contains an unexpected executable or test probe')
        run(['lipo', app / 'Contents/MacOS/PPM', '-verify_arch', 'arm64', 'x86_64'])
        profile_path = app / 'Contents/embedded.provisionprofile'
        secret_file('MAC_APP_STORE_PROVISION_PROFILE_BASE64', profile_path)
        profile = plistlib.loads(run(['security', 'cms', '-D', '-i', profile_path]).encode())
        fingerprints = validate_profile(profile)
        p12, keychain = work / 'identities.p12', work / 'signing.keychain-db'
        secret_file('MAC_APP_STORE_CERTIFICATES_P12_BASE64', p12)
        keychain_password = secrets.token_urlsafe(36)
        LOCAL_SECRETS.append(keychain_password)
        try:
            run(['security', 'create-keychain', '-p', keychain_password, keychain])
            run(['security', 'set-keychain-settings', '-lut', '21600', keychain])
            run(['security', 'unlock-keychain', '-p', keychain_password, keychain])
            run(['security', 'import', p12, '-k', keychain, '-P',
                 os.environ['MAC_APP_STORE_CERTIFICATES_PASSWORD'], '-T', '/usr/bin/codesign', '-T', '/usr/bin/productbuild'])
            run(['security', 'set-key-partition-list', '-S', 'apple-tool:,apple:', '-s', '-k', keychain_password, keychain])
            app_identity = identity(run(['security', 'find-identity', '-v', '-p', 'codesigning', keychain]),
                                    ('Apple Distribution:', '3rd Party Mac Developer Application:'), fingerprints)
            installer_identity = identity(run(['security', 'find-identity', '-v', keychain]),
                                          ('3rd Party Mac Developer Installer:',))
            entitlements = plistlib.loads((ROOT / 'packaging/macos/PPM.entitlements').read_bytes())
            entitlements.update({'com.apple.application-identifier': TEAM + '.' + BUNDLE,
                                 'com.apple.developer.team-identifier': TEAM})
            entitlements_path = work / 'entitlements.plist'
            entitlements_path.write_bytes(plistlib.dumps(entitlements))
            sign = ['codesign', '--force', '--sign', app_identity, '--keychain', keychain, '--timestamp']
            magic = {bytes.fromhex(h) for h in ('feedface', 'cefaedfe', 'feedfacf', 'cffaedfe', 'cafebabe', 'bebafeca', 'cafebabf', 'bfbafeca')}
            for path in sorted(app.rglob('*'), key=lambda p: len(p.parts), reverse=True):
                if path.is_file() and not path.is_symlink():
                    with path.open('rb') as stream:
                        macho = stream.read(4) in magic
                    if macho:
                        run([*sign, path])
                elif path.is_dir() and not path.is_symlink() and path.suffix == '.framework':
                    run([*sign, path])
            run([*sign, '--entitlements', entitlements_path, app])
            run(['codesign', '--verify', '--deep', '--strict', app])
            signed_output = run(['codesign', '-d', '--entitlements', ':-', app])
            start = signed_output.index('<?xml')
            end = signed_output.index('</plist>', start) + len('</plist>')
            effective = plistlib.loads(signed_output[start:end].encode())
            if any(effective.get(key) != value for key, value in entitlements.items()):
                raise ValueError('Effective signed app entitlements differ')
            package = output / f'PPM-{info["CFBundleShortVersionString"]}-{info["CFBundleVersion"]}.pkg'
            run(['productbuild', '--component', app, '/Applications', '--sign', installer_identity,
                 '--keychain', keychain, package])
            run(['pkgutil', '--check-signature', package])
            (work / 'private_keys').mkdir(mode=0o700)
            secret_file('APP_STORE_CONNECT_API_PRIVATE_KEY', work / 'private_keys' / f'AuthKey_{key_id}.p8', encoded=False)
            api_environment = os.environ.copy()
            api_environment['API_PRIVATE_KEYS_DIR'] = str(work / 'private_keys')
            authentication = ['--apiKey', key_id, '--apiIssuer', os.environ['APP_STORE_CONNECT_API_ISSUER_ID']]
            validation = run(['xcrun', 'altool', '--validate-app', '-f', package, *authentication],
                             cwd=work, environment=api_environment)
            (output / 'validation.log').write_text(validation)
            run(['ditto', '-c', '-k', '--sequesterRsrc', '--keepParent', app, output / 'PPM-signed.app.zip'])
            if args.upload:
                (output / 'upload.log').write_text(run([
                    'xcrun', 'altool', '--upload-package', package, '--apple-id', '6829146371', '--bundle-id', BUNDLE,
                    '--bundle-short-version-string', info['CFBundleShortVersionString'],
                    '--bundle-version', info['CFBundleVersion'], *authentication], cwd=work, environment=api_environment))
            result = {'source_revision': revision, 'bundle_id': BUNDLE, 'version': info['CFBundleShortVersionString'],
                      'build': info['CFBundleVersion'], 'signed': True, 'sandbox_container_verified': True,
                      'external_folder_selection_verified': external_verified,
                      'external_verification_source': 'manual QA attestation' if external_verified else None,
                      'uploaded': args.upload, 'review_submitted': False,
                      'package_sha256': hashlib.sha256(package.read_bytes()).hexdigest()}
            (output / 'status.json').write_text(json.dumps(result, indent=2) + '\n')
            print(json.dumps(result, indent=2))
        finally:
            if keychain.exists():
                run(['security', 'delete-keychain', keychain])
            LOCAL_SECRETS.clear()


if __name__ == '__main__':
    try:
        main()
    except (ValueError, RuntimeError) as error:
        raise SystemExit(str(error)) from None
