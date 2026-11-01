"""Prepare CSRs locally and assemble issued certificates without a Mac.

Requires cryptography==50.0.2. Private files stay in an ignored, restricted folder.
This script never creates Apple credentials remotely or sends private keys.
"""
import argparse
import base64
from datetime import datetime, timezone
import os
from pathlib import Path
import secrets
import shutil
import subprocess
import sys

from cryptography import x509
from cryptography.hazmat.primitives import hashes, serialization
from cryptography.hazmat.primitives.asymmetric import rsa
from cryptography.hazmat.primitives.serialization import pkcs12
from cryptography.x509.oid import NameOID

ROOT = Path(__file__).resolve().parents[1]
PRIVATE = ROOT / 'build/apple-signing-private'
PUBLIC = ROOT / 'build/apple-signing-requests'
TEAM = 'V6445K474Q'


def powershell(code, extra_environment):
    tool = shutil.which('pwsh')
    if not tool:
        raise ValueError('PowerShell 7 is required on Windows')
    environment = os.environ.copy()
    environment.update(extra_environment)
    result = subprocess.run([tool, '-NoProfile', '-NonInteractive', '-Command', code], env=environment,
                            capture_output=True, text=True)
    if result.returncode:
        message = result.stderr
        for key, value in extra_environment.items():
            if 'PASSWORD' in key:
                message = message.replace(value, '[REDACTED]')
        raise ValueError('Local credential preparation failed: ' + message[-1800:])


def initialize():
    if (PRIVATE.exists() and (not PRIVATE.is_dir() or any(PRIVATE.iterdir()))) or PUBLIC.exists():
        raise ValueError('Signing folders already exist; preserve existing keys and requests')
    if PRIVATE.resolve() != PRIVATE:
        raise ValueError('Signing folder must not be a symlink or redirected directory')
    if subprocess.run(['git', 'check-ignore', '-q', str(PRIVATE)], cwd=ROOT).returncode:
        raise ValueError('Private signing folder must be ignored by Git')
    PRIVATE.mkdir(parents=True, mode=0o700, exist_ok=True)
    if sys.platform == 'win32':
        powershell(r'''
        $ErrorActionPreference = 'Stop'
        $taskSid = [System.Security.Principal.WindowsIdentity]::GetCurrent().User
        $taskArguments = @($env:PPM_SIGNING_DIRECTORY, '/inheritance:r', '/grant:r',
            ('*' + $taskSid.Value + ':(OI)(CI)F'), '*S-1-5-18:(OI)(CI)F', '*S-1-5-32-544:(OI)(CI)F')
        & icacls.exe @taskArguments | Out-Null
        if ($LASTEXITCODE -ne 0) { throw 'Private directory ACL protection failed' }
        $taskAcl = Get-Acl -LiteralPath $env:PPM_SIGNING_DIRECTORY
        if (-not $taskAcl.AreAccessRulesProtected) { throw 'Private directory still inherits access rules' }
        $taskAllowed = @($taskSid.Value, 'S-1-5-18', 'S-1-5-32-544')
        if ($taskAcl.GetOwner([System.Security.Principal.SecurityIdentifier]).Value -notin $taskAllowed) {
            throw 'Unexpected private directory owner'
        }
        # OWNER RIGHTS represents this verified object owner, not another account.
        $taskAllowed += 'S-1-3-4'
        foreach ($taskRule in $taskAcl.Access) {
            $taskRuleSid = $taskRule.IdentityReference.Translate([System.Security.Principal.SecurityIdentifier]).Value
            if ($taskRule.AccessControlType -eq 'Allow' -and $taskRuleSid -notin $taskAllowed) {
                throw 'Unexpected access to private signing directory'
            }
        }
        ''', {'PPM_SIGNING_DIRECTORY': str(PRIVATE)})
    else:
        PRIVATE.chmod(0o700)
    PUBLIC.mkdir(parents=True)
    password = secrets.token_urlsafe(36).encode('ascii')
    (PRIVATE / 'password.txt').write_bytes(password)
    for purpose in ('application', 'installer'):
        key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
        (PRIVATE / f'{purpose}.key.pem').write_bytes(key.private_bytes(
            serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.BestAvailableEncryption(password)))
        request = x509.CertificateSigningRequestBuilder().subject_name(x509.Name([
            x509.NameAttribute(NameOID.COMMON_NAME, 'Project Planning Manager ' + purpose),
        ])).sign(key, hashes.SHA256())
        path = PUBLIC / f'PPM-{purpose}.certSigningRequest'
        path.write_bytes(request.public_bytes(serialization.Encoding.PEM))
        print('CSR ready:', path)
    for path in PRIVATE.iterdir():
        path.chmod(0o600)
    print('Encrypted private keys and password are retained locally; no private values were printed or uploaded.')


def issued_certificate(path, purpose, password):
    data = path.read_bytes()
    certificate = x509.load_pem_x509_certificate(data) if b'-----BEGIN' in data else x509.load_der_x509_certificate(data)
    key = serialization.load_pem_private_key((PRIVATE / f'{purpose}.key.pem').read_bytes(), password)
    if key.public_key().public_numbers() != certificate.public_key().public_numbers():
        raise ValueError(f'{purpose} certificate does not match its generated CSR private key')
    if certificate.not_valid_after_utc <= datetime.now(timezone.utc):
        raise ValueError(f'{purpose} certificate has expired')
    units = certificate.subject.get_attributes_for_oid(NameOID.ORGANIZATIONAL_UNIT_NAME)
    if not units or units[0].value != TEAM:
        raise ValueError(f'{purpose} certificate belongs to another Apple team')
    common_name = certificate.subject.get_attributes_for_oid(NameOID.COMMON_NAME)[0].value
    prefixes = ('Apple Distribution:', '3rd Party Mac Developer Application:') if purpose == 'application' else ('3rd Party Mac Developer Installer:',)
    if not common_name.startswith(prefixes):
        raise ValueError(f'Wrong {purpose} distribution certificate type')
    return certificate, key


def assemble(application, installer):
    if sys.platform != 'win32':
        raise ValueError('On macOS export both identities using Keychain Access; this assembly path uses Windows ephemeral keys')
    password = (PRIVATE / 'password.txt').read_bytes()
    outputs = [PRIVATE / name for name in ('application.p12', 'installer.p12', 'certificates.p12',
                                          'MAC_APP_STORE_CERTIFICATES_P12_BASE64.txt')]
    if any(path.exists() for path in outputs):
        raise ValueError('Existing signing outputs are preserved; do not overwrite signing material')
    prepared = []
    for purpose, source in [('application', application), ('installer', installer)]:
        certificate, key = issued_certificate(source, purpose, password)
        prepared.append(pkcs12.serialize_key_and_certificates(purpose.encode(), key, certificate, None,
                                                             serialization.BestAvailableEncryption(password)))
    # Validate both certificates before writing any credential output.
    for output, data in zip(outputs[:2], prepared):
        with output.open('xb') as stream:
            stream.write(data)
    destination = outputs[2]
    powershell(r'''
    $ErrorActionPreference = 'Stop'
    $taskFlags = [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::EphemeralKeySet -bor [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable
    $taskCollection = [System.Security.Cryptography.X509Certificates.X509Certificate2Collection]::new()
    try {
        foreach ($taskName in @('application.p12', 'installer.p12')) {
            $taskBytes = [IO.File]::ReadAllBytes([IO.Path]::Combine($env:PPM_SIGNING_DIRECTORY, $taskName))
            $taskCertificate = [System.Security.Cryptography.X509Certificates.X509CertificateLoader]::LoadPkcs12($taskBytes, $env:PPM_EXPORT_PASSWORD, $taskFlags)
            [void]$taskCollection.Add($taskCertificate)
        }
        $taskOutput = $taskCollection.Export([System.Security.Cryptography.X509Certificates.X509ContentType]::Pkcs12, $env:PPM_EXPORT_PASSWORD)
        [IO.File]::WriteAllBytes([IO.Path]::Combine($env:PPM_SIGNING_DIRECTORY, 'certificates.p12'), $taskOutput)
    } finally {
        foreach ($taskCertificate in $taskCollection) { $taskCertificate.Dispose() }
    }
    ''', {'PPM_SIGNING_DIRECTORY': str(PRIVATE), 'PPM_EXPORT_PASSWORD': password.decode('ascii')})
    (PRIVATE / 'MAC_APP_STORE_CERTIFICATES_P12_BASE64.txt').write_bytes(base64.b64encode(destination.read_bytes()))
    print('Combined distribution P12 and base64 file prepared in the restricted local folder.')
    print('No private keys were imported into a persistent Windows certificate store or uploaded.')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest='mode', required=True)
    commands.add_parser('init')
    export = commands.add_parser('assemble')
    export.add_argument('--application-certificate', type=Path, required=True)
    export.add_argument('--installer-certificate', type=Path, required=True)
    args = parser.parse_args()
    if args.mode == 'init':
        initialize()
    else:
        assemble(args.application_certificate, args.installer_certificate)


if __name__ == '__main__':
    try:
        main()
    except ValueError as error:
        raise SystemExit(str(error)) from None
