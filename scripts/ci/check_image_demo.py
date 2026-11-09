#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Check an actual installed in-app demo without requiring Python OpenCV."""
import json
import struct
import sys
from pathlib import Path

root = Path(sys.argv[1])
report = json.loads((root / "demo-report.json").read_text())
assert report["schema_version"] == 1
for side in ("sx", "dx"):
    for channel, limits in (("2", (425, 436)), ("3", (210, 220))):
        fit = report["alignment"][side][channel]
        assert fit["ecc"] > .94, fit
        assert limits[0] < fit["inverse_affine"][5] < limits[1], fit
assert abs(report["seam"]["right_down_pixels"] - 4) <= 1
assert report["seam"]["edge_correlation"] > .7
assert report["seam"]["horizontal_overlap_pixels"] == 0
for name in ("composite.png", "composite-raw.png"):
    data = (root / name).read_bytes()
    assert data[:8] == b"\x89PNG\r\n\x1a\n" and data[12:16] == b"IHDR"
    assert struct.unpack(">II", data[16:24]) == (1428, 1428)
    assert data[24:26] == bytes((8, 2)), "Expected 8-bit RGB"
    assert len(data) > 100000
assert (root / "composite.png").read_bytes() != (root / "composite-raw.png").read_bytes()
assert len(list(root.glob("*-ch*.png"))) == 6
print("Installed desktop demo: six aligned channels, ECC > .94, measured seam and two 1428x1428 RGB products verified")
