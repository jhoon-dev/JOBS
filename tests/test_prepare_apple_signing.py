"""Offline checks with synthetic certificates; never access real signing material."""
from datetime import datetime, timedelta, timezone
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

try:
    from cryptography import x509
    from cryptography.hazmat.primitives import hashes, serialization
    from cryptography.hazmat.primitives.asymmetric import rsa
    from cryptography.x509.oid import NameOID
except ImportError:
    x509 = None


@unittest.skipIf(x509 is None, 'Requires cryptography==50.0.2 in the signing-tools environment')
class AppleSigningPreparationTests(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location('apple_signing', Path(__file__).resolve().parents[1] / 'scripts/prepare-apple-signing.py')
        self.helper = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.helper)
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.helper.PRIVATE = Path(self.temp.name)
        self.password = b'synthetic-test-password'
        (self.helper.PRIVATE / 'password.txt').write_bytes(self.password)
        self.certificates = []
        for purpose, prefix in [('application', 'Apple Distribution:'), ('installer', '3rd Party Mac Developer Installer:')]:
            key = rsa.generate_private_key(public_exponent=65537, key_size=2048)
            (self.helper.PRIVATE / f'{purpose}.key.pem').write_bytes(key.private_bytes(serialization.Encoding.PEM, serialization.PrivateFormat.PKCS8, serialization.BestAvailableEncryption(self.password)))
            subject = x509.Name([x509.NameAttribute(NameOID.COMMON_NAME, prefix + ' Synthetic Test'), x509.NameAttribute(NameOID.ORGANIZATIONAL_UNIT_NAME, self.helper.TEAM)])
            now = datetime.now(timezone.utc)
            certificate = x509.CertificateBuilder().subject_name(subject).issuer_name(subject).public_key(key.public_key()).serial_number(x509.random_serial_number()).not_valid_before(now - timedelta(days=1)).not_valid_after(now + timedelta(days=1)).sign(key, hashes.SHA256())
            path = self.helper.PRIVATE / f'{purpose}.cer'
            path.write_bytes(certificate.public_bytes(serialization.Encoding.DER))
            self.certificates.append(path)

    def test_reject_wrong_certificate_without_partial_outputs(self):
        with patch.object(sys, 'platform', 'win32'):
            with self.assertRaisesRegex(ValueError, 'does not match'):
                self.helper.assemble(self.certificates[0], self.certificates[0])
        self.assertEqual(list(self.helper.PRIVATE.glob('*.p12')), [])

    def test_preserve_existing_outputs(self):
        output = self.helper.PRIVATE / 'certificates.p12'
        output.write_bytes(b'preserve')
        with patch.object(sys, 'platform', 'win32'):
            with self.assertRaisesRegex(ValueError, 'preserved'):
                self.helper.assemble(*self.certificates)
        self.assertEqual(output.read_bytes(), b'preserve')

    @unittest.skipUnless(sys.platform == 'win32', 'Windows ephemeral export path')
    def test_combined_export_contains_both_private_keys(self):
        self.helper.assemble(*self.certificates)
        encoded = self.helper.PRIVATE / 'MAC_APP_STORE_CERTIFICATES_P12_BASE64.txt'
        self.assertTrue(encoded.stat().st_size > 1000)
        self.helper.powershell(r'''
        $ErrorActionPreference = 'Stop'
        $taskFlags = [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::EphemeralKeySet -bor [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::Exportable
        $taskCollection = [System.Security.Cryptography.X509Certificates.X509Certificate2Collection]::new()
        try {
            $taskCollection.Import([IO.File]::ReadAllBytes($env:PPM_TEST_P12), $env:PPM_EXPORT_PASSWORD, $taskFlags)
            if ($taskCollection.Count -ne 2 -or @($taskCollection | Where-Object HasPrivateKey).Count -ne 2) { throw 'Both private keys must survive combined export' }
        } finally { foreach ($taskCertificate in $taskCollection) { $taskCertificate.Dispose() } }
        ''', {'PPM_TEST_P12': str(self.helper.PRIVATE / 'certificates.p12'), 'PPM_EXPORT_PASSWORD': self.password.decode()})


if __name__ == '__main__':
    unittest.main()
