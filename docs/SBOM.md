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

Add `--format cyclonedx` for a CycloneDX 1.5 BOM (ORBYX as BOM metadata, each
resolved package a library component with a `pkg:deb/<distro>/<name>@<version>`
purl) or `--format spdx` for an SPDX 2.3 JSON document (ORBYX as the described
package, each resolved package with a deb purl external reference). CI publishes
the native inventory and both standard BOMs as per-OS artifacts.

Con `--format cyclonedx` l'output è un BOM CycloneDX 1.5; con `--format spdx` un
documento SPDX 2.3. La CI pubblica inventario nativo e i due BOM standard come
artifact per-OS.

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
integration, and CycloneDX 1.5 and SPDX 2.3 export (shape, SPDXID sanitisation
and purls checked by tests). Not yet done: automated advisory correlation
against a live vulnerability feed.

Verificato: generazione deterministica, risoluzione su Ubuntu, parsing,
integrazione CTest riproducibile ed export CycloneDX 1.5 e SPDX 2.3 (struttura,
SPDXID e purl verificati dai test). Da fare: correlazione automatica con un feed
di vulnerabilità.

## Chiusura ELF transitiva / Transitive ELF closure

L'inventario ora comprende anche le librerie transitive e il loader restituiti da ldd, non soltanto le dipendenze DT_NEEDED dirette. Il campo aggiuntivo `direct` distingue le due categorie; CycloneDX/SPDX includono anche i pacchetti transitivi risolti. Risoluzione dpkg compatibile con i due nomi /lib e /usr/lib nei sistemi usr-merge; cache locale al processo per evitare query ripetute.

Verificato su due binari reali con confronto DT_NEEDED/readelf e chiusura ldd, oltre alle fixture dirette/transitive/loader:18test passati. Non inventaria i plugin caricati successivamente tramite dlopen né librerie non caricate nell'avvio ldd. Le librerie senza pacchetto/versione rimangono esplicitamente irrisolte: non equivalgono a componenti sicuri.

The inventory now includes the complete startup ldd closure, including transitive libraries and the loader, with an additive direct flag. Real readelf/ldd integration and synthetic closure fixtures pass; late dlopen plugins remain outside this static startup inventory. Unresolved entries are preserved rather than treated as safe.

## Scansione implementata: Ubuntu OSV

Dopo l’installazione puoi eseguire, dalla cartella ORBYX:

```bash
bash security.sh
```

Lo script prepara la SBOM dei binari installati e consulta l’API OSV per le
versioni **sorgenti Ubuntu** ricavate da dpkg, verificando che le versioni
installate coincidano con l’inventario. Non confronta versioni upstream ignorando
le patch della distribuzione. Nomi/versioni vengono inviati al servizio OSV;
file, immagini e percorsi locali restano sul computer.

Lo snapshot JSON sotto logs conserva richieste, risposte paginate, timestamp e
digest. `orbyx-advisories --replay SNAPSHOT.json` ripete offline la correlazione
senza rete; non rende lo snapshot una firma crittografica del fornitore.
Codici:0 nessuna corrispondenza nel perimetro verificato,1 segnalazioni presenti,
2 copertura incompleta o errore. Nessun esito garantisce assenza di vulnerabilità:
plugin caricati dinamicamente e build custom non inventariate restano fuori.
Altre distribuzioni richiedono un resolver/feed specifico e non sono spacciate
per Ubuntu. La workflow Runtime Ubuntu advisory correlation archivia una prova
sui binari effettivamente compilati. Fixture reale OSV vulnerabile/corretta e
paginazione sono coperte dalle regressioni.

Fonti: [Ubuntu OSV](https://documentation.ubuntu.com/security/security-updates/osv/)
e [API batch OSV](https://google.github.io/osv.dev/post-v1-querybatch/).
