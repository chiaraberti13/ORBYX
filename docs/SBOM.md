# Runtime inventory & SBOM / Inventario runtime e SBOM

Date / Data: 2026-11-08.

## Purpose / Scopo

`scripts/orbyx_sbom.py` produces a deterministic JSON inventory of the native
shared libraries each built ORBYX executable actually loads, resolved to the
owning distribution package and its exact version, together with ORBYX's own
components and the optional Python runtime requirement. It is the reproducible,
versioned base that a vulnerability cross-check consumes.

Lo strumento emette un inventario JSON deterministico delle librerie native
effettivamente caricate dagli eseguibili ORBYX, risolte al pacchetto della
distribuzione e alla versione esatta. È la base riproducibile su cui si innesta
la correlazione con gli avvisi di sicurezza.

## What it does NOT do / Cosa NON fa

The tool performs **no network access** and fetches **no advisories**: it never
emits a "no known vulnerabilities" claim. The correlation against a
vulnerability feed is a separate, explicit step (below). This keeps the
inventory honest and offline-reproducible.

Lo strumento **non effettua accessi di rete** e **non scarica avvisi**: non
dichiara mai l'assenza di vulnerabilità. La correlazione con un feed è un passo
separato ed esplicito (sotto).

## Generate the inventory / Generare l'inventario

```bash
cmake --build build
scripts/orbyx_sbom.py build/orbyx build/orbyx-gui --output build/orbyx-sbom.json
```

Each binary entry lists every `DT_NEEDED` soname (from `readelf -d`), its
resolved absolute path (`ldd`), and the owning package/version (`dpkg-query`).
On a non-dpkg system package/version fields are `null`; the soname/path mapping
is still produced. The output is stable across runs for the same inputs.

Add `--format cyclonedx` to emit a CycloneDX 1.5 BOM instead: the ORBYX
application as BOM metadata and each distinct resolved package as a library
component with a `pkg:deb/<distro>/<name>@<version>` purl. CI publishes both the
native inventory and the CycloneDX BOM as per-OS artifacts.

Con `--format cyclonedx` l'output è un BOM CycloneDX 1.5: l'applicazione ORBYX
nei metadati e ogni pacchetto risolto come componente con purl deb. La CI
pubblica inventario nativo e BOM CycloneDX come artifact per-OS.

Ogni voce elenca i `DT_NEEDED` (`readelf -d`), il percorso risolto (`ldd`) e il
pacchetto/versione proprietario (`dpkg-query`). Su sistemi senza dpkg i campi
pacchetto/versione sono `null`; la mappatura soname/percorso resta prodotta.

## Vulnerability cross-check / Correlazione vulnerabilità

The pinned package versions in the inventory are the input to an advisory
scan. This step requires the distribution's security feed and is **not** run
offline here; record its output alongside the inventory when performed:

Le versioni fissate nell'inventario sono l'input di una scansione avvisi. Il
passo richiede il feed di sicurezza della distribuzione e **non** è eseguito
offline qui; registrarne l'esito insieme all'inventario:

- Debian/Ubuntu security tracker or `debsecan` over the exact package versions.
- [OSV](https://osv.dev/) queries over the same `package`/`version` pairs.
- GitHub Dependabot already covers the pinned GitHub Actions and is
  complementary, not a substitute, for the native runtime inventory.

## Regression / Regressione

`tests/test_sbom.py` unit-tests the pure parsing/assembly core with synthetic
fixtures and injected resolvers, and — when binary paths are supplied, as the
`sbom-regression` CTest group does — runs the real generator and checks the
output against `readelf -d` for the same binaries.

`tests/test_sbom.py` verifica il core di parsing con fixture sintetiche e, con i
binari reali (gruppo CTest `sbom-regression`), confronta l'output con
`readelf -d`.

## Scope / Limiti

Verified: deterministic generation, soname→path→package→version resolution on
Ubuntu, parsing of resolved/unresolved/loader `ldd` lines, reproducible CTest
integration, and CycloneDX 1.5 export (shape and purls checked by tests). Not
yet done: automated advisory correlation against a live feed, and an SPDX
export.

Verificato: generazione deterministica, risoluzione su Ubuntu, parsing,
integrazione CTest riproducibile ed export CycloneDX 1.5 (struttura e purl
verificati dai test). Da fare: correlazione automatica con un feed ed export
SPDX.
