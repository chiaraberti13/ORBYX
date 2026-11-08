"""Regression tests for the deterministic ORBYX runtime SBOM generator.

Unit tests exercise the pure parsing/assembly core with synthetic fixtures and
injected resolvers, so they need neither the compiled binaries nor dpkg. When
binary paths are passed as positional arguments (as CTest does), an additional
integration test runs the real generator against them and validates the output
against what readelf reports for the same file.
"""
import importlib.util
import json
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

SCRIPT = Path(__file__).resolve().parents[1] / "scripts/orbyx_sbom.py"
_spec = importlib.util.spec_from_file_location("orbyx_sbom", SCRIPT)
sbom = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(sbom)

BINARIES: list[str] = []


class ParseNeededTests(unittest.TestCase):
    def test_extracts_sorted_unique_sonames(self):
        output = """
Dynamic section at offset 0x2d58 contains 27 entries:
  Tag        Type                         Name/Value
 0x0000000000000001 (NEEDED)             Shared library: [libopencv_core.so.406]
 0x0000000000000001 (NEEDED)             Shared library: [libjsoncpp.so.25]
 0x0000000000000001 (NEEDED)             Shared library: [libstdc++.so.6]
 0x0000000000000001 (NEEDED)             Shared library: [libjsoncpp.so.25]
 0x000000000000000c (INIT)               0x5000
"""
        self.assertEqual(
            sbom.parse_needed(output),
            ["libjsoncpp.so.25", "libopencv_core.so.406", "libstdc++.so.6"],
        )

    def test_no_needed_entries(self):
        self.assertEqual(sbom.parse_needed("no dynamic section here"), [])


class ParseLddTests(unittest.TestCase):
    def test_resolved_unresolved_vdso_and_loader(self):
        output = """\
\tlinux-vdso.so.1 (0x00007ffd123)
\tlibjsoncpp.so.25 => /lib/x86_64-linux-gnu/libjsoncpp.so.25 (0x00007f1)
\tlibmissing.so.1 => not found
\t/lib64/ld-linux-x86-64.so.2 (0x00007f9)
"""
        mapping = sbom.parse_ldd(output)
        self.assertEqual(mapping["libjsoncpp.so.25"], "/lib/x86_64-linux-gnu/libjsoncpp.so.25")
        self.assertIsNone(mapping["libmissing.so.1"])
        self.assertEqual(mapping["ld-linux-x86-64.so.2"], "/lib64/ld-linux-x86-64.so.2")
        self.assertNotIn("linux-vdso.so.1", mapping)


class BuildInventoryTests(unittest.TestCase):
    def _doc(self):
        needed = {"/bin/orbyx": ["libb.so.2", "liba.so.1", "libgone.so.9"]}
        paths = {"/bin/orbyx": {"liba.so.1": "/lib/liba.so.1",
                                "libb.so.2": "/lib/libb.so.2",
                                "libgone.so.9": None}}
        packages = {"/lib/liba.so.1": "pkg-a", "/lib/libb.so.2": "pkg-b"}
        versions = {"pkg-a": "1.0", "pkg-b": "2.0"}
        return sbom.build_inventory(
            ["/bin/orbyx"],
            needed_of=lambda b: needed[b],
            paths_of=lambda b: paths[b],
            package_of=lambda p: packages.get(p),
            version_of=lambda p: versions.get(p),
            components=[{"name": "orbyx", "version": "0.1.0"}],
            python_runtime=">=3.10",
            distribution="Ubuntu 24.04",
        )

    def test_schema_and_resolution(self):
        doc = self._doc()
        self.assertEqual(doc["schema"], "orbyx-sbom/1")
        self.assertEqual(doc["distribution"], "Ubuntu 24.04")
        libs = doc["binaries"][0]["libraries"]
        # Preserves the soname order returned by needed_of (not re-sorted here).
        self.assertEqual([lib["soname"] for lib in libs], ["libb.so.2", "liba.so.1", "libgone.so.9"])
        resolved = {lib["soname"]: lib for lib in libs}
        self.assertEqual(resolved["liba.so.1"]["package"], "pkg-a")
        self.assertEqual(resolved["liba.so.1"]["version"], "1.0")
        # An unresolved soname carries no package or version, and is not dropped.
        self.assertIsNone(resolved["libgone.so.9"]["path"])
        self.assertIsNone(resolved["libgone.so.9"]["package"])
        self.assertIsNone(resolved["libgone.so.9"]["version"])

    def test_deterministic_output(self):
        self.assertEqual(json.dumps(self._doc(), sort_keys=True),
                         json.dumps(self._doc(), sort_keys=True))

    def test_transitive_libraries_and_loader_are_included(self):
        doc=sbom.build_inventory(['binary'],needed_of=lambda _:['direct.so'],
            paths_of=lambda _:{'direct.so':'/direct','transitive.so':'/transitive','loader.so':'/loader'},
            package_of=lambda p:p,version_of=lambda p:'1',components=[],python_runtime=None,distribution='test')
        libs={v['soname']:v for v in doc['binaries'][0]['libraries']}
        self.assertEqual(set(libs),{'direct.so','transitive.so','loader.so'})
        self.assertTrue(libs['direct.so']['direct'])
        self.assertFalse(libs['transitive.so']['direct'])
        self.assertFalse(libs['loader.so']['direct'])


class CycloneDxTests(unittest.TestCase):
    def _inventory(self):
        return {
            "components": [{"name": "orbyx", "version": "0.1.0", "licence": "GPL-3.0-only"}],
            "binaries": [
                {"binary": "/bin/orbyx", "libraries": [
                    {"soname": "libb.so.2", "path": "/lib/libb.so.2", "package": "pkg-b", "version": "2.0"},
                    {"soname": "liba.so.1", "path": "/lib/liba.so.1", "package": "pkg-a", "version": "1.0"},
                    {"soname": "libgone.so.9", "path": None, "package": None, "version": None},
                ]},
                {"binary": "/bin/orbyx-gui", "libraries": [
                    {"soname": "liba.so.1", "path": "/lib/liba.so.1", "package": "pkg-a", "version": "1.0"},
                ]},
            ],
        }

    def test_bom_shape_and_metadata(self):
        bom = sbom.to_cyclonedx(self._inventory(), namespace="ubuntu")
        self.assertEqual(bom["bomFormat"], "CycloneDX")
        self.assertEqual(bom["specVersion"], "1.5")
        self.assertEqual(bom["metadata"]["component"]["name"], "orbyx")
        self.assertEqual(bom["metadata"]["component"]["licenses"],
                         [{"license": {"id": "GPL-3.0-only"}}])

    def test_components_deduped_sorted_with_purl(self):
        bom = sbom.to_cyclonedx(self._inventory(), namespace="ubuntu")
        # pkg-a appears in two binaries but must be listed once; unresolved
        # libraries carry no component.
        self.assertEqual([(c["name"], c["version"]) for c in bom["components"]],
                         [("pkg-a", "1.0"), ("pkg-b", "2.0")])
        self.assertEqual(bom["components"][0]["purl"], "pkg:deb/ubuntu/pkg-a@1.0")

    def test_deterministic(self):
        inv = self._inventory()
        self.assertEqual(json.dumps(sbom.to_cyclonedx(inv, namespace="ubuntu"), sort_keys=True),
                         json.dumps(sbom.to_cyclonedx(inv, namespace="ubuntu"), sort_keys=True))


class SpdxTests(unittest.TestCase):
    def _inventory(self):
        return {
            "components": [{"name": "orbyx", "version": "0.1.0", "licence": "GPL-3.0-only"}],
            "binaries": [
                {"binary": "/bin/orbyx", "libraries": [
                    {"soname": "libstdc++.so.6", "path": "/lib/libstdc++.so.6",
                     "package": "libstdc++6", "version": "13.1"},
                    {"soname": "liba.so.1", "path": "/lib/liba.so.1",
                     "package": "pkg-a", "version": "1.0"},
                    {"soname": "libgone.so.9", "path": None, "package": None, "version": None},
                ]},
            ],
        }

    def test_document_shape(self):
        doc = sbom.to_spdx(self._inventory(), namespace="ubuntu", created="2026-11-08T00:00:00Z")
        self.assertEqual(doc["spdxVersion"], "SPDX-2.3")
        self.assertEqual(doc["dataLicense"], "CC0-1.0")
        self.assertEqual(doc["SPDXID"], "SPDXRef-DOCUMENT")
        self.assertEqual(doc["creationInfo"]["created"], "2026-11-08T00:00:00Z")
        # Root package carries the declared licence.
        root = doc["packages"][0]
        self.assertEqual(root["SPDXID"], "SPDXRef-Package-orbyx")
        self.assertEqual(root["licenseDeclared"], "GPL-3.0-only")

    def test_spdxids_sanitised_and_unique(self):
        doc = sbom.to_spdx(self._inventory(), namespace="ubuntu", created="2026-11-08T00:00:00Z")
        ids = [pkg["SPDXID"] for pkg in doc["packages"]]
        self.assertEqual(len(ids), len(set(ids)), "SPDXIDs must be unique")
        for spdx_id in ids:
            # SPDX grammar: letters, digits, '.' and '-' only (note: libstdc++6).
            self.assertRegex(spdx_id, r"^SPDXRef-[0-9A-Za-z.\-]+$")
        libs = {pkg["name"]: pkg for pkg in doc["packages"][1:]}
        self.assertEqual(libs["libstdc++6"]["externalRefs"][0]["referenceLocator"],
                         "pkg:deb/ubuntu/libstdc++6@13.1")

    def test_deterministic_for_fixed_created(self):
        inv = self._inventory()
        a = sbom.to_spdx(inv, namespace="ubuntu", created="2026-11-08T00:00:00Z")
        b = sbom.to_spdx(inv, namespace="ubuntu", created="2026-11-08T00:00:00Z")
        self.assertEqual(json.dumps(a, sort_keys=True), json.dumps(b, sort_keys=True))


class CliValidationTests(unittest.TestCase):
    def test_missing_binary_argument_fails(self):
        result = subprocess.run([sys.executable, str(SCRIPT)], capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)

    def test_nonexistent_binary_rejected_localised(self):
        result = subprocess.run([sys.executable, str(SCRIPT), "/no/such/orbyx", "--lang", "it"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn("Non è un file eseguibile", result.stderr)

    def test_symlink_rejected(self):
        with tempfile.TemporaryDirectory() as temp:
            target = Path(temp) / "real"
            target.write_bytes(b"\x7fELF")
            link = Path(temp) / "link"
            link.symlink_to(target)
            result = subprocess.run([sys.executable, str(SCRIPT), str(link)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)


class IntegrationTests(unittest.TestCase):
    def test_real_generation_matches_readelf(self):
        # Evaluated at run time: BINARIES is populated from argv in __main__,
        # after this class is defined, so a class-level decorator would not see it.
        if not BINARIES:
            self.skipTest("no real binaries supplied")
        if not (shutil.which("readelf") and shutil.which("ldd")):
            self.skipTest("binutils/ldd not available")
        result = subprocess.run([sys.executable, str(SCRIPT), *BINARIES],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        doc = json.loads(result.stdout)
        self.assertEqual(doc["schema"], "orbyx-sbom/1")
        self.assertEqual([entry["binary"] for entry in doc["binaries"]], sorted(BINARIES))
        for entry in doc["binaries"]:
            expected = sbom.parse_needed(
                subprocess.run(["readelf", "-d", entry["binary"]], capture_output=True, text=True).stdout)
            self.assertEqual([lib["soname"] for lib in entry["libraries"] if lib['direct']], expected)
            resolved=sbom.parse_ldd(subprocess.run(['ldd',entry['binary']],capture_output=True,text=True).stdout)
            self.assertEqual({lib['soname'] for lib in entry['libraries']},set(expected)|set(resolved))
            self.assertTrue(expected, f"{entry['binary']} reported no NEEDED libraries")
            # Every resolved library must carry an existing absolute path.
            for lib in entry["libraries"]:
                if lib["path"] is not None:
                    self.assertTrue(Path(lib["path"]).exists(), lib["path"])

    def test_real_cyclonedx_export(self):
        if not BINARIES:
            self.skipTest("no real binaries supplied")
        if not (shutil.which("readelf") and shutil.which("ldd")):
            self.skipTest("binutils/ldd not available")
        result = subprocess.run([sys.executable, str(SCRIPT), *BINARIES, "--format", "cyclonedx"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        bom = json.loads(result.stdout)
        self.assertEqual(bom["bomFormat"], "CycloneDX")
        self.assertEqual(bom["specVersion"], "1.5")
        self.assertEqual(bom["metadata"]["component"]["name"], "orbyx")
        names = [component["name"] for component in bom["components"]]
        self.assertEqual(names, sorted(set(names)), "components must be unique and sorted")
        for component in bom["components"]:
            self.assertTrue(component["purl"].startswith("pkg:deb/"))

    def test_real_spdx_export(self):
        if not BINARIES:
            self.skipTest("no real binaries supplied")
        if not (shutil.which("readelf") and shutil.which("ldd")):
            self.skipTest("binutils/ldd not available")
        result = subprocess.run([sys.executable, str(SCRIPT), *BINARIES, "--format", "spdx"],
                                capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        doc = json.loads(result.stdout)
        self.assertEqual(doc["spdxVersion"], "SPDX-2.3")
        self.assertEqual(doc["packages"][0]["SPDXID"], "SPDXRef-Package-orbyx")
        ids = [pkg["SPDXID"] for pkg in doc["packages"]]
        self.assertEqual(len(ids), len(set(ids)), "SPDXIDs must be unique")


if __name__ == "__main__":
    BINARIES = [arg for arg in sys.argv[1:] if not arg.startswith("-")]
    sys.argv = [sys.argv[0]]
    unittest.main()
