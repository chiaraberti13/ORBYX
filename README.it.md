<p align="center"><img src="assets/banner.svg" alt="ORBYX — Satellite Imaging Studio" width="100%"></p>

<p align="center"><a href="README.md">🇬🇧 English</a> · <a href="README.it.md">🇮🇹 Italiano</a></p>

<p align="center">
<img src="https://img.shields.io/badge/status-sviluppo_iniziale-F2C94C?style=flat-square" alt="Sviluppo iniziale">
<img src="https://img.shields.io/badge/category-SATELLITE_IMAGING-22D3EE?style=flat-square" alt="Immagini satellitari">
<img src="https://img.shields.io/badge/stack-C%2B%2B17_%7C_Qt_6-8B949E?style=flat-square" alt="C++17 e Qt 6">
<img src="https://img.shields.io/badge/languages-EN_%7C_IT-8B5CF6?style=flat-square" alt="Inglese e italiano">
<img src="https://img.shields.io/badge/licence-GPL--3.0-2EA043?style=flat-square" alt="GNU GPL versione 3">
</p>

> Allinea i canali. Rivela la Terra. Un ambiente desktop e headless indipendente per allineare i canali satellitari RDAS.

<p align="center"><a href="ROADMAP.md">Roadmap</a> · <a href="PROGRESS.md">Verifiche</a> · <a href="docs/AUDIT.md">Audit</a> · <a href="SECURITY.md">Sicurezza</a> · <a href="THIRD_PARTY_NOTICES.md">Attribuzioni</a></p>

## Panoramica

ORBYX modernizza [RDAS Layer Aligner](https://github.com/Digitelektro/rdas_layer_aligner), mantenendone l'approccio affine/ECC OpenCV e i coefficienti di calibrazione. Lavora sui prodotti `MSUGS_VIS1` e `MSUGS_VIS2` decodificati in tempo reale da SatDump. Non riceve segnali satellitari e non decodifica direttamente il downlink.

È una prima implementazione, non una release definitiva. Sono disponibili regressioni sintetiche; dataset satellitari reali, geometria della reimportazione SatDump, equivalenza GPU e matrice completa delle piattaforme restano requisiti prima della release. I risultati effettivi sono in [PROGRESS.md](PROGRESS.md).

![ORBYX desktop](assets/desktop.png)

## Funzioni implementate

- Core C++17 e CLI headless indipendenti, con desktop opzionale Qt 6 Widgets.
- Calibrazione affine ECC: 500 iterazioni, epsilon 1e-7, offset canali 3606/1804 preservati.
- Allineamento PNG in scala di grigi a 8/16 bit, fusione sinistra/destra tramite ROI oppure prodotti allineati separati.
- Corretta provenienza del CBOR VIS2 e pixel di riempimento della fusione inizializzati a zero.
- Controlli preventivi delle intestazioni PNG, dimensioni, matrici, ROI e scritture.
- Output separato pubblicato tramite rinomina atomica della cartella; destinazioni esistenti rifiutate. Dati e profili originali conservati.
- Desktop con scelta origine/profilo/destinazione, elaborazione asincrona, anteprima origine/risultato, zoom/pan, log, lingua e tema chiaro/scuro persistenti.
- Interfaccia, aiuto CLI e diagnostica applicativa in italiano/inglese; i dettagli tecnici delle librerie possono mantenere la lingua originale.

Sono conservati i coefficienti L2, L3, **L5**, M1 e M2. L4 richiede una calibrazione perché non ha coefficienti inclusi. Riconoscimento automatico del satellite, code batch desktop, monitoraggio cartelle, editor guidato delle ROI, Natural Color e aggiornamenti applicativi sono pianificati. L'opzione `-n/--nc` era disabilitata nell'originale e non viene presentata come funzionante.

## Compilazione e installazione

Dipendenze Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build libopencv-dev libjsoncpp-dev qt6-base-dev qt6-qpa-plugins
git clone https://github.com/chiaraberti13/ORBYX.git
cd ORBYX
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
./build/orbyx-gui
```

Per la sola CLI aggiungere `-DORBYX_BUILD_GUI=OFF`: Qt non sarà necessario. Non serve un SDK OpenCL diretto; OpenCV seleziona il backend disponibile, con fallback CPU. `--cpu` forza il percorso CPU di riferimento.

Installazione facoltativa: `sudo cmake --install build --prefix /usr/local`. Il profilo installato si trova in `/usr/local/share/orbyx/calibration.json`: indicarlo esplicitamente alla CLI. Le preferenze desktop usano le impostazioni Qt dell'utente. L'elaborazione non richiede rete né privilegi amministrativi.

## Uso della CLI

```bash
./build/orbyx --help
./build/orbyx --lang it --help
./build/orbyx --version
./build/orbyx generate --lang it -s L3 -d /data/rdas \
  --config resources/calibration.json --output /data/allineati-nuovi --cpu
./build/orbyx calibrate --lang it -s L4 -d /data/rdas \
  --config resources/calibration.json --output /data/calibrazione-nuova --cpu
```

La cartella di output non deve esistere e la cartella che la conterrà deve già esistere. La calibrazione scrive un **nuovo** `calibration.json` nella destinazione, preservando le voci degli altri satelliti. Le ROI di fusione richiedono una verifica manuale. Per utilizzare il nuovo profilo passarlo con `--config`; non presumere che le ROI esistenti vadano bene per una nuova acquisizione.

L'input richiede due cartelle VIS con `MSUGS-VIS-1.png`, `MSUGS-VIS-2.png`, `MSUGS-VIS-3.png`. La generazione richiede anche il relativo `product.cbor`. I canali devono essere PNG in scala di grigi senza segno a 8/16 bit, con dimensioni uguali nel singolo prodotto VIS e altezza superiore a 3606. Limiti preventivi: 150 milioni di pixel per canale e 1 GiB per file PNG codificato. Non costituiscono un budget di memoria complessivo: i prodotti grandi possono richiedere diversi GiB di RAM.

Exit code `0` per successo, `2` per errori. Restano disponibili `-s`, `-d`, `-m`; `--output` è obbligatorio per evitare sovrascritture accidentali. Senza `--config` la CLI cerca `config.json`.

Il CBOR viene copiato senza modifiche e non ricalibrato. Una copia corretta dei byte non dimostra l'interoperabilità geometrica con SatDump.

## Elaborazioni batch

Un runner opzionale Python 3.10+ esegue lavori sequenziali con stato persistente, profili per lavoro e ripresa verificata tramite digest. Vedi [uso del batch e gestione dei fallimenti](docs/BATCH.md). È separato dalla coda desktop.

## Architettura e sviluppo

- `include/orbyx`, `src/core`: immagini, profili e pubblicazione controllata dell'output.
- `src/cli`: argomenti e funzionamento headless.
- `src/gui`: interfaccia Qt e coordinamento del worker.
- `resources`: coefficienti originali e voce desktop.
- `tests`: regressioni con immagini sintetiche e input non validi.
- `third_party/rdas_layer_aligner`: riferimento originale fissato a un commit, escluso dalla build normale.
- `docs`: [decisione sui linguaggi](docs/ADR-001-language.md), audit e verifiche.

Per i sanitizers usare `-DORBYX_SANITIZE=ON -DORBYX_BUILD_GUI=OFF -DCMAKE_BUILD_TYPE=Debug`, quindi CTest. La CI prevede Ubuntu 22.04/24.04/26.04, sanitizers e analisi statica: la configurazione dei job non dimostra che siano passati. CPack consente packaging sperimentale, non una release già verificata. Debian/Fedora/Arch/Mint, ARM64, macOS e AppImage restano da verificare.

## Sicurezza e licenza

Usare privilegi ordinari e cartelle di destinazione fidate. Decoder nativi e driver GPU restano confini di fiducia: aggiornare le dipendenze di sistema. Consultare [sicurezza](SECURITY.md), [contributi](CONTRIBUTING.md) e [condotta](CODE_OF_CONDUCT.md).

GNU GPL versione 3, con attribuzioni originali e avvisi sulle dipendenze. [LICENSE](LICENSE), [NOTICE](NOTICE), [inventario delle dipendenze](THIRD_PARTY_NOTICES.md). Nessuna affiliazione con il progetto originale. Le fotografie dimostrative originali non sono redistribuite.

[Verification record / Registro verifiche](docs/VERIFICATION.md) · [Memory limits / Limiti memoria](docs/RESOURCE_LIMITS.md)
