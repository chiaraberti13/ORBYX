#!/usr/bin/env python3
"""Detached GPG signing/verification for ORBYX release artifacts. Python 3.10+.

Explicit release step (no network, no implicit shell): wraps `gpg` to produce
and verify an armored detached signature over a checksum manifest (SHA256SUMS).
The signing key is supplied by the operator through the gpg keyring / GNUPGHOME;
this tool NEVER generates, embeds or exports a key. Real releases use the
project's real signing key (kept outside the repository); the test suite
exercises the same mechanics with an ephemeral throwaway key.
"""
from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


def _gpg(args: list[str]) -> subprocess.CompletedProcess:
    exe = shutil.which("gpg")
    if not exe:
        raise RuntimeError("gpg is not available")
    return subprocess.run([exe, "--batch", *args], capture_output=True, text=True, timeout=120)


def sign(path: Path, signature: Path, key: str | None) -> None:
    command = ["--yes", "--armor", "--detach-sign", "--output", str(signature)]
    if key:
        command += ["--local-user", key]
    command.append(str(path))
    result = _gpg(command)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "signing failed")


def verify(path: Path, signature: Path) -> bool:
    return _gpg(["--verify", str(signature), str(path)]).returncode == 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    sub = parser.add_subparsers(dest="command", required=True)
    s = sub.add_parser("sign", help="write an armored detached signature")
    s.add_argument("file", type=Path)
    s.add_argument("--output", type=Path, required=True)
    s.add_argument("--key", default=None, help="signing key id/uid (default: gpg's default key)")
    v = sub.add_parser("verify", help="verify a detached signature")
    v.add_argument("file", type=Path)
    v.add_argument("--signature", type=Path, required=True)
    args = parser.parse_args()

    try:
        if args.command == "sign":
            sign(args.file, args.output, args.key)
            print(f"Signed: {args.output}")
            return 0
        if verify(args.file, args.signature):
            print("Good signature")
            return 0
        print("Bad or missing signature", file=sys.stderr)
        return 1
    except (RuntimeError, OSError) as error:
        print(f"orbyx-sign: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
