#!/usr/bin/env python3
import hashlib
import importlib.util
import io
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location("fetch", Path(__file__).resolve().parents[1] / "scripts/fetch_real_rdas.py")
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class FetchTests(unittest.TestCase):
    def test_download_integrity_and_existing_file(self):
        with tempfile.TemporaryDirectory() as name:
            path = Path(name) / "sample"
            data = b"authentic test transport bytes"
            manifest = {"url": "https://example.invalid", "size": len(data), "sha256": hashlib.sha256(data).hexdigest()}
            module.fetch(path, manifest, lambda *a, **kw: io.BytesIO(data))
            module.fetch(path, manifest, lambda *a, **kw: self.fail("existing verified file must avoid network"))
            self.assertEqual(path.read_bytes(), data)
            path.write_bytes(b"keep")
            with self.assertRaises(ValueError):
                module.fetch(path, manifest)
            self.assertEqual(path.read_bytes(), b"keep")

    def test_bad_payloads_never_publish(self):
        for data in (b"short", b"incorrect", b"too many bytes"):
            with self.subTest(data=data), tempfile.TemporaryDirectory() as name:
                path = Path(name) / "sample"
                manifest = {"url": "https://example.invalid", "size": 9, "sha256": hashlib.sha256(b"real data").hexdigest()}
                with self.assertRaises(ValueError):
                    module.fetch(path, manifest, lambda *a, **kw: io.BytesIO(data))
                self.assertEqual(list(Path(name).iterdir()), [])

    def test_symlink_and_publication_race(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            path = root / "sample"
            path.symlink_to(root / "missing")
            manifest = {"url": "https://example.invalid", "size": 1, "sha256": hashlib.sha256(b"x").hexdigest()}
            with self.assertRaises(ValueError):
                module.fetch(path, manifest)
            path.unlink()
            def competing_producer(*args, **kwargs):
                path.write_bytes(b"keep")
                return io.BytesIO(b"x")
            with self.assertRaises(FileExistsError):
                module.fetch(path, manifest, competing_producer)
            self.assertEqual(path.read_bytes(), b"keep")
            self.assertEqual(sorted(p.name for p in root.iterdir()), ["sample"])


if __name__ == "__main__":
    unittest.main()
