# Ambienti reali di piattaforma / Real platform environments

## Italiano

Il workflow [`platforms.yml`](../.github/workflows/platforms.yml) predispone risorse
reali di CI, senza richiedere hardware o macchine all'utente e senza ricorrere a
runner GPU o servizi a pagamento. La presenza del workflow **non** costituisce un
risultato superato: consultare i job dell'esatta revisione eseguita.

| Percorso | Ambiente | Prova eseguita quando il job passa | Confine della dichiarazione |
|---|---|---|---|
| Fedora nativa | Container ufficiale Fedora 44 x86_64 su host x86_64 | Compilazione Qt/OpenCV/JsonCpp con pacchetti Fedora, CTest completo, generazione RPM | Userspace Fedora reale; non una VM Fedora con kernel e desktop propri |
| RPM pulito | Seconda istanza dell'immagine Fedora originale | DNF installa il solo RPM e risolve le dipendenze runtime; nessun pacchetto `-devel`/compilatore ammesso; CLI e GUI installate elaborano fixture e producono otto file identici; rimozione del pacchetto | Verifica packaging/plugin offscreen; non una sessione grafica interattiva |
| ARM64 nativa | Runner GitHub `ubuntu-24.04-arm`, con controllo `uname -m = aarch64` | Compilazione, CTest e DEB `Architecture: arm64` su processore ARM64 | Non QEMU, non cross-compilazione; nessuna dichiarazione GPU |
| DEB ARM64 pulito | Container Ubuntu 24.04 nativo sul medesimo host ARM64 | APT installa il solo DEB e risolve le dipendenze runtime; nessuna dipendenza di sviluppo; confronto pipeline CLI/GUI installate e rimozione | Userspace pulito, kernel condiviso con il runner; non una VM desktop completa |

Le fixture sono sintetiche e dichiarate tali: servono a verificare il programma
nell'ambiente reale, non trasformano questa prova in validazione satellitare
radiometrica/geometrica. Il binario `orbyx-fixture-tool` pubblicato negli artifact
serve esclusivamente a generarle; non viene installato nel sistema come parte di
ORBYX. Gli input della prova restano presenti dopo la rimozione del pacchetto.

Il sorgente è montato in sola lettura nel container di build; il container di
runtime vede soltanto lo script di verifica e gli artifact, non l'ambiente di
compilazione. Sono registrati digest immagine, pacchetti/versioni, architettura e
log. Fedora e Ubuntu sono fissate a digest OCI verificati e registrati nel job. Le dipendenze risolte dai repository di distribuzione possono
cambiare: gli inventari del run consentono di conoscere le versioni effettive.

Gli artifact hanno durata di 14 giorni. Il workflow usa solo le etichette standard
GitHub per repository pubblici; non configura runner a pagamento, credenziali
cloud o una macchina self-hosted dell'utente.

### Ancora fuori dal perimetro

- AppImage: il bundling di Qt/plugin/codec deve essere verificato con strumenti
  versionati e checksum e poi eseguito su una base pulita senza Qt/OpenCV. Un
  semplice AppDir o archivio rinominato non è una AppImage verificata.
- VM grafica completa, integrazione con menu del desktop e screen reader.
- Hardware GPU e equivalenza OpenCL; CPU e ARM64 non vengono chiamate GPU.
- Dati satellitari reali e geometria SatDump, che hanno un test distinto.

### Fonti verificate

- [Fedora 44: distribuzione ufficiale](https://www.fedoraproject.org/workstation/download/)
  (rilascio 28 aprile 2026).
- [Registro container Fedora](https://registry.fedoraproject.org/): indice OCI
  `fedora:44` risolto l'8 novembre 2026 a
  `sha256:5aae8bdc5f40bf70f8f189a70770e6f1fabcb04976e841262431c3bbfdbe3538`.
- Base Ubuntu ufficiale `ubuntu:24.04`, risolta l’8 novembre 2026 a
  `sha256:fe81560990fae86dc4743202b2cb4d3df98d0bcf949173cecef15ce1127ef852`.
- [GitHub: ARM64 standard per repository pubblici](https://github.blog/changelog/2025-08-07-arm64-hosted-runners-for-public-repositories-are-now-generally-available/).
- [GitHub: specifiche dei runner](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).

## English

The dedicated workflow provisions real CI environments without requiring the
user's hardware or paid runners. A configured job is not a passed test; inspect
the result for the exact commit.

Fedora 44 is built and tested in its official, digest-pinned x86_64 container. A
separate fresh container installs the resulting RPM through DNF, permitting only
automatically resolved runtime dependencies. It rejects compiler/development
packages, runs the installed CLI and Qt offscreen GUI on synthetic fixtures,
compares all eight product files, and removes the package while preserving input
data. This is real Fedora userspace, not a complete Fedora VM or desktop session.

The `ubuntu-24.04-arm` job asserts `aarch64`, builds/tests on a native ARM64 runner
and verifies the Debian package architecture. A fresh Ubuntu ARM64 container then
resolves and installs the DEB through APT without development packages and checks
the same installed processing flow. It uses neither QEMU nor cross-compilation.
Container userspace is clean; its kernel is shared with the hosted runner.

Source is read-only in the build container, and the runtime container sees only
verification scripts/artifacts. Logs record image digests, actual package versions
and architecture. Artifacts expire after 14 days. Synthetic imagery validates the
real platform execution, not real-acquisition geometry or radiometry. GPU, complete
interactive desktop VM/screen-reader coverage, SatDump real-data validation and
AppImage bundling remain separate acceptance gates.

## Evidenza remota / Remote evidence

Verifica effettiva conclusa con successo sul commit `0d7bf195b6af4ab2d2392c87234945accf499ede`: [run 42501765159](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765159). Il risultato vale per le verifiche descritte, senza estenderlo a piattaforme o identità diverse. / The successful run covers the checks described here, without implying other platforms or signer identities.

Verifica ripetuta con Natural Color e harness aggiornato: [run42517397266](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397266), commit `9a52e07d1e941b155ce9194fe019fba4de1d4549`, `success`. Per la VM completa con kernel proprio vedere [FEDORA_VM.md](FEDORA_VM.md). / Revalidated successfully on this revision.
