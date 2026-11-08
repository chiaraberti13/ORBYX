#!/usr/bin/env python3
"""Conservative RDAS product recognition for ORBYX. Python 3.10+, standard library.

Decides whether a directory is a *complete and stable* live-decoded RDAS product
ready to be processed, and scans a parent folder listing the ready products by
name. It is deliberately read-only and non-automatic: it NEVER starts a job and
NEVER touches the data, so manual control is preserved (roadmap M2-04). A product
still being written (changing file sizes) is reported as unstable and withheld.

This recognises the on-disk structure and stability only. It does NOT validate
real SatDump metadata or disambiguate genuine ambiguous acquisitions, which need
real datasets (roadmap M3-01).
"""
from __future__ import annotations

import argparse
import sys
import time
from pathlib import Path

VIS_DIRECTORIES = ("MSUGS_VIS1", "MSUGS_VIS2")
REQUIRED_FILES = ("MSUGS-VIS-1.png", "MSUGS-VIS-2.png", "MSUGS-VIS-3.png", "product.cbor")


def required_paths(directory: Path) -> list[Path]:
    return [directory / vis / name for vis in VIS_DIRECTORIES for name in REQUIRED_FILES]


def structure_status(directory: Path) -> str:
    """'not_product', 'incomplete' or 'complete' from the on-disk layout alone."""
    if directory.is_symlink() or not directory.is_dir():
        return "not_product"
    for vis in VIS_DIRECTORIES:
        sub = directory / vis
        if sub.is_symlink() or not sub.is_dir():
            return "not_product"
    for path in required_paths(directory):
        # Symlinks, missing files and empty files all mean "not ready".
        if path.is_symlink() or not path.is_file():
            return "incomplete"
        try:
            if path.stat().st_size == 0:
                return "incomplete"
        except OSError:
            return "incomplete"
    return "complete"


def snapshot(directory: Path) -> dict[str, int]:
    """Map each required file to its size, or -1 when missing/irregular."""
    sizes: dict[str, int] = {}
    for path in required_paths(directory):
        try:
            sizes[str(path)] = path.stat().st_size if (path.is_file() and not path.is_symlink()) else -1
        except OSError:
            sizes[str(path)] = -1
    return sizes


def is_stable(first: dict[str, int], second: dict[str, int]) -> bool:
    """Stable when every required file is present and unchanged between samples."""
    return first == second and all(size > 0 for size in first.values())


def classify(directory: Path, settle: float = 1.0, sleep=time.sleep) -> str:
    """'not_product', 'incomplete', 'unstable' or 'complete' (structure + stability)."""
    status = structure_status(directory)
    if status != "complete":
        return status
    first = snapshot(directory)
    sleep(settle)
    return "complete" if is_stable(first, snapshot(directory)) else "unstable"


def scan(folder: Path, settle: float = 1.0, sleep=time.sleep) -> list[str]:
    """Names of immediate sub-directories that are complete and stable products."""
    ready = []
    for child in sorted(folder.iterdir()):
        if child.is_dir() and not child.is_symlink() and classify(child, settle, sleep) == "complete":
            ready.append(child.name)
    return ready


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("folder", type=Path, help="parent folder to scan for RDAS products")
    parser.add_argument("--settle", type=float, default=1.0, help="seconds between stability samples")
    args = parser.parse_args()
    if args.folder.is_symlink() or not args.folder.is_dir():
        print(f"orbyx-detect: not a directory: {args.folder}", file=sys.stderr)
        return 2
    for name in scan(args.folder, args.settle):
        print(name)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
