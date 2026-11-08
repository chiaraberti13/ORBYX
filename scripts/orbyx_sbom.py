#!/usr/bin/env python3
"""Deterministic runtime SBOM / binary inventory for ORBYX. Python 3.10+.

Emits a reproducible JSON inventory of the native shared libraries each built
ORBYX executable actually loads, resolved to the owning distribution package
and its exact version, together with ORBYX's own components and the optional
Python runtime requirement.

This tool does NOT contact the network and does NOT fetch security advisories:
it produces the exact, versioned base that an advisory cross-check consumes.
The correlation against a vulnerability feed (for example the distribution's
own security tracker or an OSV query over the pinned versions) is a separate,
documented step described in docs/SBOM.md and is not performed here, so no
unverified "no known vulnerabilities" claim is ever produced.

The parsing core is pure and side-effect free (parse_needed, parse_ldd,
build_inventory take injected resolvers) so it can be regression-tested without
the real binaries or distribution tooling; see tests/test_sbom.py.
"""
from __future__ import annotations

import argparse
from functools import lru_cache
import json
import re
import shutil
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import Callable, Optional

SCHEMA = "orbyx-sbom/1"

ITALIAN = {
    "Not an executable file: ": "Non è un file eseguibile: ",
    "Required tool not found: ": "Strumento richiesto non disponibile: ",
    "readelf failed for ": "readelf non riuscito per ",
    "ldd failed for ": "ldd non riuscito per ",
    "At least one binary path is required": "È richiesto almeno un percorso di eseguibile",
    "Cannot write output: ": "Impossibile scrivere l'output: ",
}

_NEEDED_RE = re.compile(r"\(NEEDED\)\s+Shared library:\s+\[([^\]]+)\]")
_ADDRESS_RE = re.compile(r"\s+\(0x[0-9a-fA-F]+\)\s*$")


def translate(message: str, language: str) -> str:
    if language != "it":
        return message
    for source, target in ITALIAN.items():
        if message.startswith(source):
            return target + message[len(source):]
    return "Dettaglio tecnico: " + message


def parse_needed(readelf_output: str) -> list[str]:
    """Return the sorted, de-duplicated DT_NEEDED sonames from `readelf -d`."""
    seen: set[str] = set()
    for name in _NEEDED_RE.findall(readelf_output):
        seen.add(name)
    return sorted(seen)


def parse_ldd(ldd_output: str) -> dict[str, Optional[str]]:
    """Map each soname to its resolved absolute path, or None when unresolved.

    Handles the three `ldd` line shapes: a `soname => /path (0x..)` resolution,
    an unresolved `soname => not found`, and a bare loader/VDSO line. The
    virtual `linux-vdso`/`linux-gate` entries are skipped; the dynamic loader is
    reported under its own soname.
    """
    mapping: dict[str, Optional[str]] = {}
    for raw in ldd_output.splitlines():
        line = raw.strip()
        if not line or line.startswith("linux-vdso") or line.startswith("linux-gate"):
            continue
        if "=>" in line:
            left, right = line.split("=>", 1)
            soname = left.strip()
            right = right.strip()
            if not soname:
                continue
            if right.startswith("not found"):
                mapping[soname] = None
            else:
                path = _ADDRESS_RE.sub("", right).strip()
                mapping[soname] = path or None
        else:
            path = _ADDRESS_RE.sub("", line).strip()
            if path.startswith("/"):
                mapping[Path(path).name] = path
    return mapping


def build_inventory(
    binaries: list[str],
    *,
    needed_of: Callable[[str], list[str]],
    paths_of: Callable[[str], dict[str, Optional[str]]],
    package_of: Callable[[str], Optional[str]],
    version_of: Callable[[str], Optional[str]],
    components: list[dict],
    python_runtime: Optional[str],
    distribution: Optional[str],
) -> dict:
    """Assemble the deterministic inventory document from injected resolvers."""
    entries = []
    for binary in sorted(binaries):
        resolved = paths_of(binary)
        libraries = []
        direct = needed_of(binary)
        # ldd exposes the startup ELF closure, not just the binary's DT_NEEDED.
        for soname in [*direct, *sorted(set(resolved)-set(direct))]:
            path = resolved.get(soname)
            package = package_of(path) if path else None
            libraries.append({
                "soname": soname,
                "path": path,
                "package": package,
                "version": version_of(package) if package else None,
                "direct": soname in direct,
            })
        entries.append({"binary": binary, "libraries": libraries})
    return {
        "schema": SCHEMA,
        "distribution": distribution,
        "components": components,
        "python_runtime": python_runtime,
        "binaries": entries,
    }


# --- Real resolvers (Linux / dpkg); degrade gracefully elsewhere. -----------

def _run(args: list[str]) -> str:
    return subprocess.run(args, check=True, capture_output=True, text=True).stdout


def real_needed_of(binary: str, language: str) -> list[str]:
    if not shutil.which("readelf"):
        raise RuntimeError("Required tool not found: readelf")
    try:
        return parse_needed(_run(["readelf", "-d", binary]))
    except subprocess.CalledProcessError as error:
        raise RuntimeError(f"readelf failed for {binary}: {error.stderr.strip()}") from error


def real_paths_of(binary: str, language: str) -> dict[str, Optional[str]]:
    if not shutil.which("ldd"):
        raise RuntimeError("Required tool not found: ldd")
    try:
        return parse_ldd(_run(["ldd", binary]))
    except subprocess.CalledProcessError as error:
        raise RuntimeError(f"ldd failed for {binary}: {error.stderr.strip()}") from error


@lru_cache(maxsize=None)
def real_package_of(path: str) -> Optional[str]:
    if not shutil.which("dpkg-query"):
        return None
    real = str(Path(path).resolve())
    candidates = list(dict.fromkeys([path, real]))
    # usr-merge can leave /lib paths in dpkg's file database while ldd resolves
    # them below /usr/lib, or vice versa. Query both spellings explicitly.
    for value in list(candidates):
        if value.startswith('/usr/lib/') or value.startswith('/usr/lib64/'):
            candidates.append(value[4:])
        elif value.startswith('/lib/') or value.startswith('/lib64/'):
            candidates.append('/usr'+value)
    for candidate in candidates:
        try:
            out = _run(["dpkg-query", "-S", candidate])
            return out.splitlines()[0].split(":", 1)[0] or None
        except (subprocess.CalledProcessError, OSError):
            continue
    return None


@lru_cache(maxsize=None)
def real_version_of(package: str) -> Optional[str]:
    if not package or not shutil.which("dpkg-query"):
        return None
    try:
        return _run(["dpkg-query", "-W", "-f=${Version}", package]).strip() or None
    except subprocess.CalledProcessError:
        return None


def read_os_release() -> Optional[str]:
    return _os_release_field("PRETTY_NAME")


def os_release_id() -> str:
    """Short distribution id for package URLs, e.g. 'ubuntu'; 'deb' as fallback."""
    return _os_release_field("ID") or "deb"


def _os_release_field(key: str) -> Optional[str]:
    try:
        for line in Path("/etc/os-release").read_text(encoding="utf-8").splitlines():
            if line.startswith(key + "="):
                return line.split("=", 1)[1].strip().strip('"') or None
    except OSError:
        return None
    return None


def to_cyclonedx(document: dict, *, namespace: str) -> dict:
    """Convert an orbyx-sbom inventory into a CycloneDX 1.5 BOM (deterministic).

    The first ORBYX component becomes BOM metadata; each distinct resolved
    (package, version) pair becomes a library component with a deb purl.
    Unresolved libraries (no package) are omitted from the component list, as
    CycloneDX components require an identity.
    """
    primary = (document.get("components") or [{}])[0]
    metadata_component = {
        "type": "application",
        "name": primary.get("name", "orbyx"),
        "version": primary.get("version", "0.0.0"),
    }
    licence = primary.get("licence")
    if licence:
        metadata_component["licenses"] = [{"license": {"id": licence}}]

    seen: set[tuple[str, str]] = set()
    components = []
    for entry in document.get("binaries", []):
        for library in entry.get("libraries", []):
            package, version = library.get("package"), library.get("version")
            if not package or not version:
                continue
            key = (package, version)
            if key in seen:
                continue
            seen.add(key)
            components.append({
                "type": "library",
                "name": package,
                "version": version,
                "purl": f"pkg:deb/{namespace}/{package}@{version}",
            })
    components.sort(key=lambda component: (component["name"], component["version"]))
    return {
        "bomFormat": "CycloneDX",
        "specVersion": "1.5",
        "version": 1,
        "metadata": {"component": metadata_component},
        "components": components,
    }


def orbyx_version(repo_root: Path) -> str:
    try:
        text = (repo_root / "CMakeLists.txt").read_text(encoding="utf-8")
        match = re.search(r"project\(ORBYX\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)", text)
        if match:
            return match.group(1)
    except OSError:
        pass
    return "0.1.0"


def _spdx_id(prefix: str, text: str) -> str:
    return prefix + re.sub(r"[^0-9A-Za-z.-]+", "-", text).strip("-")


def to_spdx(document: dict, *, namespace: str, created: str) -> dict:
    """Convert an orbyx-sbom inventory into an SPDX 2.3 JSON document.

    The ORBYX application is the described package; each distinct resolved
    (package, version) pair is a package with a deb purl external reference.
    SPDXIDs are sanitised to the SPDX grammar and de-duplicated. `created` is
    injected so the document is reproducible under test.
    """
    primary = (document.get("components") or [{}])[0]
    name = primary.get("name", "orbyx")
    version = primary.get("version", "0.0.0")
    licence = primary.get("licence", "NOASSERTION")

    packages = [{
        "SPDXID": "SPDXRef-Package-orbyx",
        "name": name,
        "versionInfo": version,
        "downloadLocation": "NOASSERTION",
        "filesAnalyzed": False,
        "licenseConcluded": "NOASSERTION",
        "licenseDeclared": licence,
    }]
    seen: set[tuple[str, str]] = set()
    used_ids: set[str] = {"SPDXRef-Package-orbyx"}
    resolved = []
    for entry in document.get("binaries", []):
        for library in entry.get("libraries", []):
            package, pkg_version = library.get("package"), library.get("version")
            if not package or not pkg_version or (package, pkg_version) in seen:
                continue
            seen.add((package, pkg_version))
            spdx_id = _spdx_id("SPDXRef-Package-", f"{package}-{pkg_version}")
            candidate, suffix = spdx_id, 1
            while candidate in used_ids:
                candidate = f"{spdx_id}-{suffix}"
                suffix += 1
            used_ids.add(candidate)
            resolved.append({
                "SPDXID": candidate,
                "name": package,
                "versionInfo": pkg_version,
                "downloadLocation": "NOASSERTION",
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "externalRefs": [{
                    "referenceCategory": "PACKAGE-MANAGER",
                    "referenceType": "purl",
                    "referenceLocator": f"pkg:deb/{namespace}/{package}@{pkg_version}",
                }],
            })
    packages.extend(sorted(resolved, key=lambda p: (p["name"], p["versionInfo"])))
    return {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": f"{name}-{version}-runtime",
        "documentNamespace": f"https://github.com/chiaraberti13/ORBYX/spdx/{name}-{version}",
        "creationInfo": {"created": created, "creators": ["Tool: orbyx-sbom"]},
        "packages": packages,
        "relationships": [{
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relatedSpdxElement": "SPDXRef-Package-orbyx",
            "relationshipType": "DESCRIBES",
        }],
    }


def main() -> int:
    language = "en"
    for index, value in enumerate(sys.argv[1:], 1):
        if value == "--lang" and index + 1 < len(sys.argv):
            language = sys.argv[index + 1]
        elif value.startswith("--lang="):
            language = value.split("=", 1)[1]
    italian = language == "it"
    parser = argparse.ArgumentParser(
        description=("Inventario runtime SBOM deterministico di ORBYX. Python 3.10+."
                     if italian else __doc__.splitlines()[0]))
    parser.add_argument("binaries", nargs="*", type=str,
                        help="eseguibili ORBYX da inventariare" if italian else "ORBYX executables to inventory")
    parser.add_argument("--output", type=Path,
                        help="file JSON di output (predefinito: stdout)" if italian else "output JSON file (default: stdout)")
    parser.add_argument("--format", choices=("inventory", "cyclonedx", "spdx"), default="inventory",
                        help="formato di output" if italian else "output format")
    parser.add_argument("--lang", choices=("en", "it"), default="en",
                        help="lingua dei messaggi" if italian else "message language")
    args = parser.parse_args()

    try:
        if not args.binaries:
            raise ValueError("At least one binary path is required")
        for binary in args.binaries:
            path = Path(binary)
            if path.is_symlink() or not path.is_file():
                raise ValueError(f"Not an executable file: {binary}")

        repo_root = Path(__file__).resolve().parents[1]
        components = [
            {"name": "orbyx", "version": orbyx_version(repo_root),
             "licence": "GPL-3.0-only",
             "derived_from": "Digitelektro/rdas_layer_aligner"},
        ]
        document = build_inventory(
            [str(Path(b)) for b in args.binaries],
            needed_of=lambda b: real_needed_of(b, args.lang),
            paths_of=lambda b: real_paths_of(b, args.lang),
            package_of=real_package_of,
            version_of=real_version_of,
            components=components,
            python_runtime=">=3.10 (optional batch runner)",
            distribution=read_os_release(),
        )
        if args.format == "cyclonedx":
            document = to_cyclonedx(document, namespace=os_release_id())
        elif args.format == "spdx":
            created = datetime.now(timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
            document = to_spdx(document, namespace=os_release_id(), created=created)
        text = json.dumps(document, ensure_ascii=False, indent=2, sort_keys=False) + "\n"
        if args.output:
            try:
                args.output.write_text(text, encoding="utf-8")
            except OSError as error:
                raise ValueError(f"Cannot write output: {error}") from error
        else:
            sys.stdout.write(text)
        return 0
    except (ValueError, RuntimeError) as error:
        sys.stderr.write(f"orbyx-sbom: {translate(str(error), args.lang)}\n")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
