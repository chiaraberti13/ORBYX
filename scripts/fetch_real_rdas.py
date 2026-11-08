#!/usr/bin/env python3
"""Explicit download of the pinned real Elektro-L4 sample; no credentials needed."""
from __future__ import annotations
import argparse
import hashlib
import json
import os
from pathlib import Path
import tempfile
import urllib.request

MANIFEST = Path(__file__).resolve().parents[1] / "test-data/real/elektro_l4_rdas.json"


def verify(path: Path, size: int, digest: str) -> None:
    if path.is_symlink() or not path.is_file() or path.stat().st_size != size:
        raise ValueError("Expected a regular file of the pinned size")
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    if checksum.hexdigest() != digest:
        raise ValueError("SHA256 mismatch")


def fetch(destination: Path, manifest: dict, opener=urllib.request.urlopen) -> None:
    """Bound actual bytes, verify before exclusive publication, preserve existing files."""
    if destination.exists() or destination.is_symlink():
        verify(destination, manifest["size"], manifest["sha256"])
        return
    fd, name = tempfile.mkstemp(prefix=".orbyx-download-", dir=destination.parent)
    temporary = Path(name)
    try:
        with os.fdopen(fd, "wb") as out:
            request = urllib.request.Request(manifest["url"], headers={"User-Agent": "ORBYX-real-data-test"})
            with opener(request, timeout=60) as response:
                total = 0
                while True:
                    block = response.read(min(1024 * 1024, manifest["size"] - total + 1))
                    if not block:
                        break
                    total += len(block)
                    if total > manifest["size"]:
                        raise ValueError("Download exceeds pinned size")
                    out.write(block)
            out.flush()
            os.fsync(out.fileno())
        verify(temporary, manifest["size"], manifest["sha256"])
        os.link(temporary, destination)  # fails if another producer has published
    finally:
        temporary.unlink(missing_ok=True)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_text())
    fetch(args.destination, manifest)
    print(json.dumps({"path": str(args.destination), **manifest}, indent=2))


if __name__ == "__main__":
    main()
