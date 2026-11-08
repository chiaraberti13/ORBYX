"""Regression tests for release signing mechanics (M4-02), using an ephemeral key.

Generates a throwaway GPG key in an isolated GNUPGHOME, signs a checksum
manifest and verifies the detached signature, and confirms that tampering with
the signed file makes verification fail. The ephemeral key never leaves the
temporary directory; real releases use the project's real key.
"""
import os
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/orbyx_sign.py"


@unittest.skipUnless(shutil.which("gpg"), "gpg not available")
class SignTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        root = Path(self.temp.name)
        home = root / "gnupg"
        home.mkdir(mode=0o700)
        self.env = dict(os.environ, GNUPGHOME=str(home))
        generated = subprocess.run(
            ["gpg", "--batch", "--pinentry-mode", "loopback", "--passphrase", "",
             "--quick-generate-key", "ORBYX Test <test@example.invalid>", "default", "default", "never"],
            env=self.env, capture_output=True, text=True, timeout=120)
        self.assertEqual(generated.returncode, 0, generated.stderr)
        self.sums = root / "SHA256SUMS"
        self.sums.write_text("0000000000000000000000000000000000000000000000000000000000000000  artifact\n")
        self.signature = root / "SHA256SUMS.asc"

    def tearDown(self):
        self.temp.cleanup()

    def _sign(self, *args):
        return subprocess.run([sys.executable, str(SCRIPT), *args], env=self.env,
                              capture_output=True, text=True, timeout=60)

    def test_sign_then_verify_succeeds(self):
        signed = self._sign("sign", str(self.sums), "--output", str(self.signature))
        self.assertEqual(signed.returncode, 0, signed.stderr)
        self.assertTrue(self.signature.exists())
        verified = self._sign("verify", str(self.sums), "--signature", str(self.signature))
        self.assertEqual(verified.returncode, 0, verified.stderr)

    def test_tampered_file_fails_verify(self):
        self._sign("sign", str(self.sums), "--output", str(self.signature))
        self.sums.write_text("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff  artifact\n")
        verified = self._sign("verify", str(self.sums), "--signature", str(self.signature))
        self.assertEqual(verified.returncode, 1, verified.stdout + verified.stderr)

    def test_missing_signature_fails(self):
        verified = self._sign("verify", str(self.sums), "--signature", str(self.signature))
        self.assertEqual(verified.returncode, 1)


if __name__ == "__main__":
    unittest.main()
