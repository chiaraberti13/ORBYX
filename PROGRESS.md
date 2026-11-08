# ORBYX — Avanzamento

## 2026-11-08

Repository inizialmente vuoto. Roadmap creata prima dello sviluppo. Upstream di riferimento: Digitelektro/rdas_layer_aligner, commit 6aee5b4ba96b528389890ea6442fc96a7344a6ff. Audit in corso. Nessuna compatibilità ancora verificata.

M0-01 e M0-02: audit statico completo e ADR-001 pubblicabili. Sorgenti originali e config conservati in third_party/rdas_layer_aligner; immagini dimostrative non importate. Push git HTTPS privo di credenziali: pubblicazione tramite connettore GitHub autorizzato, mantenendo main e controllo del parent.

M0-03: analizzati i README dei tre repository di riferimento. Banner ORBYX originale, README EN/IT, licenza GPLv3 preservata e avvisi di provenienza completati. XML del banner valido. Nessuna fotografia originale redistribuita.

M1-01: core C++17 e CLI EN/IT implementati. Build GCC 13.3 / OpenCV 4.6 / JsonCpp 1.9.5 su Ubuntu 24.04 x86_64. Suite sintetica: 8/16 bit, affine con traslazioni note, calibrazione ECC (<0,1 pixel), rigenerazione (differenza massima <=1), ROI, padding zero, CBOR distinti VIS1/VIS2, rollback, PNG/header/file grandi, JSON duplicati, dimensioni incoerenti e symlink. Cinque gruppi CTest normali passati (include smoke GUI preliminare).

ASan+UBSan: quattro gruppi headless passati con detect_leaks=0. LeakSanitizer non eseguibile nel runtime locale perché /proc/task non accessibile; il primo tentativo con detect_leaks=1 ha fallito per questa limitazione ambientale, non è una verifica leak superata. Le librerie distro non sono strumentate. Build e test usano dipendenze Ubuntu estratte localmente poiché l'installazione globale non è disponibile. Nessuna prova su GPU o dati reali.

M2-01: GUI Qt Widgets con worker asincrono, selezione origine/profilo/destinazione, modalità calibrazione/allineamento, preview zoom/pan e preferenze lingua/tema persistenti. Screenshot verificati IT/scuro 1280x860 ed EN/chiaro 960x680; pannello scorrevole alle dimensioni ridotte. Build e smoke offscreen passati. Nessun test con screen reader o sessione interattiva reale ancora eseguito; resta M4-01. Screenshot principale in assets/desktop.png.

M2-02: runner Python3.10+ opzionale headless, lavori sequenziali, profilo per job, stato atomico fsync, lock consultivo, digest di input/profilo/binario/output, ripresa e gestione Ctrl+C. Quattordici test standard-library passati, comprendenti conflitti percorsi, lock, FIFO/hardlink, drift e conservazione della provenienza, localizzazione e ripresa fra lingue. Coda desktop ancora M2-03.

Verifica aggiuntiva desktop: drag-and-drop cartella e selettore CH1/2/3 implementati. Test gui-product-flow esegue realmente il worker GUI su fixture RDAS e confronta tutti gli 8 file prodotti con la CLI; identità byte verificata e sovrascrittura rifiutata con exit2 senza dialogo bloccante. Sette gruppi CTest normali passati.

M1-02 in corso: workflow con runner Ubuntu 22.04/24.04/26.04, build/test desktop, test headless ASan/UBSan con leak detection attiva in CI, Cppcheck, CodeQL per core/GUI e Dependabot Actions. Azioni fissate a commit; cron settimanali eseguiti da GitHub, non un'automazione personale. Cppcheck2.13 locale passato. CPack TGZ e installazione locale riusciti; Debian package sperimentale richiesto nei job, non ancora dichiarato verificato. Inventario vulnerabilità dei pacchetti distro e SBOM restano da implementare.

Prima CI remota 4d569ee: Ubuntu24.04 build, 7 gruppi CTest e pacchetto DEB completati; sanitizers con detect_leaks=1 e Cppcheck passati. Ubuntu22.04 ha rivelato OpenGL_INCLUDE_DIR mancante durante configurazione Qt: aggiunta dipendenza esplicita libgl-dev in workflow e istruzioni, nuova matrice necessaria. Ubuntu26.04 e CodeQL ancora in esecuzione al momento di questa nota.

Packaging: aggiunti icona SVG desktop originale, installazione standard hicolor, opzione ORBYX_INSTALL_BATCH e dipendenza Python>=3.10 esplicita nei DEB/RPM. Installazione locale e pacchetto sorgenti ispezionati; RPM/AppImage non verificati. Aggiunti smoke degli eseguibili installati nei job DEB. Checkout aggiornato al tag v6 risolto a SHA e credenziali non persistenti. Push di sola documentazione non ripetono build già eseguite; cron e PR mantengono i controlli.

Documentazione audit e ADR ora integralmente disponibile EN/IT con navigazione. M3-01 bloccato dalla mancanza di acquisizioni RDAS reali redistribuibili e ambiente SatDump; non è stato sostituito con una verifica fittizia.

## Chiusura sessione — 2026-11-08

Task verificati: M0-01 audit, M0-02 ADR, M0-03 identità/documentazione, M1-01 core/CLI, M1-02 test/CI, M2-01 desktop iniziale, M2-02 batch headless. M3-02 packaging parziale verificato per TGZ e DEB, non per RPM/AppImage/ARM64. Nessuna modifica pubblicata al repository originale.

### Evidenza finale

Revisione di codice/build verificata: `4190002a1212dba0acf24c2f3d0a168dd7fe9813`.

- [Build and regression 42477106167](https://github.com/chiaraberti13/ORBYX/actions/runs/42477106167): tutti i job success. Ubuntu22.04/24.04/26.04 x86_64: build, sette gruppi CTest, DEB, installazione sul runner e avvio CLI/GUI offscreen/batch installati. Artifacts sorgenti+DEB disponibili nel run, conservati14giorni.
- Batch: quattordici test annidati nel gruppo batch-regression, inclusi input/output drift, lock, conflitti, provenienza e lingua.
- Sanitizers remoto: cinque gruppi headless passati con ASan, UBSan e detect_leaks=1. La limitazione locale LeakSanitizer rimane documentata, ma la verifica remota è stata eseguita.
- Cppcheck: successo locale e remoto.
- [CodeQL 42477106212](https://github.com/chiaraberti13/ORBYX/actions/runs/42477106212): analisi completata con successo. Non equivale a una garanzia di assenza di vulnerabilità.
- Banner/icona SVG e collegamenti della documentazione verificati; screenshot IT/scuro1280x860 ed EN/chiaro960x680 ispezionati.

### Commit pubblicati su main

| Commit | Contenuto |
|---|---|
| 55a18bf | Istruzioni iniziali |
| 2f40a1a | Roadmap, audit, ADR e baseline originale |
| fc7f5b8 | Banner, README EN/IT, licenze e attribuzioni |
| a66b4ca | Motore validato e CLI bilingue |
| 4fe1614 | GUI e verifica visiva |
| 96c133d | Batch durevole e regressione completa GUI→output |
| 4d569ee | CI, sanitizers, CodeQL e packaging |
| 5e2ad9b | Correzione dipendenza OpenGL su Ubuntu22.04 |
| 4190002 | Metadati pacchetti, icona e smoke dopo installazione |

### Limiti e prossimo task

M3-01 resta bloccato: servono acquisizioni RDAS reali redistribuibili con provenienza e un ambiente SatDump per verificare geometria/metadati. Nessuna equivalenza GPU, supporto ARM64/macOS/altre distro, installazione su VM pulita, RPM/AppImage, fuzzing esteso, release firmata o accessibilità con screen reader è stata dichiarata completata.

Prossimo task eseguibile: M2-03, coda desktop e gestione dei profili, riusando il contratto batch già verificato. M1-03, inventario runtime/SBOM/scansione vulnerabilità dei pacchetti, è un'attività indipendente. Natural Color, riconoscimento automatico, monitoraggio cartelle, onboarding e aggiornamenti sicuri restano nei task dedicati della roadmap. Nessun lavoro continua automaticamente fuori dalla sessione salvo i workflow GitHub configurati.

## Verifica indipendente — 2026-11-08

Audit di revisione eseguito in un container pulito, separato dalla sessione di sviluppo, per riprodurre le dichiarazioni senza fidarsi dello stato precedente. Ambiente: Ubuntu 24.04.5 LTS x86_64, GCC 13.3.0, CMake 3.28.3, OpenCV 4.6.0+dfsg-13.1ubuntu1, JsonCpp 1.9.5-6build1 (dipendenze installate dai repository Ubuntu nel container).

Riproduzione effettiva su commit `f5afb98` (identico a `origin/main` al momento dell'audit; le quattro PR Dependabot aperte non sono state integrate):

- Configurazione e build headless (`-DORBYX_BUILD_GUI=OFF`) riuscite; nessun warning emesso dal core/CLI/test con `-Wall -Wextra -Wpedantic`.
- CTest: 5/5 gruppi non-GUI superati (`batch-regression`, `engine-regression`, `cli-help`, `cli-version`, `cli-invalid`). `engine-regression` esegue realmente la pipeline ECC di OpenCV e verifica le traslazioni sintetiche recuperate entro 0,1 px e la rigenerazione entro 1 livello su fixture 8 e 16 bit, oltre a merge con padding azzerato, rollback, rifiuto symlink/ROI non valide e provenienza CBOR distinta VIS1/VIS2.
- Suite batch Python: 14/14 test standard-library superati (schema, lock advisory, alias hardlink, FIFO, drift input/output/config/engine, ripresa verificata e localizzazione EN/IT).
- Gruppi GUI (`gui-product-flow`, `gui-smoke`) e sanitizers non rieseguiti in questo container (Qt6 non installato; build Debug/ASan non ripetuta): restano coperti dalla CI remota.
- Evidenza remota confermata via API GitHub: run «Build and regression» [42477106167](https://github.com/chiaraberti13/ORBYX/actions/runs/42477106167) e CodeQL [42477106212](https://github.com/chiaraberti13/ORBYX/actions/runs/42477106212), entrambi conclusione `success` sul commit `4190002` di `main` (push).

Esito: lo stato documentato è coerente con il codice e con le esecuzioni reali; nessuna sovra-dichiarazione di piattaforme, precisione o capacità rilevata. Le deviazioni dall'upstream (canvas di merge dimensionato sulle ROI e azzerato — A01; salvataggio dei canali allineati — A05; provenienza CBOR per-sensore — A06) sono documentate in `docs/AUDIT.md` e coperte da test di regressione. Nessun difetto tecnico accertato da correggere: non sono state introdotte modifiche al codice per non aggiungere interventi superflui. Verifiche aperte invariate (M3-01 dati reali/SatDump, GPU, ARM64/macOS/altre distro, RPM/AppImage, VM pulita, fuzzing esteso, release firmata, accessibilità con screen reader).

## Ripresa dopo annotazioni — 2026-11-08

Recepita la verifica indipendente di Claude (49c8171, merge e0463d1), che conferma lo stato precedente senza richiedere correzioni alla roadmap. Avviato M2-03: integrazione della coda durevole nel desktop e gestione dei profili, mantenendo il controller batch già verificato.

Durante la preparazione della coda sono stati rafforzati parsing e lettura dei manifest/stati: snapshot con limite sui byte effettivamente letti, rifiuto di chiavi duplicate, numeri non finiti e nesting eccessivo; versione dello stato strettamente intera. Diciassette test batch passati; gli stati non validi restano invariati e non avviano il motore. La verifica precedente di Claude resta riferita al suo commit, senza attribuirle questa modifica successiva.

## M1-03 inventario runtime/SBOM (parziale) — 2026-11-08

Attività indipendente, separata dalla coda desktop (M2-03) in corso in parallelo. Implementato `scripts/orbyx_sbom.py`: inventario JSON deterministico delle librerie native effettivamente caricate dagli eseguibili ORBYX, risolte a soname→percorso→pacchetto→versione tramite `readelf -d`, `ldd` e `dpkg-query`, con componenti ORBYX e requisito Python opzionale. Il core di parsing è puro e testabile con resolver iniettati; nessun accesso di rete e nessuna dichiarazione di assenza vulnerabilità.

Verifica effettiva su `main` (Ubuntu 24.04.5, OpenCV 4.6.0, JsonCpp 1.9.5): generazione reale su `build/orbyx` e `build/orbyx_tests` con risoluzione corretta dei pacchetti (es. `libopencv-core406t64` 4.6.0+dfsg-13.1ubuntu1, `libjsoncpp25` 1.9.5-6build1, `libc6` 2.39-0ubuntu8.9). Nuovo gruppo CTest `sbom-regression` (6/6 gruppi non-GUI passati) con test di unità su parsing risolto/non risolto/loader e integrazione che confronta l'output con `readelf -d` sui binari reali. CI: nuovo passo che genera l'inventario dai binari installati e lo pubblica come artifact per-OS. Documentazione in `docs/SBOM.md`.

Limiti dichiarati: la correlazione automatica con un feed di sicurezza (Debian/Ubuntu tracker, OSV) e l'export in formato standard CycloneDX/SPDX non sono implementati; il passo di scansione avvisi resta documentato, non eseguito offline. M1-03 è quindi «verificato in parte», non completato.

## M3-03 harness di fuzzing (parziale) — 2026-11-08

Attività indipendente dalla coda desktop (M2-03). Implementato `tests/fuzz_parsers.cpp`, harness libFuzzer sulla superficie di input non fidata raggiungibile dall'API pubblica del core: lettura PNG limitata, validazione IHDR e budget dimensioni e decoder (via `inspect()`), più il catalogo di localizzazione (via `translate()`). Nuova opzione CMake `ORBYX_FUZZ` (richiede Clang, fallisce subito con altri compilatori): strumenta `orbyx_core` con `fuzzer-no-link,address,undefined` e costruisce l'eseguibile con `-fsanitize=fuzzer,address,undefined`. Gruppo CTest `fuzz-parsers-smoke` (`-runs=8000`) e job CI `fuzz` su ubuntu-24.04 con campagna a tempo (`-max_total_time=60`).

Verifica effettiva su `main` (Ubuntu 24.04.5, Clang 18.1.3, con `libclang-rt-18-dev`): build dell'harness riuscita; 20000 run completate senza crash/leak/UB; gruppo CTest `fuzz-parsers-smoke` passato in ~7,5 s. La dir di build `fuzz/` è aggiunta a `.gitignore`.

Limiti dichiarati: i codec nativi (OpenCV) non sono strumentati; il fuzzing del profilo JSON e dell'intera pipeline `run()`, un corpus persistente e i benchmark/equivalenza GPU-OpenCL (resto di M3-03) restano aperti e richiedono hardware reale. M3-03 è quindi «verificato in parte», non completato. Documentazione in `docs/FUZZING.md`.
