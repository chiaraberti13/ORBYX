#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Bounded SatDump metadata inspection and stable, read-only acquisition discovery."""
from __future__ import annotations
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import stat
import struct
import sys
import time
import zlib

FILES = ("MSUGS-VIS-1.png", "MSUGS-VIS-2.png", "MSUGS-VIS-3.png", "product.cbor")


def read_regular(path: Path, maximum: int, prefix: int | None = None) -> bytes:
    fd = os.open(path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        info = os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or info.st_size > maximum:
            raise ValueError("Invalid or excessive input file")
        with os.fdopen(os.dup(fd), "rb") as stream:
            data = stream.read((prefix if prefix is not None else maximum) + 1)
        if prefix is None and len(data) > maximum:
            raise ValueError("Input exceeds bounds")
        return data[:prefix] if prefix is not None else data
    finally:
        os.close(fd)


def decode_cbor(data: bytes):
    """SatDump definite-length CBOR. Reject ambiguity, duplicates and excessive nesting."""
    position = 0
    nodes = 0

    def take(count):
        nonlocal position
        if count < 0 or count > len(data) - position:
            raise ValueError("Truncated CBOR")
        value = data[position:position + count]
        position += count
        return value

    def parse(depth=0):
        nonlocal nodes
        nodes += 1
        if depth > 24 or nodes > 10000:
            raise ValueError("CBOR complexity limit")
        head = take(1)[0]
        major, extra = head >> 5, head & 31
        if major == 7:
            if extra in (20, 21, 22):
                return {20: False, 21: True, 22: None}[extra]
            if extra in (25, 26, 27):
                value = struct.unpack({25: ">e", 26: ">f", 27: ">d"}[extra], take({25: 2, 26: 4, 27: 8}[extra]))[0]
                if not math.isfinite(value):
                    raise ValueError("Non-finite CBOR")
                return value
            raise ValueError("Unsupported CBOR simple value")
        if extra < 24:
            count = extra
        elif extra in (24, 25, 26, 27):
            count = int.from_bytes(take({24: 1, 25: 2, 26: 4, 27: 8}[extra]), "big")
        else:
            raise ValueError("Indefinite CBOR is ambiguous")
        if major in (0, 1):
            return count if major == 0 else -1 - count
        if major in (2, 3):
            raw = take(count)
            return raw if major == 2 else raw.decode("utf-8")
        if count > 10000:
            raise ValueError("CBOR collection limit")
        if major == 4:
            return [parse(depth + 1) for _ in range(count)]
        if major == 5:
            result = {}
            for _ in range(count):
                key = parse(depth + 1)
                if not isinstance(key, str) or key in result:
                    raise ValueError("Invalid or duplicate CBOR key")
                result[key] = parse(depth + 1)
            return result
        raise ValueError("Unsupported CBOR tag")
    value = parse()
    if position != len(data):
        raise ValueError("Trailing CBOR data")
    return value


def snapshot(directory: Path) -> dict:
    result = {}
    if directory.is_symlink() or not directory.is_dir():
        raise ValueError("Input must be a non-symlink directory")
    for vis in ("MSUGS_VIS1", "MSUGS_VIS2"):
        sensor = directory / vis
        if sensor.is_symlink() or not sensor.is_dir():
            raise ValueError("Missing or symlink sensor")
        for name in FILES:
            path = sensor / name
            info = path.lstat()
            if not stat.S_ISREG(info.st_mode) or info.st_size == 0:
                raise ValueError("Missing, empty or symlink file")
            result[f"{vis}/{name}"] = (info.st_dev, info.st_ino, info.st_size, info.st_mtime_ns, info.st_ctime_ns, hashlib.sha256(read_regular(path,1024*1024)).hexdigest() if name=="product.cbor" else None)
    return result


def png_header(path: Path) -> bytes:
    fd=os.open(path,os.O_RDONLY|os.O_NOFOLLOW|os.O_NONBLOCK)
    try:
        info=os.fstat(fd)
        if not stat.S_ISREG(info.st_mode) or not 57 <= info.st_size <= 1024*1024*1024:
            raise ValueError("Incomplete or excessive PNG")
        with os.fdopen(os.dup(fd),'rb') as stream:
            header=stream.read(33);stream.seek(-12,os.SEEK_END);tail=stream.read(12)
        if (len(header)!=33 or header[:8]!=b'\x89PNG\r\n\x1a\n' or header[8:16]!=b'\x00\x00\x00\x0dIHDR'
            or zlib.crc32(header[12:29])!=int.from_bytes(header[29:33],'big')
            or tail!=b'\x00\x00\x00\x00IEND\xaeB\x60\x82'
            or header[26:28]!=b'\x00\x00' or header[28] not in (0,1)):
            raise ValueError("Incomplete or invalid PNG structure")
        return header
    finally:os.close(fd)

def inspect_product(directory: Path) -> dict:
    before = snapshot(directory)
    dimensions = []
    identities = set()
    for vis in ("MSUGS_VIS1", "MSUGS_VIS2"):
        metadata = decode_cbor(read_regular(directory / vis / "product.cbor", 1024 * 1024))
        if not isinstance(metadata, dict) or metadata.get("type") != "image" or metadata.get("instrument") != "msugs_vis":
            raise ValueError("Not an MSU-GS VIS image product")
        images = metadata.get("images")
        if not isinstance(images, list) or len(images) != 3:
            raise ValueError("Ambiguous VIS channel metadata")
        mapped = {}
        for entry in images:
            if not isinstance(entry, dict) or entry.get("file") not in FILES[:3] or entry["file"] in mapped:
                raise ValueError("Ambiguous channel filenames")
            channel = FILES.index(entry["file"]) + 1
            if str(entry.get("name")) != str(channel) or type(entry.get("bit_depth")) is not int or not 1 <= entry["bit_depth"] <= 16:
                raise ValueError("Ambiguous channel identity or depth")
            mapped[entry["file"]] = entry
        sensor_dimensions = []
        for name in FILES[:3]:
            header = png_header(directory / vis / name)
            if len(header) != 33 or header[:8] != b"\x89PNG\r\n\x1a\n" or header[12:16] != b"IHDR":
                raise ValueError("Invalid PNG header")
            w, h = struct.unpack(">II", header[16:24])
            depth, color = header[24:26]
            if w < 1 or h <= 3606 or w * h > 150000000 or depth not in (8, 16) or color != 0:
                raise ValueError("Preview or unsupported native PNG geometry")
            if mapped[name]["bit_depth"] > depth:
                raise ValueError("Metadata exceeds PNG storage depth")
            sensor_dimensions.append((w, h, depth))
        if len(set(sensor_dimensions)) != 1:
            raise ValueError("Channel dimensions differ")
        dimensions.append(sensor_dimensions[0])
        for key in ("satellite", "satellite_name", "satellite_id"):
            if key in metadata:
                value = metadata[key]
                if not isinstance(value, str) or not value.strip():
                    raise ValueError("Ambiguous satellite metadata")
                identities.add(value.strip())
    if len(identities) > 1:
        raise ValueError("Conflicting satellite metadata")
    if before != snapshot(directory):
        raise ValueError("Acquisition changed during inspection")
    fingerprint = hashlib.sha256(json.dumps(before, sort_keys=True).encode()).hexdigest()
    return {"path": str(directory.absolute()), "status": "ready", "dimensions": dimensions,
            "satellite": next(iter(identities), None), "fingerprint": fingerprint}


def discover(folder: Path, settle: float = 1.0, sleep=time.sleep) -> list[dict]:
    if not math.isfinite(settle) or not 0 <= settle <= 60:
        raise ValueError("settle must be finite and between 0 and 60 seconds")
    if folder.is_symlink() or not folder.is_dir():
        raise ValueError("Choose a local directory")
    candidates = [p for p in sorted(folder.iterdir()) if p.is_dir() and not p.is_symlink()]
    if len(candidates) > 10000:
        raise ValueError("Too many acquisition directories")
    initial = {}
    incomplete = []
    for path in candidates:
        try:
            initial[path] = snapshot(path)
        except (OSError, ValueError) as error:
            if any((path/vis).exists() or (path/vis).is_symlink() for vis in ("MSUGS_VIS1","MSUGS_VIS2")):
                incomplete.append({'path':str(path.absolute()),'status':'incomplete','reason':str(error)})
    sleep(settle)  # one shared settling interval, not N serial waits
    reports = incomplete
    for path in initial:
        try:
            if initial[path] != snapshot(path):
                reports.append({"path": str(path.absolute()), "status": "unstable"})
                continue
            reports.append(inspect_product(path))
        except (OSError, ValueError, UnicodeError, RecursionError) as error:
            reports.append({"path": str(path.absolute()), "status": "rejected", "reason": str(error)})
    return reports


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("folder", type=Path)
    parser.add_argument("--settle", type=float, default=1.0)
    args = parser.parse_args()
    try:
        print(json.dumps({"schema": "orbyx-import/1", "products": discover(args.folder, args.settle)}))
        return 0
    except (ValueError, OSError) as error:
        print(str(error), file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
