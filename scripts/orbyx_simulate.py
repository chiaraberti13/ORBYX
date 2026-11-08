#!/usr/bin/env python3
"""Synthetic RDAS dataset simulator for ORBYX. Python 3.10+, standard library only.

Generates a synthetic MSU-GS-shaped acquisition (two VIS directories, three
8-bit grayscale PNG channels each with the real CH2=3606 / CH3=1804 vertical
offsets and a placeholder product.cbor), injecting a KNOWN horizontal shift into
the misaligned channels, then drives the real ORBYX engine through a full
`calibrate` and `generate` cycle and checks that the known shift is recovered
and aligned products are produced.

This is a SIMULATION. It exercises the end-to-end pipeline mechanics on
synthetic data. It does NOT verify real-scene radiometric accuracy, calibration
quality on genuine acquisitions, or SatDump geometric interoperability, all of
which require real RDAS datasets and a SatDump environment (roadmap M3-01).
"""
from __future__ import annotations

import argparse
import json
import math
import shutil
import struct
import subprocess
import sys
import tempfile
import zlib
from pathlib import Path

CH2_OFFSET = 3606
CH3_OFFSET = 1804


def write_gray_png(path: Path, width: int, height: int, pixels: bytearray) -> None:
    """Write an 8-bit grayscale PNG (colour type 0) that ORBYX's preflight accepts."""
    def chunk(tag: bytes, data: bytes) -> bytes:
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", width, height, 8, 0, 0, 0, 0)
    raw = bytearray()
    for y in range(height):
        raw.append(0)  # filter: none
        raw.extend(pixels[y * width:(y + 1) * width])
    payload = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(bytes(raw), 6)) + chunk(b"IEND", b"")
    path.write_bytes(payload)


def texture(width: int, active: int, seed: int) -> list[int]:
    """A smooth, feature-rich band (low-frequency pattern) so ECC converges."""
    values = []
    for y in range(active):
        for x in range(width):
            v = 128 + 90 * math.sin((x + seed) / 21.0) * math.cos((y + seed) / 27.0)
            v += 30 * math.sin((x + 2 * y + seed) / 13.0)
            values.append(max(0, min(255, int(v))))
    return values


def channel_image(width: int, height: int, active: int, band: list[int], offset: int, dx: int) -> bytearray:
    """Place the band at a vertical offset, shifted horizontally by dx; zero elsewhere."""
    pixels = bytearray(width * height)
    for y in range(active):
        for x in range(width):
            sx = x - dx
            if 0 <= sx < width:
                pixels[(y + offset) * width + x] = band[y * width + sx]
    return pixels


def build_dataset(root: Path, width: int, height: int, active: int, shift_px: int, feature_margin: int = 0) -> None:
    for vis in (1, 2):
        directory = root / f"MSUGS_VIS{vis}"
        directory.mkdir(parents=True)
        band = texture(width, active, seed=vis * 101)
        # Optional guard band avoids clipping translated features at the image
        # edge, which otherwise biases a reduced-resolution affine estimate.
        if feature_margin:
            for y in range(active):
                for x in range(width):
                    if x < feature_margin or x >= width-feature_margin:
                        band[y*width+x] = 0
        # CH1 is the reference (no offset, no shift); CH2/CH3 carry the known shift.
        for channel, offset, dx in ((1, 0, 0), (2, CH2_OFFSET, shift_px), (3, CH3_OFFSET, shift_px)):
            pixels = channel_image(width, height, active, band, offset, dx)
            write_gray_png(directory / f"MSUGS-VIS-{channel}.png", width, height, pixels)
        (directory / "product.cbor").write_bytes(b"synthetic-simulation-vis" + str(vis).encode())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--orbyx", required=True, type=Path, help="ORBYX CLI executable")
    parser.add_argument("--out", type=Path, default=None, help="working directory (must not exist); default: a temp dir cleaned on success")
    parser.add_argument("--satellite", default="SIM", help="satellite/profile id (A-Z0-9_)")
    parser.add_argument("--width", type=int, default=256)
    parser.add_argument("--active", type=int, default=160, help="height of the content band")
    parser.add_argument("--shift", type=int, default=3, help="known horizontal shift injected into CH2/CH3")
    parser.add_argument("--calibration-scale", type=float, default=1.0, help="explicit ECC working scale; transforms checked in native pixels")
    parser.add_argument("--feature-margin", type=int, default=0, help="blank horizontal guard band for unclipped translated features")
    args = parser.parse_args()

    height = CH2_OFFSET + args.active + 32
    cleanup = args.out is None
    if cleanup:
        work = Path(tempfile.mkdtemp(prefix="orbyx-sim-"))
    else:
        if args.out.exists():
            print(f"orbyx-simulate: output already exists: {args.out}", file=sys.stderr)
            return 2
        work = args.out
        work.mkdir(parents=True)
    data = work / "acquisition"
    build_dataset(data, args.width, height, args.active, args.shift, args.feature_margin)
    empty = work / "empty.json"
    empty.write_text("{}")

    calib_out = work / "calibration"
    generate_out = work / "aligned"
    run = lambda cmd: subprocess.run([str(args.orbyx), *cmd], check=True, timeout=600)
    run(["calibrate", "-s", args.satellite, "-d", str(data), "--config", str(empty), "--output", str(calib_out), "--cpu", "--calibration-scale", str(args.calibration_scale)])
    profile = json.loads((calib_out / "calibration.json").read_text())

    ok = True
    for vis in ("VIS1", "VIS2"):
        key = args.satellite + vis
        for channel, offset in (("CH2", CH2_OFFSET), ("CH3", CH3_OFFSET)):
            ty = profile[key][channel]["ty"]
            tx = profile[key][channel]["tx"]
            # The recovered vertical translation must match the real channel offset,
            # and the recovered horizontal translation the injected shift, within 1 px.
            if abs(ty - offset) > 1.0 or abs(tx - args.shift) > 1.0:
                ok = False
                print(f"  {key} {channel}: tx={tx:.3f} (want ~{args.shift}), ty={ty:.3f} (want ~{offset})")

    run(["generate", "-s", args.satellite, "-d", str(data), "--config", str(calib_out / "calibration.json"), "--output", str(generate_out), "--cpu"])
    products = sorted(p.name for p in generate_out.iterdir())
    produced = any("Aligned" in name for name in products)

    print(f"Simulation dataset: {data}")
    print(f"Calibration recovered CH2 ty≈{CH2_OFFSET}, CH3 ty≈{CH3_OFFSET}, injected shift≈{args.shift}: {'OK' if ok else 'MISMATCH'}")
    print(f"Aligned products: {products}")
    print("NOTE: synthetic simulation only — not a verification of real-scene accuracy or SatDump geometry.")
    success = ok and produced
    if cleanup and success:
        shutil.rmtree(work, ignore_errors=True)
    return 0 if success else 1


if __name__ == "__main__":
    raise SystemExit(main())
