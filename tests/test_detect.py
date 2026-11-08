"""Regression tests for conservative RDAS product recognition (M2-04).

Builds synthetic complete/incomplete/unstable products with the simulation
generator and checks the recogniser never reports a not-ready product as ready
and never touches the data.
"""
import importlib.util
import os
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def _load(name: str):
    spec = importlib.util.spec_from_file_location(name, ROOT / "scripts" / f"{name}.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


detect = _load("orbyx_detect")
sim = _load("orbyx_simulate")

WIDTH, ACTIVE = 16, 4
HEIGHT = sim.CH2_OFFSET + ACTIVE + 8
NOSLEEP = lambda _seconds: None


class DetectTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.root = Path(self.temp.name)
        self.product = self.root / "acquisition"
        sim.build_dataset(self.product, WIDTH, HEIGHT, ACTIVE, shift_px=1)

    def tearDown(self):
        self.temp.cleanup()

    def test_complete_product_is_recognised(self):
        self.assertEqual(detect.structure_status(self.product), "complete")
        self.assertEqual(detect.classify(self.product, sleep=NOSLEEP), "complete")

    def test_missing_channel_is_incomplete(self):
        (self.product / "MSUGS_VIS1" / "MSUGS-VIS-2.png").unlink()
        self.assertEqual(detect.structure_status(self.product), "incomplete")
        self.assertEqual(detect.classify(self.product, sleep=NOSLEEP), "incomplete")

    def test_missing_vis_directory_is_not_a_product(self):
        for item in (self.product / "MSUGS_VIS2").iterdir():
            item.unlink()
        (self.product / "MSUGS_VIS2").rmdir()
        self.assertEqual(detect.structure_status(self.product), "not_product")

    def test_empty_file_is_incomplete(self):
        (self.product / "MSUGS_VIS2" / "product.cbor").write_bytes(b"")
        self.assertEqual(detect.structure_status(self.product), "incomplete")

    def test_symlinked_file_is_rejected(self):
        target = self.product / "MSUGS_VIS1" / "MSUGS-VIS-1.png"
        link = self.product / "MSUGS_VIS2" / "MSUGS-VIS-1.png"
        link.unlink()
        link.symlink_to(target)
        self.assertEqual(detect.structure_status(self.product), "incomplete")

    def test_growing_product_is_unstable(self):
        # A size change between samples means the product is still being written.
        path = self.product / "MSUGS_VIS1" / "product.cbor"
        first = detect.snapshot(self.product)
        path.write_bytes(path.read_bytes() + b"more")
        self.assertFalse(detect.is_stable(first, detect.snapshot(self.product)))

    def test_scan_lists_only_ready_products_without_touching_data(self):
        incomplete = self.root / "incomplete"
        sim.build_dataset(incomplete, WIDTH, HEIGHT, ACTIVE, shift_px=1)
        (incomplete / "MSUGS_VIS1" / "product.cbor").unlink()
        before = {p: p.stat().st_mtime_ns for p in detect.required_paths(self.product)}
        ready = detect.scan(self.root, sleep=NOSLEEP)
        self.assertEqual(ready, ["acquisition"])
        self.assertNotIn("incomplete", ready)
        # Recognition is read-only: it must not modify the data it inspects.
        after = {p: p.stat().st_mtime_ns for p in detect.required_paths(self.product)}
        self.assertEqual(before, after)


if __name__ == "__main__":
    unittest.main()
