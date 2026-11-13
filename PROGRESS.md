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

## M3-02 packaging RPM (avanzamento) — 2026-11-08

Difetto di metadati accertato e corretto: il pacchetto RPM riportava `License : unknown` e `Group : unknown`. Aggiunti i tag CPack mancanti (`CPACK_RPM_PACKAGE_LICENSE "GPLv3"`, gruppo, URL) e, per il DEB, `Section: science` e `Homepage`. Verifica effettiva su Ubuntu 24.04.5 con `rpmbuild` 4.18.2: generazione RPM riuscita, `License: GPLv3`, requires automatici corretti (`libopencv_core.so.406`, `libjsoncpp.so.25`, `libc`, `python3 >= 3.10`), payload con CLI/batch/calibrazione/licenze; DEB rigenerato con Section/Homepage e Depends auto-risolti. La CI ora genera DEB+RPM, stampa `rpm -qpi` e fallisce se la licenza non è `GPLv3`; gli artifact includono il `.rpm`.

Limiti dichiarati: la generazione RPM su Ubuntu non implica compatibilità Fedora; installazione su una distribuzione RPM reale, AppImage, ARM64 e installazione su VM pulita restano da verificare. M3-02 resta «in corso».

## M1-03 export CycloneDX — 2026-11-08

Chiuso uno dei limiti dichiarati per M1-03: aggiunto `--format cyclonedx` a `scripts/orbyx_sbom.py` (funzione pura `to_cyclonedx`), che emette un BOM CycloneDX 1.5 con l'applicazione ORBYX nei metadati (licenza `GPL-3.0-only`) e ogni pacchetto risolto come componente `library` con purl `pkg:deb/<distro>/<nome>@<versione>`, deterministico e deduplicato. Nuovi test (`CycloneDxTests` + integrazione sui binari reali): 13 test SBOM passati. La CI genera anche il BOM CycloneDX per-OS. Restano aperti la correlazione con un feed di sicurezza e l'export SPDX.

## M1-03 export SPDX — 2026-11-08

Aggiunto anche `--format spdx` (funzione pura `to_spdx`): documento SPDX 2.3 JSON con ORBYX come pacchetto descritto (`licenseDeclared` GPL-3.0-only), ogni pacchetto risolto con `externalRefs` purl deb, SPDXID sanitizzati alla grammatica SPDX (es. `libstdc++6`) e deduplicati, timestamp `created` iniettabile per riproducibilità sotto test. Nuovi test (`SpdxTests` + integrazione reale): 17 test SBOM passati. La CI pubblica anche il documento SPDX per-OS. Con CycloneDX e SPDX entrambi presenti e verificati, per M1-03 resta aperta soltanto la correlazione automatica con un feed di vulnerabilità, che richiede il feed della distribuzione e non è eseguibile offline.

## M2-03 coda desktop e duplicazione profili — 2026-11-08

Implementato M2-03 riusando i componenti già verificati. (1) **Duplicazione profili nel core**: `orbyx::duplicateProfile` copia le voci di un satellite su un nuovo identificativo, preservando tutte le voci originali, con pubblicazione atomica esclusiva (nessuna sovrascrittura) e rifiuto di id non validi/satellite mancante/destinazione esistente; coperta da nuovi casi in `engine-regression`. (2) **Coda desktop**: `orbyx-gui --queue MANIFEST` delega al runner batch già verificato (nessuna shell, nessuna rete, solo argomenti espliciti), ereditando stato durevole, ripresa con digest e nessuna perdita dati; nuovo test `gui-queue-flow` verifica che ogni lavoro in coda produca gli stessi byte della CLI e che `--resume` su input invariati completi senza perdite. (3) **Widget visibili**: gruppo «Coda di elaborazione» con tabella (lavoro/operazione/stato) e pulsanti aggiungi/rimuovi/avvia, più pulsante «Duplica profilo…»; lo stato di ogni lavoro è letto dal file di stato durevole (indipendente dalla lingua).

Verifica effettiva su Ubuntu 24.04.5 (Qt 6.4.2): build GUI pulita senza warning; 9/9 gruppi CTest passati (inclusi `gui-product-flow`, `gui-smoke`, `gui-queue-flow`); screenshot offscreen IT/scuro 1320×900 ispezionato con coda e pulsante profilo renderizzati e layout integro. Limite dichiarato: l'esecuzione della coda e la duplicazione profili sono verificate da test automatici a livello di esecuzione e di core; l'interazione completa dei widget (click su aggiungi/avvia, dialog di duplicazione) e la verifica con screen reader non sono coperte da test automatici e restano aperte (si legano a M4-01). M2-03 è quindi «verificato in parte».

## M4-02 integrità release (avanzamento) — 2026-11-08

Primo passo di M4-02: la CI genera un `SHA256SUMS` riproducibile su tutti gli artifact di release (DEB, RPM, archivio sorgenti e documenti SBOM/CycloneDX/SPDX), lo verifica in loco con `sha256sum -c` e lo pubblica tra gli artifact. Flow verificato in locale su Ubuntu 24.04.5 (generazione + `sha256sum -c` → OK su deb/rpm/json/sorgenti). Documentazione in `docs/PACKAGING.md`. Limite dichiarato: i checksum garantiscono l'integrità, non l'autenticità; la firma GPG delle release e la procedura di aggiornamento/rollback richiedono chiavi di firma e restano da implementare. M4-02 resta «in corso».

## Simulazione sintetica end-to-end (M3-01 resta bloccato) — 2026-11-08

Controllato il repository originario: la cartella `img/` contiene solo composite di **output** (JPEG dimostrativi), non dataset RDAS di input grezzi; i dati reali nascono solo da una decodifica SatDump di un passaggio satellitare. Non esiste quindi un dataset reale ridistribuibile né qui né upstream.

Aggiunto `scripts/orbyx_simulate.py` (Python puro, nessuna dipendenza): genera un'acquisizione sintetica in forma MSU-GS (due VIS, tre PNG 8 bit con offset reali CH2=3606/CH3=1804 e `product.cbor` segnaposto) con uno shift orizzontale **noto** iniettato, poi esegue il ciclo reale `calibrate`→`generate` del motore e verifica il recupero degli offset e dello shift entro 1 px e la creazione dei prodotti allineati. I PNG sono scritti da un encoder autonomo, quindi il test esercita anche il decoder su input codificato da terzi. Nuovo gruppo CTest `simulation-flow` (10/10 gruppi passati). Esecuzione reale 2026-11-08 (Ubuntu 24.04.5): «Calibration recovered CH2 ty≈3606, CH3 ty≈1804, injected shift≈3: OK».

Dichiarazione esplicita: è una **simulazione su dati sintetici**. Prova la meccanica della pipeline; **non** verifica precisione radiometrica/geometrica su acquisizioni reali, qualità di calibrazione su passaggi veri, né la geometria di reimport SatDump (CBOR copiato, mai validato). L'accettazione di **M3-01** resta quindi **bloccata**, così come M3-04 (Natural Color) e M2-04 (monitoraggio cartelle) che vi dipendono. Nessun risultato è stato dichiarato verificato su dati reali. Documentazione in `docs/SIMULATION.md`.

## M3-03 benchmark CPU reale — 2026-11-08

Aggiunto `scripts/orbyx_benchmark.py`: genera un dataset sintetico e misura i tempi **reali** end-to-end di `calibrate` e `generate` sul backend CPU. Misura effettiva su Ubuntu 24.04.5 x86_64 (host di sessione), immagine 384×4022 (1,54 Mpx), 3 iterazioni, `--cpu`: `generate` min ~81 ms / mediana ~85 ms; `calibrate` min ~32,1 s / mediana ~33,6 s. Il `generate` è veloce; il `calibrate` è dominato dall'ECC (500 iterazioni, preservate dall'upstream) e cresce con l'area — su acquisizioni reali (~80 Mpx) sarà molto più lento: è un costo dell'algoritmo preservato, non una regressione. Documentazione in `docs/PERFORMANCE.md`. Sono misure CPU reali, **non** un confronto GPU: l'equivalenza GPU/OpenCL resta aperta (serve hardware). Il benchmark è uno strumento documentato, non un gate di CI (troppo lento per ogni run).

## M2-04 riconoscimento prodotti (simulazione) — 2026-11-08

Aggiunto `scripts/orbyx_detect.py` (Python puro, sola lettura): classifica una cartella come `complete`/`incomplete`/`unstable`/`not_product` in base alla struttura RDAS (MSUGS_VIS1/2 con i 3 PNG + `product.cbor`) e alla stabilità delle dimensioni tra due campioni (un prodotto ancora in scrittura è «instabile» e trattenuto). Rifiuta symlink e file vuoti. La scansione **elenca** i prodotti pronti senza avviare alcun job e senza toccare i dati: il controllo manuale resta all'utente (accettazione M2-04). Nuovo gruppo CTest `detect-regression` (7 casi su dataset simulati: completo, canale mancante, cartella VIS mancante, file vuoto, symlink, crescita/instabilità, scansione read-only che non modifica i dati). Limite dichiarato: è verificato **in simulazione**; l'import automatico e la disambiguazione di metadati SatDump reali o ambigui restano aperti e dipendono da M3-01 (dati reali). Nessuna rete né shell implicita introdotta.

## M3-04 base di composizione RGB (simulazione) — 2026-11-08

Aggiunta `orbyx::composeRgb` nel core: compone un prodotto RGB a 3 canali (CH1→R, CH2→G, CH3→B) dai canali allineati, preservando la profondità 8/16 bit **senza conversione implicita né perdita**, lasciando immutati gli input e pubblicando con rename atomico esclusivo (nessuna sovrascrittura). `readImage` è stato reso parametrico (`rdas=false`) per accettare i prodotti allineati già ritagliati (altezza < 3606) mantenendo tutti gli altri controlli di formato e i limiti di dimensione. Nuovi casi in `engine-regression` (11/11 gruppi CTest): forma/profondità RGB preservate, mappatura canali 8 e 16 bit, input invariati, destinazione non sovrascritta, rifiuto di profondità incoerente. Limite dichiarato: è la **base meccanica** della composizione, verificata in simulazione; la correzione cromatica Natural Color (passo hue, 8-bit nell'upstream) e la validazione su **scene reali** (accettazione «fixture cromatiche e scene reali») restano aperte e dipendono da M3-01. Non ho dichiarato validato il colore su scene reali.

## M4-02 firma release — meccanica (simulazione) — 2026-11-08

Aggiunto `scripts/orbyx_sign.py`: passo esplicito di release (nessuna rete, nessuna shell implicita) che firma e verifica una firma GPG staccata su `SHA256SUMS`. Lo strumento non genera né incorpora chiavi: la chiave è fornita dall'operatore via keyring/GNUPGHOME. Nuovo gruppo CTest `sign-regression` (3 casi): genera una **chiave effimera** di test in un `GNUPGHOME` isolato, firma, verifica («Good signature»), e conferma che la manomissione del file fa fallire la verifica. Verifica effettiva su Ubuntu 24.04.5 con GnuPG 2.4.4. Documentazione in `docs/PACKAGING.md`. Limite dichiarato: è provata la **meccanica** firma/verifica con chiave effimera; la firma con la **chiave reale** di release (secret fuori dal repo) e la procedura di aggiornamento/rollback restano aperte. M4-02 passa da «in corso» a «verificato in parte».

## Risorse reali e verifiche indipendenti — 2026-11-08

Ripartenza dal commit7fc0953, preservando il lavoro remoto e senza sovrascrivere il checkout precedente. Recuperata acquisizione RDAS reale Elektro-L4 dall'issue2 del progetto originale:769938432byte, SHA256 b92ded3c71f1752d12208ed358e0d392e05d2ddbefe97f2e72b69e3c09498d48; decodifica/validazione ancora in corso. Il raw resta esterno al repository: la pagina non concede una licenza esplicita di redistribuzione.

Aggiunti workflow separati per build/installazione Fedora44 in container puliti e ARM64 nativo GitHub, e per attestazione autentica Sigstore/GitHub OIDC dei pacchetti senza chiave personale. YAML, Bash e pin delle risorse verificati; esiti remoti ancora da osservare. Un container Fedora reale non viene descritto come una VM completa. La firma OIDC certifica il workflow ORBYX, non impersona la firma GPG personale dell'utente.

## Ripresa dalla release CADU e verifiche reali — 2026-11-08

La release `rdas-cadu-l4` ora ospita il campione originale come `elektro_rdas.cadu`:769938432byte, SHA256 b92ded3c71f1752d12208ed358e0d392e05d2ddbefe97f2e72b69e3c09498d48. Nessun nuovo commit Claude successivo a0d7bf19 rilevato all'avvio; preservati i suoi sviluppi precedenti e il checkout con modifiche locali incompiute. Nuovo worktree isolato dalla revisione remota aggiornata.

Evidenza remota prima delle nuove modifiche, commit0d7bf19: [piattaforme native42501765159](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765159), [AppImage42501765193](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765193), [attestazione42501765172](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765172), [regressioni42501765205](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765205) e [CodeQL42501765143](https://github.com/chiaraberti13/ORBYX/actions/runs/42501765143): tutti `success`. Fedora e installazioni pulite sono container reali con kernel condiviso, ARM64 è nativo; nessuna VM grafica completa dichiarata. Sigstore/OIDC verifica l'identità del workflow e rifiuta manomissioni, senza chiave GPG personale.

Nuovo avanzamento M3-01: scaricati e verificati realmente raw e AppImage ufficiale SatDump2.0.0-alpha-1aedd60 (digest in docs/REAL_RDAS.md). Decodifica autentica a sei PNG6004×17200 uint16 con CBOR reali; calibrazione CPU esplicita a scala0,125 riuscita (5,36s,863364KiB RSS), generazione alla risoluzione originale6004×13594 riuscita (12,58s,1865716KiB). Confronto indipendente Mat/warpAffine Python OpenCV4.6 contro motore C++/UMat OpenCV4.6: differenza massima zero su tutti i pixel dei sei canali. Entrambi i prodotti allineati riaperti dal processore headless SatDump: letti tutti e tre i PNG di ciascun VIS, uscita0. I CBOR per-VIS restano identici, transform.type=1/NONE.

Limiti emersi e mantenuti: sorgente senza proiezione/calibrazione utilizzabile, quindi avvisi SatDump `Using Dummy`/`Not a standard projection` conservati; nessuna geolocalizzazione assoluta o precisione rispetto a verità a terra certificata. Stime a scala0,125/0,25 differiscono fino a circa6px agli angoli VIS2; nessun profilo L4 universale installato. Un confronto iniziale con Python OpenCV5 mostrava differenze di interpolazione rispetto al motore4.6: il riferimento è ora esplicitamente4.6 e non viene esteso ad altre versioni. Natural Color reale, ROI di merge e GPU restano aperti.

Implementati download esplicito con size/SHA256 e pubblicazione esclusiva, manifest di provenienza, checker reale, script riproducibile e workflow dedicato; raw e immagini esclusi dagli artifact CI. `--calibration-scale` è opt-in, default1, con affine riportata ai pixel originali tramite dimensioni effettive e centri pixel; fit ECC grandi serializzati per contenere la memoria. Regressioni sintetiche nuove: shift noto su larghezza dispari, affine nota16bit con scale/shear (errore<0,75px sui punti controllati), opzioni invalide, download corrotto/eccessivo/symlink/race/no-overwrite. Dieci gruppi headless locali superati; test aggiuntivo affine noto superato e Cppcheck passato. GPG locale non riesce ad avviare gpg-agent in questo runtime: non dichiarato passato localmente; il gruppo firma era già verde nella CI del commit precedente, verrà rieseguito nella nuova CI. Workflow nuovo ancora da osservare prima di dichiararlo remoto verificato.

### Esito verifiche sulla revisione de60721671061040fae6af46fb7fec4ac2133368

Tutte le esecuzioni remote concluse `success`:

- [RDAS reale/SatDump42514688645](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688645): download verificati, decodifica autentica, hash input invariati, calibrazione esplicita0,125, generazione nativa16bit, confronto pixel e caricamento headless di entrambi i prodotti SatDump. Report/log/profilo negli artifact; nessuna immagine grezza caricata.
- [Build/regressioni42514688658](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688658): Ubuntu22.04/24.04/26.04, sanitizers, Cppcheck e fuzz tutti verdi; compresa la regressione GPG che non era eseguibile nel runtime locale.
- [Piattaforme42514688690](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688690): ARM64 nativo e Fedora44 con installazioni in userspace pulito riusciti.
- [AppImage42514688605](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688605): build e avvio CLI/GUI in container pulito riusciti.
- [Attestazione42514688657](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688657): firma autentica Sigstore/OIDC e verifica con rifiuto di manomissione riuscite.
- [CodeQL42514688711](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688711): completato con successo.

Anche lo script completo locale, rieseguito senza modifiche durante l'esecuzione, è terminato0 con sei differenze pixel massime pari a zero e input invariati. Il report locale durevole è in [docs/REAL_RDAS_EVIDENCE.md](docs/REAL_RDAS_EVIDENCE.md). ASan/UBSan locali sui nuovi casi core/scala passati con detect_leaks=0; la CI sanitizers ha passato anche la propria verifica leak. Scala CLI non valida (nan,inf,0,1.1,testo,suffisso) e scala in generate respinte con messaggi italiani.

M3-01 passa da bloccato per assenza di dati a verificato in parte su acquisizione reale. Non sono chiusi geolocalizzazione/verità a terra/ROI di merge. Prossimo sviluppo reso possibile dal campione: M3-04, correzione cromatica Natural Color con fixture16bit e revisione della scena reale. GPU reale, verifica screen reader/VM grafica completa e aggiornamento/rollback firmato restano nei rispettivi task; nessuna capacità non provata è dichiarata completata.

## M3-04 Natural Color su scene reali — 2026-11-08

Ripartenza dalla revisione8f92e1c, conservando dati e modifiche precedenti. Implementata `composeNaturalColor`: RGB321 MSU-GS (CH3→R,CH2→G,CH1→B), correzione hue del codice GPL originale (+60°rosso,−6°giallo,40%corretto), floating point per strisce fino a128righe/1Mpx, output alla profondità originale8/16bit. La precedente `composeRgb` mantiene CH1→R; nuova pubblicazione tramite directory privata con cleanup RAII e rename esclusivo anche per composizione generica.

CLI `natural-color`/`compose-rgb` con opzioni esplicite, messaggi EN/IT e nessuna sovrascrittura; desktop con pulsante Esporta Natural Color e scorciatoia Ctrl+Shift+N, selezione prodotto VIS e nuovo PNG, worker asincrono. I controlli di elaborazione/coda/esportazione sono disabilitati insieme durante un job. Corretto anche il contrasto della tabella coda nel tema scuro, risultato illeggibile nella revisione visiva.

Verifiche locali: quindici gruppi CTest GUI/headless passati (esclusa firma GPG non avviabile nel runtime locale; resta coperta dalla CI); core8/16bit con oracolo scalare double indipendente da cvtColor, neutralità grigio10001 (bit bassi16bit preservati), tile boundaries, mapping, blend, dimensioni errate, input invariati/no-overwrite. Flusso CLI e worker desktop EN/IT producono output identici. Cppcheck e nuovi test core/CLI ASan+UBSan passati localmente (detect_leaks=0). Screenshot desktop IT/scuro a960×680 e1320×1000 ispezionati; pannello sinistro scorrevole alle dimensioni minime.

Su RDAS reale: due compositi6004×13594×3 uint16 esportati. Riferimento indipendente colorsys Python su12684/12679 campioni: errore massimo0,48livelli su65535;6/11campioni entro0,001°dai confini discontinui hue riportati separatamente. Immagine reale ispezionata: Terra/nubi/coste visibili, rampe e discontinuità originali mantenute. Nessuna normalizzazione automatica, gamma implicita, colore scientifico calibrato o correzione geometrica dichiarati. Report/digest locali conservati in docs/NATURAL_COLOR_EVIDENCE.md; checker e workflow RDAS aggiornati per rieseguire export/verifica su entrambi i VIS. Esiti CI di questa revisione ancora da osservare.

## Chiusura Natural Color e preparazione GPU/VM — 2026-11-08

Natural Color pubblicato su main nel commit a531cecdcacccd9b6164e8cfe344158bef95db58. Verifiche remote da leggere nei run42516055833(RDAS reale),42516055860(regressioni),42516055842(piattaforme),42516055905(AppImage),42516055848(attestazione) e42516055874(CodeQL).

Aggiunto `orbyx_gpu_verify`/CTest opencl-gpu: inventario reale, obbligo dispositivo GPU, kernel OpenCL esplicito4096float e confronto affine richiesto UMat8/16bit; uscita77 se manca hardware, errore1 su difetti del percorso disponibile. Locale:0piattaforme/0dispositivi/0GPU, uscita77 correttamente Skipped; undici gruppi headless applicabili passati, GPU non superata. Il report non certifica dispatch end-to-end né benchmark; OpenCV può fare fallback per singole operazioni. Chiarito anche il motore: ECC è CPU/Mat e ora il log non lo etichetta OpenCL. M3-03 rimane aperto per hardware reale/profilazione, senza simulazione di GPU (`docs/GPU_REAL.md`).

Preparata prova Fedora44 in VM completa con QEMU, kernel/disk propri, immagine ufficiale reale583729152byte verificata SHA25628680fe5b371a5a82ebf43a31926e086a168e59949d03969c5093e7071f90b7f. Cloud-init/SSH con chiave temporanea locale e nessun secret personale; installazione RPM, assenza toolchain, equivalenza CLI/GUI e Natural Color, rimozione e input preservati. Nessun disco o chiave privata negli artifact. Bash/YAML/pin verificati, boot e risultato completo ancora da osservare nella CI (`docs/FEDORA_VM.md`).

### Verifiche finali completate — 2026-11-08

Natural Color su acquisizione autentica e tutte le CI della prima implementazione a531cec concluse `success`: [RDAS42516055833](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055833), [regressioni42516055860](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055860), [piattaforme42516055842](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055842), [AppImage42516055905](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055905), [attestazione42516055848](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055848), [CodeQL42516055874](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055874). Pipeline reale nuovamente passata anche sul core2c669fd: [run42516672101](https://github.com/chiaraberti13/ORBYX/actions/runs/42516672101).

Corretti durante la prova due difetti del nuovo harness: OpenCV4.13/Fedora solleva eccezione su enumerazione OpenCL senza runtime, ora controllato prima; cloud-init25.3 rifiuta `datasource_list` nei user-data, campo rimosso e schema ufficiale dei campi utilizzati validato localmente. Nessun codice d'errore cloud-init ignorato.

**VM completa superata:** [run42517397293](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397293), revisione `9a52e07d1e941b155ce9194fe019fba4de1d4549`. Accelerazione KVM effettiva; kernel guest `6.19.10-300.fc44.x86_64`; cloud-init `status: done`, `extended_status: done`; escluso container. RPM installato su Fedora44 pulito senza toolchain/-devel, CLI/GUI identici sugli otto file e su Natural Color, disinstallazione e input preservati. Log seriale/guest e inventario negli artifact; chiavi private e dischi esclusi. M3-02 verificato per la matrice Linux dichiarata, senza estendere a display interattivo/FUSE/macOS.

Sulla stessa revisione9a52e07 tutti gli altri run `success`: [build/regressioni42517397183](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397183), [Fedora/ARM64 — run42517397266](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397266), [AppImage42517397247](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397247), [attestazione autentica42517397338](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397338), [CodeQL42517397274](https://github.com/chiaraberti13/ORBYX/actions/runs/42517397274). Il gate GPU è correttamente Skipped, non passato; le regressioni CPU applicabili sono verdi.

Tentata anche una risorsa standard pubblica alternativa, senza runner a pagamento: [inventario Apple OpenCL42517297975](https://github.com/chiaraberti13/ORBYX/actions/runs/42517297975), commitf2bf5b5, macos-15-intel. API native:0GPU OpenCL disponibili. Compilazione/esecuzione del verifier GPU conseguentemente saltata; il successo del job d'inventario non è un pass hardware né una verifica dell'app su macOS.

Stato delle cinque verifiche riepilogato in [docs/VERIFICATION_STATUS.md](docs/VERIFICATION_STATUS.md). Completati Natural Color visivo e matrice Linux inclusa VM reale; autenticazione artifact OIDC reale verificata. Restano non certificati GPU effettiva e geometria assoluta/verità a terra. Firma GPG personale, aggiornamento/rollback automatico e gli altri sviluppi pianificati della roadmap non vengono dichiarati completati da queste prove.

## Installazione guidata e log Ubuntu26.04 — 2026-11-08

Recepita segnalazione reale: contesa APT con unattended-upgrades, CMake inizialmente assente e tentativi di avviare binari mai compilati. Aggiunti install.sh/start.sh/orbyx.sh: rilevamento automatico delle cartelle, dipendenze complete, attesa lock APT senza rimuoverli/interrompere processi, configurazione/build/test/installazione per utente normale e verifica finale. Nessun upgrade generale/autoremove. Interruzione alla prima fase fallita e launcher che rifiuta un'installazione incompleta.

Cartella logs con log per sessione ed errors.log; include errori stdout/stderr e fase/codice, ignorata da git e senza upload automatico dei log utente. La CI carica solo i propri log del runner temporaneo. Lock locale condiviso installazione/avvio. README aggiornati con comandi senza percorsi da compilare manualmente; documentazione in docs/INSTALLATION.md.

Bash e sintassi Python verificati localmente. I test installer richiedono utente non-root: il runtime locale non permette un UID secondario e li registra come saltati, non passati. Workflow dedicato Ubuntu26.04 predisposto per eseguire sia regressioni con comandi simulati sia dipendenze/build/test/installazione reali e smoke dei launcher come utente normale. Risultato remoto ancora da osservare prima della dichiarazione di verifica.

### Installer Ubuntu26.04 verificato

[Run42519134799](https://github.com/chiaraberti13/ORBYX/actions/runs/42519134799), commit632b15ea1a3591aa1369eae34a4882d9377a1454: `success`. Quattro test di flusso con comandi simulati passati, compreso retry del lock, stop dopo build/test falliti, stdout negli errori e symlink log rifiutato. Corretto durante la CI il conteggio del test retry, che contava anche la riga del wrapper sudo oltre alle vere invocazioni APT: nessun rilassamento del controllo di contesa.

Poi lo stesso job ha installato realmente le dipendenze su Ubuntu26.04, configurato/compilato, passato CTest, installato per l'utente normale e verificato CLI/GUI tramite i launcher senza percorsi manuali. Conferma finale creata solo dopo gli smoke installati. I log del runner sono negli artifact; quelli delle installazioni utente restano locali. Un controllo locale separato ha confermato uscita23 e conservazione nel file errori di una failure emessa solo su stdout. Log e lock locali esclusi anche dagli archivi sorgenti CPack, non solo da git.

La macchina Ubuntu26.04 della segnalazione non è accessibile da questa sessione: la correzione è pubblicata e verificata nel repository, da usare con git pull e bash install.sh, poi bash start.sh. Non occorre inventare o inserire percorsi di dati per installare/avviare il desktop.

## Guida iniziale per principianti

README principale ora in italiano, con guida inglese separata e alias italiano conservato. Passaggi numerati per terminale, primo clone oppure aggiornamento, installazione, controllo del messaggio di successo, avvio, primi lavori GUI/L4, Natural Color ed errori con log. Rimossi dal percorso iniziale parametri numerici, comandi di compilazione e cartelle di dati dimostrative. I comandi di installazione/avvio sono quelli già verificati realmente su Ubuntu26.04. Questa modifica documentale viene pubblicata separatamente prima degli altri sviluppi.

## Onboarding: demo senza dati o percorsi manuali

Dopo la pubblicazione separata della guida f4dca8f, aggiunto demo.sh: verifica installazione, genera fixture dichiaratamente sintetiche, esegue calibrazione/allineamento reali del motore e composizione Natural Color, apre il desktop con input/profilo/destinazione predisposti quando esiste una sessione grafica. Nuovi risultati per esecuzione, nessuna sovrascrittura; cartella demo-output e log esclusi da git/archivi sorgenti. Non è una nuova acquisizione reale né una validazione geometrica. CI Ubuntu26.04 estesa alla demo installata; sintassi e collegamenti verificati, risultato remoto da osservare. Hint L4 reso coerente con un profilo personalizzato già calibrato. La finestra demo mostra esplicitamente «Demo sintetica — nessuna acquisizione satellitare reale»; build GUI e smoke offscreen locali passati.

## Fuzzing profili JSON e generazione

Esteso l'harness libFuzzer al parser del profilo e al vero percorso run/generate CPU con fixture sintetiche piccole. Corpus versionato (PNG valido, profili separati/uniti e invalidi) e dizionario, copie/artefatti nella build, conservazione CI delle mutazioni/crash. Corretto anche il file temporaneo prevedibile del vecchio harness: directory privata mkdtemp e cleanup RAII. Nessun cambiamento all'algoritmo del prodotto.

Compilazione e replay locale di tutti i semi passati su OpenCV4.6; campagna Clang/libFuzzer/ASan/UBSan estesa in CI da osservare. Codecs/JsonCpp distro non strumentati, stima ECC non fuzzata da questo harness; nessuna dichiarazione di assenza vulnerabilità. Pubblicazione separata dopo guida e demo, come richiesto.

## Inventario runtime: dipendenze transitive

Corretto l’inventario SBOM: prima enumerava soltanto DT_NEEDED diretti, ora include tutta la chiusura ldd e il loader, con campo direct. Export CycloneDX/SPDX ampliati di conseguenza, fallback usr-merge /lib-/usr/lib e cache delle query. Diciotto test, inclusa integrazione readelf/ldd su due binari reali, passati. I plugin dlopen successivi restano fuori dal perimetro statico; componenti irrisolti mantenuti come tali. Pubblicazione separata prima della correlazione avvisi.

## Correzione dai log utente: locale italiano e inventario ELF

I log ricevuti mostrano dipendenze installate e compilazione riuscita, poi un unico test fallito: lettura dei DT_NEEDED nel controllo SBOM. Il parser assumeva l'etichetta inglese Shared library e le sonde ereditavano la lingua del desktop. La correzione legge il tag ELF stabile NEEDED senza dipendere dall'etichetta tradotta e forza locale C soltanto per le sonde esterne, senza cambiare la lingua del programma/utente. Il test non viene disabilitato.

Aggiunte regressioni per etichette italiana/francese/tedesca/inglese e per ambiente delle sonde. Venti test SBOM, inclusi binari reali e chiusura transitive, passati localmente. Workflow installer Ubuntu26.04 ora genera e usa realmente il locale it_IT.UTF-8 durante installazione, test e avvio/demo. Verifica remota da osservare; pubblicazione immediata su main della correzione. I log utente originali non vengono copiati nel repository.

### Conferma sul locale italiano e sugli incrementi pubblicati

La correzione3fb4d321125aec4461002013e4bda94b85b47a7f ha superato la [procedura reale Ubuntu26.04 in italiano42521133995](https://github.com/chiaraberti13/ORBYX/actions/runs/42521133995): generazione locale it_IT.UTF-8, installazione dipendenze, build/CTest/installazione utente, launcher CLI/GUI e demo senza apertura interattiva. Il test SBOM segnalato dall'utente è eseguito e passa, non viene escluso. [Regressioni42521133979](https://github.com/chiaraberti13/ORBYX/actions/runs/42521133979): tutti i job Ubuntu22.04/24.04/26.04, sanitizers, fuzz e Cppcheck success. Anche AppImage42521133959, Fedora/ARM6442521133973, VM42521133968, CodeQL42521133957 e attestazione42521134041 success.

Incrementi già pubblicati separatamente: f4dca8f guida principiante,03efc6d demo guidata,bc0f536 fuzz profili/generazione,26d6b74 chiusura ELF transitiva,3fb4d32 correzione dal log utente. La [campagna fuzz42520331102](https://github.com/chiaraberti13/ORBYX/actions/runs/42520331102) ha completato104219esecuzioni in61secondi, copertura1563 e69elementi di corpus, senza crash/UB/ASan rilevati nella campagna; leak detection disattivata per questo job e librerie distro non strumentate. Nessuna garanzia di assenza vulnerabilità. Report/corpus riproducibili negli artifact.

Il README principale è ora una guida italiana con comandi progressivi, controllo del messaggio di successo, uso GUI/L4/Natural Color, errori e log; inglese in README.en.md. Il lavoro non viene tenuto in un unico commit finale: ogni incremento è già su main. Rimangono i task esplicitamente aperti nella roadmap, inclusi hardware GPU/geometria di riferimento e altre funzionalità non dichiarate completate.

## Verifica indipendente M3-01 su dati reali — 2026-11-08

Riproduzione end-to-end della pipeline su dati reali in un container separato, per confermare lo stato dichiarato. Il CADU reale Elektro-L4 (`elektro_rdas.cadu`, 769.938.432 byte, sha256 `b92ded3c71f1752d12208ed358e0d392e05d2ddbefe97f2e72b69e3c09498d48`) è stato recuperato dall'issue pubblica, caricato come asset della release `rdas-cadu-l4`, scaricato nel container via API REST GitHub e decodificato con SatDump (`elektro_rdas`, build locale) producendo prodotti autentici MSU-GS: `MSUGS_VIS1`/`MSUGS_VIS2`, tre canali 6004×17200 16-bit + `product.cbor` reali.

ORBYX a piena risoluzione senza scala va in OOM (SIGKILL) — limite reale confermato; la crash-safety ha tenuto (nessun output parziale, rollback pulito, input intatti). Con `calibrate --calibration-scale 0.25` (fix esistente) niente OOM: calibrazione L4 reale con coefficienti sensati (linee ≈ identità; ty ≈ offset 3606/1804 con residui; tx ±10–26 px = disallineamento reale). `generate` ha prodotto prodotti allineati separati VIS1/VIS2 a 6004×13594 16-bit (crop −3606 corretto), CBOR preservati. Conferma indipendente dello stato «verificato in parte su dati reali» di M3-01. Restano aperti, come dichiarato, precisione rispetto a verità a terra, geolocalizzazione assoluta e ROI di merge (il campione non porta proiezione/calibrazione utilizzabili).

## 2026-11-09 — Demo reale nell’app e interazioni coda

Aggiunto il pulsante desktop **Avvia demo immagini** (EN/IT, Ctrl+Maiusc+D).
Elabora realmente i sei JPEG forniti: registrazione di fase/ECC, Natural Color,
correzione verticale delle metà adiacenti e immagine unica 1428×1428 mostrata
nell’anteprima. Risultati intermedi, livelli senza contrasto, report e log
sono conservati; originali integri, provenienza e SHA256 in assets/demo-jpeg.
Non è una validazione RDAS/geografica assoluta e la giunzione resta visibile.

`gui-widget-flow` verifica i widget di produzione con click EN/IT, aggiunta e
rimozione, ripresa da tastiera senza modificare il prodotto, vero errore exec e
pulsante demo con immagine effettivamente visualizzata (5,46 s, esito positivo).
Corretti manifest atomico, recupero FailedToStart e rifiuto di stati non
terminali/malformati. Un errore di avvio non riutilizza più uno stato completato
vecchio. Restano da verificare dialoghi profili e screen reader fisico.

### Secondo incremento — dialoghi, viewport e runtime installati

`gui-widget-flow` copre ora anche i tre dialoghi di duplicazione profilo EN/IT
(compresa selezione del nuovo file e digest dell’originale), rifiuto di stato
`running` anche con backend exit 0, demo da pulsante con immagine mostrata,
verifica SHA256 dei sei JPEG dopo l’uso, input mancanti/non registrabili,
rollback e rifiuto di destinazioni esistenti. I click richiedono che il centro
del pulsante sia nel viewport: provati 1280×860 e 960×680 con scorrimento.

La prima CI ha confermato Ubuntu22/24/26, ARM64, AppImage, acquisizione reale
SatDump, sanitizzatori, fuzzing, firma GPG effimera e provenienza OIDC.
Ha rilevato un blocco Fedora nel test widget: la build fuori dai sorgenti non
aveva il runner batch. Corretto copiando i supporti nella build, senza percorsi
personali. Il test è poi passato anche con eseguibili/supporti trasferiti fuori
dai sorgenti e avviati dalla radice. Reso esplicito il controllo bounds del
marker JPEG segnalato da cppcheck, senza sopprimere il controllo.

La demo del programma **installato** è passata localmente. Il checker
`scripts/ci/check_image_demo.py` verifica PNG RGB1428×1428, sei intermedi,
ECC >0,94 e giunzione misurata; aggiunto alle installazioni Ubuntu26, Fedora,
ARM64, AppImage e VM Fedora. Screenshot automatici della demo attendono la
fine del worker. Nuova CI richiesta per confermare il secondo incremento.
Screen reader reale, GPU fisica e geometria assoluta restano aperti.

### Decoder JPEG nel test headless e VM Fedora

Aggiunto `image-demo-regression` al CTest comune, quindi anche ad ASan/UBSan:
composizione reale dai sei JPEG, canale di riferimento invariato pixel per pixel,
originali conservati, correlazione ECC e scarto di giunzione verificati;
troncamenti, marker incompleti, limite1MiB, symlink e canale mancante rifiutati
con rollback; destinazione esistente conservata. Passato localmente in3,70s.

Su8103ae6 la matrice nativa Fedora/ARM64 è verde, inclusa la demo installata.
La VM completa ha passato la compilazione e19 gruppi CTest (GPU saltata),
poi cloud-init ha segnalato un errore recuperabile nel tentativo di cambiare
hostname. La prova non necessita di un hostname specifico: rimossa tale
richiesta e impostato `preserve_hostname: true`; il controllo stretto dell’esito
cloud-init resta attivo. In attesa di nuova prova completa della VM.

### Conferma remota finale — b86103f

Tutte le sei workflow applicabili all’ultimo incremento sono verdi: build e
regressioni42577236764 (tre Ubuntu, sanitizers con leak detection attiva,
Cppcheck, fuzz), CodeQL42577236733, piattaforme native42577236864,
AppImage42577236837, VM Fedora completa42577236705 e provenienza42577236585.
VM: cloud-init `done` senza degradazione, kernel6.19.10-300.fc44.x86_64,
demo installata validata, CLI/GUI equivalenti e disinstallazione riuscita.
Nei build GUI20gruppi CTest:19eseguiti, GPU saltata. Il job ASan/UBSan/LSan
ha passato15gruppi (GPU saltata), incluso image-demo-regression in4,63s.

Verificate separatamente installazione guidata Ubuntu26 e pipeline RDAS/SatDump
reale sul precedente8103ae6; non toccate dall’ultimo incremento. Riconoscimento
strutturale provato anche sulla preview SatDump reale, con tutti i9file invariati.
Limiti residui e funzionalità ancora aperte distinti in `docs/TEST_RESULTS.md`.

## Sviluppo residuo — calibrazione guidata

Aggiunta procedura desktop a tre passi EN/IT: acquisizione/profilo/destinazione,
satellite e scala ECC, revisione esplicita. Nessuna elaborazione parte prima del
comando dell’utente. Controlli su file mancanti, cartelle simboliche e destinazioni
esistenti/interne all’origine. Al termine della calibrazione il profilo prodotto
viene selezionato e l’app propone l’allineamento in una nuova destinazione.
Scala ECC conservata anche nei lavori batch, opzionale e compatibile con manifest
precedenti. Test dei passi e del rifiuto destinazione esistente EN/IT nel widget
reale; regressioni del prodotto e dei widget superate. La geometria assoluta non
è dedotta dal wizard e le ROI restano soggette alla verifica dell’operatore.

### Monitoraggio e importazione MSU-GS

Aggiunto Monitora acquisizioni EN/IT, non modale: processo di ispezione separato,
intervallo comune di stabilizzazione, metadati CBOR limitati e non ambigui,
header PNG con CRC/IEND e geometria nativa, difesa da symlink e scritture della
stessa dimensione. Le preview non sono spacciate per prodotti elaborabili.
Importazione manuale senza avvio; opt-in di accodamento con satellite compatibile,
deduplica e fingerprint riconfermato nel batch.10test Python, incluso il rifiuto batch dei dati cambiati dopo importazione, e integrazione Qt
con processo reale di ispezione, scelta manuale e auto-accodamento passati;
regressioni batch conservate. Nessun metadato geografico mancante è inventato.

### Correlazione runtime con Ubuntu OSV

Implementata scansione delle versioni sorgenti effettive ricavate da dpkg e
coerenti con la SBOM. API OSV ufficiale, paginazione completa, snapshot dei
risultati con digest e replay offline. Mancata risoluzione/errore di rete non
produce un falso esito pulito.5regressioni, inclusa fixture ottenuta realmente
dall’API: UBUNTU-CVE-2025-6491 presente per php8.3 vulnerabile e assente per
la versione corretta. `bash security.sh` usa automaticamente i binari installati,
con report sotto logs; workflow periodica per il runtime Ubuntu reale.
Fonte/schema: https://documentation.ubuntu.com/security/security-updates/osv/
e https://google.github.io/osv.dev/post-v1-querybatch/ . Nessun percorso locale
viene inviato: solo nomi/versioni dei pacchetti sorgente.

### Risoluzione delle diversion dpkg

La prima scansione reale ha risolto106pacchetti sorgente e trovato305segnalazioni,
ma ha correttamente segnalato copertura incompleta: gli avvisi dpkg-divert per
loader e libtirpc venivano letti come nomi di pacchetto. Ispezionato l’artifact
reale, corretto il parser perché selezioni soltanto una riga di proprietà valida
(non deduca proprietà dalle diversion), aggiunte due regressioni con l’output
osservato. Le segnalazioni di vulnerabilità restano nel report, non ignorate.

### Aggiornamento nativo e rollback autenticati

Implementato motore Python senza shell/sudo/fallback non firmati, GUI EN/IT e
launcher update.sh. Verifica gh attestation con identità ORBYX/workflow/main,
commit completo approvato e runner ospitato; snapshot prima della verifica,
archivi a root unica limitati senza traversal/link/device, manifest piattaforma
firmato nell’archivio, health check CLI/GUI prima dell’attivazione atomica.
Release precedenti conservate; rollback ricostruisce dall’archivio riverificato,
non si fida dei file installati o di hash locali modificabili. Migrazione atomica
dell’installazione sorgente con backup; nessun rollback firmato verso il legacy.
Install.sh rifiuta di modificare direttamente una release gestita verificata.
9regressioni locali con verifica crittografica iniettata (dichiarata), inclusi
rollback con contenuti diversi, cache manomessa, mismatch piattaforma, percorso
ostile, link, pin errato e concorrenza. Prova autentica install/rollback/tamper
aggiunta alla workflow OIDC con build native separate Ubuntu22/24/26.

### Persistenza effettiva della coda desktop

Aggiunte/rimozioni/stati salvati subito con QSaveFile, lock di processo e confronto
della revisione: un’altra istanza non può sovrascrivere silenziosamente lavori
salvati nel frattempo. Ripristino alla riapertura con input solo dei metadati
locali della coda, non delle acquisizioni. Stati conclusi mostrati Da verificare
finché il batch non riconferma i digest; lavori interrotti non sono Completati.
ID UUID separati dalla numerazione visibile, migrazione del precedente manifest,
parametri rivedibili con doppio click e profili personalizzati selezionabili.
Test reali di riapertura e conflitto fra due finestre aggiunti ai widget EN/IT.
Corretto il nome del pulsante Chiudi del nuovo dialogo che nascondeva QWidget::close.
La stabilità CBOR ora comprende il digest dei metadati, anche se il filesystem
non aggiorna i timestamp per una riscrittura nella stessa frazione di secondo.

### Rifinitura e protezione delle installazioni gestite

Rifiutate directory update scrivibili da altri utenti; aggiunta regressione
specifica (10test updater). Il launcher preferisce il motore installato, e
l’installer riconosce una release gestita prima di installare dipendenze.
Test widget aggiunti per migrazione del vecchio manifest senza scritture
implicite e rifiuto di uno stato corrotto senza sovrascriverlo. README aggiornati
con i nuovi pulsanti e comandi per principianti.

### Ultimi controlli dei flussi per principianti

Il monitor mostra anche le acquisizioni incomplete, trattenendole dalla coda.
Il wizard controlla struttura PNG nativa, dimensioni coerenti e oggetto JSON
prima del passaggio successivo: le anteprime ridotte non sono accettate come RDAS.
La verifica completa del decoder e dei trasformati resta nel core prima degli
output. L’interfaccia continua a richiedere revisione del satellite e delle ROI.

Installazione sorgente e updater condividono inoltre lo stesso lock di modifica:
una migrazione firmata concorrente non può trasformare il prefisso in un symlink
fra il controllo iniziale di install.sh e la copia dei file. Nessuna release
gestita viene modificata in-place dall’installer sorgente.

### Chiusura verificata dello sviluppo software — 9 novembre 2026

Codice `4ea5019025d8707b1d2e629729caf020ac08712a`: tutte le sette workflow applicabili concluse success (Build/regression42589266485, installer42589266383, OIDC42589266318, piattaforme42589266400, VM42589266441, AppImage42589266435, CodeQL42589266430). Build GUI23gruppi:22pass e GPU skipped; sanitizers18gruppi:17pass e GPU skipped, leak detection attiva. Scansione OSV e replay reali verdi su e922cde (42584973253); i findings restano visibili.

Completati i task software sopra descritti; la precedente frase che li indicava ancora da implementare in TEST_RESULTS è stata sostituita con risultati e ambito effettivi. Prove esterne residue: GPU fisica, geografia assoluta certificata, tecnologie assistive reali e chiave GPG personale. README L4 semplificato usando il wizard. Dettagli e collegamenti in docs/TEST_RESULTS.md.

### M2-05 — Nuova esigenza: unione manuale multi-porzione e colore

Il requisito aggiunto dopo la precedente chiusura richiede un editor nell’app,
non soltanto due prodotti VIS allineati. Nessun vincolo SX/DX o di due parti:
introdotto motore a massimo32livelli, pubblicato nel primo incremento `ea30ebe`.
PNG grigio/RGB8/16bit, trasformazioni di scena libere, crop, ordine, opacità,
sfumatura bordi, livelli/gamma/EV/saturazione e guadagni RGB; renderer a tile,
budget buffer e pubblicazione PNG atomica esclusiva. La lettura RGB è opt-in:
il percorso RDAS continua a richiedere canali grayscale, con le stesse protezioni.

Secondo incremento: finestra EN/IT **Componi porzioni e colore…**, drag del
livello selezionato e contorno, anteprima ridotta con zoom/Adatta, annulla/ripeti,
progetti JSON riapribili e avviso modifiche non salvate, colore da tre canali
allineati tramite il core esistente, demo reale aperta come porzioni modificabili,
log locale apribile dall’app. Nessuna dipendenza da GIMP o nuovo decoder grezzo.
Originali intatti; export8/16bit a risoluzione di scena, preview8bit dichiarata.

Verifica locale: prima suite completa applicabile24gruppi senza errori, GPU
Skipped e GPG escluso per il limite dell’agente locale già documentato. Dopo le
rifiniture, mosaic-regression, mosaic-widget-flow e gui-widget-flow nuovamente
passati; prima anche engine, color-cli e image-demo passati. Test N=3 su righe e
colonne, origine negativa, geometria, colori e bit bassi16bit, gestione invalidi,
file esistente immutato, drag/annulla/riapertura/PNG confrontato col renderer e
Natural Color effettivo EN/IT. Screenshot della demo modificabile ispezionato.
Cppcheck completo passato. Pipeline remote avviate; esiti conclusivi da registrare.
La geografia certificata, la GPU fisica e lo screen reader reale non sono implicati.

### Compatibilità Qt recente e rendering delle aree occupate

La CI del secondo incremento4f307e8 ha rilevato un errore di compilazione del
nuovo test su Qt recente (Ubuntu26/Fedora): findChild su una sottoclasse senza
Q_OBJECT è ora rifiutato. Il test cerca il QLabel con nome esatto e verifica
poi il tipo concreto tramite dynamic_cast; nessuna prova disattivata o classe
fittizia sostituita. Locale Qt6.4 non imponeva questo controllo.

Rendering limitato ai tile del rettangolo trasformato di ciascun livello,
anziché attraversare l’intero canvas per ogni porzione; guadagni/esponente
precalcolati e percorso neutro senza pow per pixel. Mosaic core e widget
nuovamente passati con geometria/colore/export invariati negli oracoli previsti.

### Chiusura verificata M2-05

Codice conclusivo `fa06d806726349d805d570011916f2be9324a94a`: tutte le sette
workflow concluse success. Build/regressioni42615323472 (Ubuntu22/24/26,
Cppcheck, fuzz, ASan/UBSan/leak detection), Fedora/ARM6442615323543,
VM42615323571, AppImage42615323468, OIDC42615323406, CodeQL42615323424 e
RDAS/SatDump reale42615323626. Desktop25gruppi,24eseguiti e GPU skipped;
sanitizers19gruppi,18eseguiti e GPU skipped. Compatibilità del nuovo test con
Qt recente confermata su Ubuntu26 e Fedora senza escludere controlli.
Guida e report aggiornati; task M2-05 chiuso nel suo ambito esplicito. Restano
separate le prove esterne già dichiarate e l’eventuale futura integrazione dei
grezzi richiesta soltanto come domanda conoscitiva.

### M2-06 — Sovrapposizione automatica modificabile

Esteso il compositore senza introdurre ipotesi SX/DX. La porzione selezionata
resta il riferimento; ORBYX estrae dettagli visuali in un’immagine di lavoro
ridotta, confronta le porzioni in entrambe le direzioni e accetta soltanto una
trasformazione robusta con soglie esplicite su inlier, errore, area comune e
confidenza. Una catena di corrispondenze consente di collocare porzioni che non
toccano direttamente il riferimento. Livelli uniformi o senza corrispondenza
restano invariati.

L’interfaccia EN/IT mostra le evidenze prima dell’applicazione e propone
**Applica/Annulla** con annullamento predefinito. La proposta applicata resta un
normale stato dell’editor: annulla/ripeti, trascinamento e valori numerici restano
disponibili. Il messaggio persistente richiede di verificare ogni giunzione prima
dell’export. Test core locali passati su tre ritagli concatenati, rotazione/scala,
assenza di sovrapposizione e porzione uniforme. L’automatismo è visuale, non
certifica la geografia e non aggiunge un decoder dei grezzi SatDump.

### Chiusura verificata M2-06

Motore pubblicato in `7d87b42936cdbe27959606c14fe9e39ed085aa1d` e interfaccia,
test e guida in `338675dc356fb260ae60e5076f42b8280ff2fd58`. Suite locale:
24/24gruppi applicabili senza errori, GPU fisica skipped e firma GPG esclusa per
il limite dell’agente locale; Cppcheck completo senza segnalazioni. Il test GUI
EN/IT conferma evidenze nel dialogo, annullamento immutabile, applicazione,
annulla/ripeti e persistenza del progetto.

Tutte le pipeline applicabili concluse success: Build/regressioni42620629575,
Fedora/ARM6442620629613, VM42620629591, AppImage42620629586,
OIDC42620629514, CodeQL42620629494. Il motore identico del primo incremento ha
superato anche RDAS/SatDump autentico42620271984. Desktop25gruppi,24eseguiti e
GPU skipped; sanitizers19gruppi,18eseguiti e GPU skipped. M2-06 chiuso nel suo
ambito esplicito; restano esterne georeferenziazione certificata, giudizio visivo
universale e GPU fisica.

### M2-07 — Primo incremento: ROI storica modificabile

Il percorso grafico **Genera** conserva ora i due prodotti VIS allineati anche
quando il profilo contiene entrambe le ROI e il motore crea regolarmente il
prodotto storico unito. Il pulsante **Componi porzioni e colore…** riconosce
questi prodotti, genera le due composizioni colore e le apre come livelli con i
ritagli e la disposizione ricavati dallo stesso profilo. L’utente parte quindi
dal risultato ROI già noto ma può modificare giunzione, geometria e colore senza
uscire da ORBYX. Se le ROI non sono presenti, sono disponibili le porzioni
intere per l’allineamento visuale o manuale.

La compatibilità è esplicita: l’opzione che conserva i livelli è attivata dalla
GUI, mentre la CLI e il motore predefinito mantengono lo stesso insieme di file.
Il test core confronta pixel per pixel il prodotto ROI con e senza livelli
modificabili; il risultato è identico. Verifica anche ROI esatte, fallback a
immagine intera e rifiuto delle ROI non valide. I test del compositore e della
finestra principale coprono caricamento asincrono, geometria iniziale e flusso
completo EN/IT.

Verifica locale del primo incremento: suite applicabile24/24 passata, zero
errori; GPU OpenCL fisica skipped e firma GPG esclusa localmente per i limiti
già dichiarati. Cppcheck sui file modificati senza segnalazioni. Pubblicazione
codice: `f554703c76a7e354a244358ef66ec2333dc12120`. Restano in M2-07 la scelta
comparativa esplicita ROI/visuale e il coordinatore completo; M2-08 collegherà
anche coda, importazione e ripresa in un’unica procedura guidata.

### M2-08 — Primo incremento: procedura SatDump-to-finale

Aggiunta alla finestra principale l’azione **Crea immagine finale…**. Un wizard
EN/IT accompagna l’utente attraverso cinque pagine — Importa, Allinea canali,
Unisci porzioni, Colore, Esporta — mantenendo le scelte tecniche essenziali ma
spiegandone l’effetto. La pagina di unione rileva se il profilo contiene una
coppia ROI completa e chiarisce se l’editor partirà dai ritagli storici o dalle
porzioni intere. Il colore iniziale può essere Natural Color RGB321 o RGB123.

La procedura rifiuta cartelle SatDump incomplete, symlink nelle posizioni
critiche, profili non JSON o oltre1MiB, destinazioni esistenti, interne
all’origine o prive di un genitore valido. Richiede una conferma finale e poi
usa il normale motore ORBYX: non esiste un secondo percorso semplificato. Al
termine apre automaticamente il compositore sulle porzioni generate, lasciando
all’utente il controllo delle giunzioni e l’esportazione finale.

Verifica locale: compilazione GUI pulita, Cppcheck senza segnalazioni e test
widget end-to-end EN/IT passato. Il test percorre tutte le cinque pagine,
controlla i rifiuti, esegue realmente l’allineamento dal pulsante unico, verifica
il prodotto ROI storico e l’apertura automatica dei due livelli modificabili.
Pubblicazione codice: `69bcefd75f5af004f0914f8bd572fdbf8cad3745`.
Restano aperti in M2-08 coda/ripresa integrata, raggruppamento generico e stato
durevole dei passaggi.

### M2-07 — Secondo incremento: ritorno esplicito alla ROI

Il compositore conserva internamente la geometria ROI iniziale quando viene
aperto dal flusso ORBYX. Il nuovo comando **Ripristina ROI del profilo** permette
di recuperarla dopo spostamenti, ritagli o una proposta di sovrapposizione
visuale, senza ricalcolare i canali e senza sostituire il prodotto storico.
L’operazione entra nella cronologia annulla/ripeti. Una proprietà esplicita
indica la strategia corrente: `profile-roi`, `visual-auto` o `manual`.

Test widget EN/IT: caricamento delle ROI esatte, modifica della seconda
porzione, ripristino della posizione e dei ritagli originari, strategia riportata
a `profile-roi`, salvataggio del progetto. Mosaic e GUI end-to-end passati;
Cppcheck pulito. Pubblicazione codice:
`83f75d7f07a5d6f2a59cb5fbfa21cb907741d74d`.

### M2-09 — Primo incremento: regolazione fine della giunzione

Il canvas del compositore è ora focalizzabile e accessibile da tastiera. Le
frecce spostano il livello selezionato di1pixel; `Shift`+freccia applica un
passo di10pixel. Il trascinamento e i campi X/Y restano invariati. Ogni pressione
crea uno stato annullabile/ripetibile e marca esplicitamente la strategia come
manuale, così l’utente può rifinire una costa doppia o una giunzione senza
dipendere dalla precisione del mouse.

Test widget EN/IT su passo fine e grosso, assi X/Y, annulla, ripeti e strategia;
suite specifica del compositore e test GUI completo passati. Le soglie di attesa
del test headless per dialoghi e demo sono state ampliate perché due esecuzioni
sotto carico avevano raggiunto punti diversi senza un difetto funzionale;
nessuna verifica è stata rimossa. Cppcheck pulito. Pubblicazione codice:
`955dde7b5848849aa0925d9babe4a14fe6d5299e`.

### M2-09 — Secondo incremento: lampeggio della giunzione

Aggiunta l’ispezione **Lampeggia livello selezionato (solo anteprima)**. A
intervalli regolari il renderer di anteprima mostra e nasconde la porzione
selezionata, inclusa la sua cornice, per rendere evidenti dettagli sdoppiati o
una posizione errata. La funzione usa una copia temporanea dello stato: non
cambia visibilità, geometria, progetto o PNG esportato.

Test widget EN/IT: alternanza effettiva, arresto e ripristino dell’anteprima,
confronto serializzato esatto dello stato prima/dopo. Suite compositore passata
e Cppcheck pulito. Pubblicazione codice:
`b81580ced6c9d79836faf575ba0130217ca177c6`.

### M2-09 — Terzo incremento: mappa differenza della sovrapposizione

Il menu di ispezione offre ora **Differenza: selezionato e adiacente**. ORBYX
renderizza separatamente la coppia nello stesso sistema di coordinate, calcola
la differenza assoluta, limita il risultato all’intersezione reale dei due
poligoni trasformati e la rappresenta con una scala in falsi colori. Le aree
non sovrapposte restano nere e non vengono scambiate per errori di giunzione.

Il confronto è generato da copie temporanee dei livelli: visibilità, opacità,
geometria, cronologia, progetto ed export non cambiano. Test EN/IT con coppia
sovrapposta e differenza nota, conteggio di pixel diagnostici non nulli,
selezione automatica del livello adiacente e stato serializzato identico prima e
dopo. Compilazione e suite compositore passate. Cppcheck locale non avviabile
per configurazione mancante del pacchetto isolato; la pipeline pulita è il gate
definitivo. Pubblicazione codice:
`4063c60d4b670dc2f617ff0a37b5e177f4dd721a`.

### M2-09 — Quarto incremento: confronto a scacchiera

La modalità **Scacchiera: selezionato e adiacente** riusa la coppia scelta dal
controllo differenza e alterna tasselli da32pixel delle due anteprime nello
stesso sistema di coordinate. Nell’area comune permette di seguire la continuità
dei dettagli; fuori dalla sovrapposizione mantiene visibili le rispettive
estensioni. Non modifica livelli, cronologia, progetto o export.

Test widget EN/IT: presenza delle tre modalità, coppia adiacente corretta,
rendering completato e stato serializzato identico al ritorno all’anteprima
composita. Compilazione e suite compositore passate. Pubblicazione codice:
`dcf1f98163f92835ab91d8a4e7eee50dcdfb237e`.

### M2-09 — Quinto incremento: tendina prima/dopo

La modalità **Tendina: selezionato e adiacente** mostra la stessa coppia con un
separatore verticale regolabile da0 a100%. A sinistra del separatore usa il
livello selezionato, a destra quello adiacente; una linea colorata rende evidente
la posizione di confronto. Il cursore è disabilitato nelle altre modalità per
evitare controlli senza effetto.

Test widget EN/IT: quarta modalità presente, abilitazione contestuale del
cursore, valore35% realmente renderizzato, coppia corretta e stato invariato al
ritorno alla composizione. Compilazione e suite compositore passate.
Pubblicazione codice: `03a93cef65c3a8aeb84a58574ee58c72ab81ce23`.

### M2-09 — Sesto incremento: zoom dedicato alla seam

Il comando **Zoom sulla sovrapposizione selezionata** calcola l’intersezione dei
contorni trasformati del livello selezionato e di quello adiacente, adatta lo
zoom entro25–400% e centra l’area nel riquadro scorrevole. Se l’intersezione è
vuota mostra un messaggio e non inventa una seam.

Test widget EN/IT su coppia realmente sovrapposta, centratura attivata e stato
del progetto identico prima/dopo. La prima compilazione ha evidenziato l’header
Qt incompleto di `QScrollBar`; è stato incluso esplicitamente e la successiva
build più suite compositore sono passate. Pubblicazione codice:
`3f9790fd3a32d07e6dc52e694d10fcdc90117192`.

### M2-09 — Settimo incremento: solver robusto dei punti di controllo

Il core espone coppie `mobile → riferimento` nelle coordinate dei rispettivi
ritagli. Da almeno tre coppie finite e non degeneri stima una trasformazione di
similarità con RANSAC, compone il risultato nel sistema di scena del livello di
riferimento e restituisce indici inlier, residuo di ogni coppia, errore mediano e
massimo degli inlier. Coordinate, numero di coppie e scala sono limitati.

Il test usa otto corrispondenze esatte e un outlier grossolano: recupera
traslazione, rotazione e scala note entro tolleranze numeriche, rifiuta l’outlier
e verifica residui, input insufficiente e geometria degenerata. Suite core
passata. Pubblicazione: `a491d8fadc44847d5e12c49925d44e190318e061`.

### M2-09 — Ottavo incremento: editor dei punti di controllo

La finestra EN/IT **Allinea con punti di controllo…** permette di aggiungere,
rimuovere e modificare coppie X/Y. Il calcolo mostra il residuo per riga,
evidenzia gli outlier e riporta inlier/totale, errore mediano e massimo prima di
abilitare l’applicazione. Il livello selezionato viene trasformato rispetto
all’adiacente; l’operazione è registrata come `control-points`, loggata e
annullabile/ripetibile.

Test widget con cinque punti coerenti e un outlier: evidenza `5/6`, applicazione,
strategia, diagnostica, annulla e ripeti. Suite compositore e GUI globale EN/IT
passate. Pubblicazione: `063f7dcf5139a65c10fc5b06a56989fb398763eb`.

### M2-09 — Nono incremento: proposta reversibile dei bordi uniformi

Il core rileva righe e colonne esterne a basso dettaglio e restituisce soltanto
una proposta di ritaglio in coordinate assolute: non modifica livelli, sorgenti
o pixel. Immagini uniformi/ambigue e proposte troppo piccole vengono rifiutate;
un ritaglio già presente delimita l’analisi. Test core con contenuto sintetico a
limiti noti, area conservata, ritaglio annidato e rifiuto uniforme passato.
Pubblicazione core: `739d4a81d42899713dc1920f0ba5fb96216effd8`.

Nell’editor EN/IT **Proponi ritaglio bordi uniformi…** esegue l’analisi in
background e mostra rettangolo attuale/proposto e percentuale conservata.
Annulla non cambia il livello; Applica conserva il file sorgente e compensa la
posizione tenendo conto di scala e rotazione, così il contenuto rimasto non si
sposta nella scena. L’operazione è loggata e annullabile/ripetibile. Test widget
EN/IT su annullamento, evidenze, applicazione, compensazione e annulla passato.
Pubblicazione editor: `29023a398fd5752d89d7b7152dbc475a70883ee1`.

### M2-09 — Decimo incremento: copertura spaziale dei punti

Il solver calcola sugli inlier l’estensione normalizzata nelle immagini mobile
e di riferimento. Il riepilogo mostra le due percentuali e segnala quando il
riquadro coperto è sotto5% o uno degli assi sotto15%. È un avviso, non un
rifiuto: residui e trasformazione restano disponibili, ma l’utente sa che una
rotazione/scala ricavata da un solo angolo è fragile.

Test core con punti ampiamente distribuiti e gruppo concentrato; test widget
EN/IT sul testo e sulla proprietà diagnostica. Entrambe le suite specifiche
sono passate localmente. Pubblicazione codice:
`75c47d9f0e0df9c5066294f88caac560b60a9e6c`.

### M2-10 — Primo incremento: modalità di blending esplicite

Ogni livello distingue ora **Nessun blending (il livello superiore
sostituisce)** e **Sfumatura (transizione visuale)**. Nel primo modo opacità e
feather non partecipano al calcolo e i controlli corrispondenti vengono
disabilitati; nel secondo resta identico il comportamento storico. Il modo è
salvato nel JSON e il parser assegna Feather ai progetti precedenti che non
possiedono il campo, senza migrazioni distruttive.

Test core: sostituzione esatta dell’immagine16bit nonostante opacità/feather,
round-trip, rifiuto modo sconosciuto e fixture legacy. Test widget EN/IT:
selezione dei due modi, stato e abilitazione contestuale dei controlli. Suite
specifiche passate localmente. Seam ottimizzata e maschere non sono ancora
dichiarate presenti. Pubblicazione codice:
`b16d825bd6084259434467c3d5a84b9fcd0e2d56`.

## Correzione dai log utente — GPU OpenCL divergente e installazione bloccata — 2026-11-09

Log reali ricevuti (Ubuntu con «Intel(R) Graphics», driver 26.05.037020): la
build e 24/25 test passano, ma `opencl-gpu` fallisce con
`affine_8bit_max_error 157.0` e «Requested OpenCL affine differs from CPU by
more than one level». Con `set -Eeuo pipefail`, il fallimento di `ctest`
interrompeva `install.sh` in fase test, quindi `.orbyx-install-success` non
veniva scritto e `start.sh` rifiutava l’avvio con «Installazione incompleta».

Diagnosi: due difetti collegati. (1) **Correttezza**: il percorso `generate`
abilitava OpenCL (`setUseOpenCL(!cpu && !calibrate)`) e `align()` esegue
`warpAffine` su `cv::UMat`, quindi su GPU. L’equivalenza GPU non era mai stata
verificata; su questa GPU reale l’affine diverge di 157 livelli, perciò senza
`--cpu` i prodotti allineati sarebbero stati **errati** senza avviso.
(2) **Installazione**: una proprietà del driver GPU, in un percorso che
l’utente può non usare, bloccava l’installazione di un’app che funziona su CPU.

Correzioni:
- **Motore** (`src/core/engine.cpp`): nuova sonda `openclAffineStatus()`
  (una volta per processo) che enumera i dispositivi e confronta lo stesso
  `warpAffine` INTER_LINEAR|WARP_INVERSE_MAP su CPU e GPU a 8 e 16 bit, con la
  stessa tolleranza del verificatore. La GPU viene usata **solo** se riproduce
  la CPU entro un livello; altrimenti `generate` resta su CPU con avviso EN/IT
  «OpenCL GPU affine diverges from CPU; using CPU backend». Nessun dispositivo
  GPU ⇒ «Backend: CPU» come prima, senza avviso. Selezione del backend per
  correttezza; algoritmo e trasformazioni invariati.
- **Test** (`CMakeLists.txt`): `opencl-gpu` etichettato `LABELS gpu`, invariato
  nel resto (ancora `SKIP_RETURN_CODE 77`, ancora esegue e riporta i 157 livelli).
- **Installer** (`install.sh`): il gate funzionale ora è
  `ctest --label-exclude gpu` (tutti i test ORBYX, fallimenti ancora fatali);
  la verifica GPU gira a parte in `orbyx_gpu_diagnostic`, che riporta
  verificata / assente / divergente in EN/IT e **non** interrompe mai
  l’installazione. Il test non è disabilitato né nascosto.

Verifica effettiva (container pulito Ubuntu 24.04.5, OpenCV 4.6.0, **senza GPU**):
build headless pulita; gate funzionale 18/18 passati con la label gpu esclusa;
diagnostica GPU classificata «skipped → ORBYX usa la CPU», ritorno 0, nessun
abort (trap ERR non scattata). `generate` sintetico stampa «Backend: CPU» senza
avviso spurio. La classificazione «divergente» (status `failed`, uscita ctest 8)
è ora una regressione automatica in `tests/test_install.py`: verifica messaggio,
prosecuzione dell'installazione, marker di successo e avvio tramite `start.sh`.
I cinque test dell'installer passano. La riproduzione su una GPU realmente divergente non è
eseguibile in questo runtime senza hardware: il percorso di fallback del motore
è coperto dalla logica e dalla sonda identica al verificatore, non da una GPU
divergente simulata. La verifica remota completa (incluse GUI e CI) è da
osservare dopo il push.

## Usabilità GUI: profilo incluso e simulazione immagini manuale — 2026-11-09

Dai riscontri utente: «non ho profili .json da caricare» e «vorrei fare
simulazioni manuali anche con le immagini della cartella demo». Modifiche di
sola interfaccia, senza toccare il motore né gli algoritmi.

- **Profilo mai bloccante**: il campo «Profilo di calibrazione» era già
  precompilato col profilo incluso (`resources/calibration.json`, installato in
  `share/orbyx/calibration.json`); ora l'etichetta dice «(incluso)», la
  risoluzione del percorso è in un helper `bundledProfile()` e `execute()`
  ripristina il profilo incluso se l'utente svuota il campo. Così servono solo
  origine e destinazione; nessun `.json` da procurarsi.
- **Simulazione immagini manuale**: nuovo pulsante «Simulazione immagini…»
  (`simulateButton`, Ctrl+Maiusc+S) che esegue la stessa pipeline reale della
  demo su una cartella **scelta dall'utente**, con default la cartella demo
  inclusa; accetta sei JPEG `sx_/dx_ MSUGS-VIS-1/2/3`. `executeImageDemo` ora
  prende una cartella opzionale; nessuna rete né shell implicita (riusa
  `orbyx::runImageDemo`). Rifiuto esplicito di cartelle senza i sei file.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen): build GUI
pulita; `gui-widget-flow` esteso con un caso che verifica presenza/accessibilità
del nuovo pulsante e l'esecuzione su cartella scelta (composito 1428×1428
mostrato, progress 100, nessun ERROR). Suite completa 24/24 passata con la label
`gpu` esclusa (la verifica GPU resta diagnostica separata). Screenshot IT/scuro
1280×1000 ispezionato: nuovo pulsante nell'intestazione, etichetta «(incluso)»
col percorso precompilato, layout integro. Verifica remota/CI da osservare dopo
il push.

Chiarimento onesto sullo stato complessivo: non è stato «finito tutto» il
ROADMAP. Restano aperti, come già dichiarato, i punti che richiedono hardware o
prove manuali reali (equivalenza GPU su dispositivo, geolocalizzazione assoluta
e verità a terra, accessibilità con screen reader fisico, aggiornamento/rollback
firmato con chiave reale). L'app è pienamente usabile senza dati né `.json`.

### M2-10 — Secondo incremento: maschere rettangolari di esclusione

Prossimo task di roadmap (M2-10, filone attivo del compositore). Aggiunta una
**maschera di esclusione rettangolare per livello**, non distruttiva: il campo
`MosaicLayer.exclude` indica una regione del livello (nel sistema del suo
ritaglio) che **non contribuisce** alla composizione, rivelando i livelli
sottostanti; i pixel sorgente non vengono mai modificati. Il renderer salta i
soli pixel la cui coordinata sorgente cade nella maschera. Larghezza o altezza
zero = «nessuna maschera», così le modifiche parziali dei quattro controlli
(`excludeX/Y/W/H`) restano sempre valide; la maschera è salvata nel progetto
come campo `exclude` opzionale e i progetti precedenti, che lo omettono, restano
identici. Nessuna modifica al percorso ROI storico (il compositore è distinto da
`generate`). Validazione: non negativi e ≤100000; a render, maschera entro la
regione del livello.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-regression` esteso con prova byte-per-byte — la maschera rivela
esattamente il livello sottostante nella regione indicata, lascia invariato il
resto del livello superiore, non altera il file sorgente, round-trip del
progetto identico ai pixel, maschera inattiva (dimensione zero) non serializzata,
e rifiuto di valori negativi/oltre i limiti e di maschera fuori regione.
`mosaic-widget-flow` esteso EN/IT — i quattro controlli esistono, l'edit aggiorna
il livello selezionato, l'anteprima resta valida, la maschera è registrata nel
progetto e azzerabile. Suite completa 24/24 passata con label `gpu` esclusa;
screenshot del compositore ispezionato. In questa fase restavano aperti, per
M2-10, seam ottimizzata, maschere poligonali e compensazione visuale.

### M2-10 — Terzo e quarto incremento: maschere poligonali

Il core ora applica per ogni livello più poligoni non distruttivi nelle
coordinate del ritaglio. Le aree **includi** formano un'unione, le aree
**escludi** hanno precedenza e rivelano i livelli inferiori. La maschera viene
calcolata per tile, senza creare una bitmap grande quanto l'intera scena; sono
imposti limiti a numero di maschere e vertici. Progetti precedenti senza il
campo opzionale `masks` conservano lo stesso risultato. Le regressioni coprono
16 bit, precedenza, persistenza, sorgenti invariate e input invalidi.

Nell'interfaccia, **Disegna maschera poligonale…** apre il livello ritagliato e
permette di aggiungere almeno tre vertici, scegliere includi/escludi, annullare
l'ultimo punto o cancellare il disegno. Applicazione e cancellazione delle
maschere entrano nella cronologia Annulla/Ripeti. Compilati con successo il
core, l'oggetto del test widget e l'oggetto della GUI; l'esecuzione pulita della
suite completa resta da confermare nella CI dei commit `655c6a4` e `f134146`.
A questo punto restavano seam ottimizzata e compensazione visuale.

### M2-10 — Quinto incremento: giunzione a costo minimo

Aggiunta una terza modalità esplicita per livello, distinta da sostituzione e
sfumatura. ORBYX calcola una cucitura deterministica nella sovrapposizione
minimizzando differenze di intensità e gradiente; la working image ha lato
massimo 2048 pixel, mentre il renderer finale resta a tile e 8/16 bit nativi.
Le maschere include/escludi partecipano alla copertura. Se non esiste un
percorso continuo, il risultato registra il fallback e la GUI avvisa che sono
stati usati i parametri di sfumatura salvati. La modalità viene persistita nel
progetto e i file precedenti mantengono la semantica storica. Compilati senza
warning core, GUI e oggetti dei test sul commit `53732bb`; il link finale locale
resta bloccato dalle librerie esterne isolate già note, quindi la suite completa
non viene dichiarata superata fino alla CI pulita. A questo punto restavano
blending multibanda e compensazione visuale.

### M2-10 — Sesto incremento: compensazione visuale proposta

Il livello selezionato può ora essere uniformato visualmente a quello adiacente
senza uscire da ORBYX. Il core campiona al massimo circa 150000 punti della zona
comune, applica geometria, ritagli e maschere, elimina campioni quasi neri o
saturi e usa mediane robuste per proporre variazione EV e moltiplicatori RGB.
Servono almeno 64 campioni affidabili. La finestra di conferma mostra i valori e
non applica nulla automaticamente; una proposta riferita a geometria o sorgente
ormai cambiate viene rifiutata. L'applicazione modifica soltanto i parametri
visuali del progetto, è tracciata e annullabile e lascia invariati originali e
canali SatDump. Fixture 16 bit, mancata sovrapposizione, sorgenti invariati e
flusso Annulla sono coperti dai test; core, GUI e oggetti test compilano sul
commit `2a1daec`. Resta da valutare il blending multibanda e da eseguire la
suite in un ambiente pulito con tutte le dipendenze runtime.

## 2026-11-10 — Ripresa dopo lavoro utente; M2-07 anteprima comparativa ROI/attuale

All'avvio il lavoro precedente (fix GPU, profilo incluso, simulazione immagini,
maschere di esclusione) risultava integrato su `main` tramite PR #8, insieme ad
ampi avanzamenti M2-10 dell'utente (maschere poligonali include/escludi, giunzione
a costo minimo, compensazione visuale, blend `Seam`). Ho ripreso il branch
`claude/charming-dijkstra-fl8tw8` dall'ultima `main` (il mio PR era già unito) e
seguito il gate di roadmap: il prossimo task P0 ancora al primo incremento è
**M2-07** (coordinatore delle strategie di unione), prima di M2-08.

Incremento M2-07 implementato: **anteprima comparativa affiancata ROI / stato
attuale**. Nuovo comando GUI «Confronta ROI / stato attuale…»
(`mosaicCompareStrategies`) e metodo puro `strategyComparisonImage(edge)` che
rende in un'unica immagine la composizione ROI del profilo (sinistra) e la
strategia corrente — visuale o manuale — (destra), etichettate EN/IT e separate
da un divisore. La scelta è esplicita (Mantieni ROI / Mantieni stato attuale);
nulla viene applicato o esportato senza conferma e la baseline ROI
(`profileRoiBaseline`) non viene mai modificata. Non tocca il percorso ROI
storico né la CLI `generate`.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-widget-flow` esteso — il controllo esiste ed è accessibile; dopo una
modifica del livello corrente, `strategyComparisonImage` produce un'immagine
8-bit affiancata la cui larghezza contiene entrambe le strategie, con entrambe le
metà non vuote e i livelli correnti invariati dall'operazione. Suite completa
24/24 passata con label `gpu` esclusa; immagine di confronto ispezionata
(etichette «ROI profilo» / «Attuale», divisore). Resta aperto, per M2-07,
l'assistenza con più di due porzioni dal flusso principale.

## 2026-11-10 — M2-07 completato (ambito GUI): coordinatore strategie e >2 porzioni

Ripreso il branch dall'ultima `main` (il mio PR #10 con l'anteprima comparativa
era già unito). Completato l'ultimo punto aperto di M2-07: l'assistenza con più
di due porzioni dal flusso principale.

Aggiunto il coordinatore **«Strategia consigliata…»** (`mosaicRecommend` /
`recommendStrategy()`): per qualunque numero di porzioni (≥2) sceglie la strategia
raccomandata. Con una coppia ROI del profilo completa raccomanda la ROI storica
senza modificare nulla; altrimenti avvia la cascata visuale automatica (conferma
esplicita). Con **più di due porzioni**, `autoAlign` ora riporta `unalignedCount`
e, per ogni porzione senza corrispondenza affidabile, instrada esplicitamente ai
punti di controllo (assistita) o al posizionamento manuale, **senza inventare una
posizione**. Le quattro strategie (Profilo ROI, Visuale automatica, Assistita,
Manuale) restano coordinate; il percorso ROI storico e la CLI `generate` non sono
toccati.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-widget-flow` esteso — (a) con coppia ROI completa il coordinatore
raccomanda `profile-roi` senza cambiare i livelli; (b) con **quattro porzioni**
(tre sovrapposte + una da una scena non correlata) e nessuna ROI, il coordinatore
raccomanda e applica la cascata visuale, allinea almeno le due porzioni
sovrapposte oltre all'ancora, segnala `unalignedCount≥1` e il messaggio instrada
la porzione non corrisposta ad «assistita/assisted». Suite completa 24/24 passata
con label `gpu` esclusa.

M2-07 passa da «in corso» a «verificato per l'ambito GUI dichiarato». Resta
esterno all'ambito il rifattorizzare le strategie in un unico servizio core
unificato; lo screen reader reale è in M4-01.

## 2026-11-10 — M2-08 primo incremento: ripresa del wizard guidato

Dopo M2-07, avviato M2-08 come richiesto. Primo incremento: **persistenza e
ripresa dello stato del wizard** «Crea immagine finale». Il wizard salva
automaticamente (autosave) origine, profilo, destinazione, satellite, motore,
resa colore e passaggio raggiunto in un progetto `orbyx-final-wizard/1`
(`QStandardPaths::AppDataLocation`/final-wizard.json) a ogni modifica di campo o
cambio pagina. Metodi nuovi: `wizardProject()`, `applyWizardProject()`,
`saveProject()`, `hasSavedProject()`, `resumeFromSaved()`. Le connessioni di
autosave sono aggiunte dopo l'impostazione dei valori iniziali, quindi aprire un
nuovo wizard non sovrascrive mai una sessione salvata: solo le modifiche
successive dell'utente la aggiornano. Il wizard prepara solo le impostazioni;
l'esecuzione resta della finestra principale (nessuna rete o shell implicita).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`gui-widget-flow` esteso EN/IT — un wizard con valori impostati salva il progetto;
un secondo wizard, costruito con valori diversi, rileva il progetto salvato e lo
riprende, ripristinando esattamente origine/profilo/destinazione/satellite/motore
e resa colore. Suite completa 24/24 passata con label `gpu` esclusa.

Restano aperti, per M2-08: coda/ripresa durevole del batch nello stesso wizard,
riconoscimento di gruppi generici oltre ai due VIS RDAS, e salvataggio del
risultato per singolo passaggio con verifica del digest.

## 2026-11-10 — Verifica stato e M2-08 terzo incremento: rilevamento prodotti import

Verifica dello stato richiesta dall'utente: su `main` (merge PR #11, `5cd238c`)
M2-07 è verificato e M2-08 è «in corso» con due incrementi; M2-09 implementato
(verifica remota), M2-10 in corso (sei incrementi), M2-11/12/13 ancora proposti.
Nessun ramo remoto oltre `main`. Siamo quindi effettivamente ancora a M2-08.

Terzo incremento M2-08: **rilevamento dei prodotti nel passo «Importa»**. Il
wizard riconosce e riporta, senza dipendere dai nomi SX/DX, qualunque prodotto
`MSUGS_VIS*` con i tre canali e i prodotti già allineati presenti nella cartella
di import, esponendo le proprietà `detectedVisProducts`/`detectedAlignedProducts`
e un riepilogo EN/IT (`finalImportSummary`). È sola lettura: non modifica mai
l'esportazione SatDump; scansione limitata (max 64 voci, rifiuto symlink).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`gui-widget-flow` esteso EN/IT — con una cartella contenente `MSUGS_VIS1/2/3` e
un prodotto `MSUGS_VIS-1_Aligned`, il rilevamento riporta 3 prodotti VIS e 1 già
allineato e il riepilogo lo mostra. Suite completa 24/24 passata con label `gpu`
esclusa.

Restano aperti, per M2-08: coda/ripresa durevole del batch nel wizard, la
**correzione interattiva** del raggruppamento (oltre al riconoscimento) e il
salvataggio del risultato per singolo passaggio con verifica del digest.

## 2026-11-10 — M2-08 quarto incremento: coda durevole dal wizard guidato

Continuazione di M2-08. Al passo «Esporta» del wizard «Crea immagine finale» una
casella opt-in **«Aggiungi questo lavoro alla coda durevole invece di avviarlo
subito»** (`finalQueue`, getter `queueInsteadOfRun()`) consente di accodare il
lavoro guidato sulla coda durevole e ripristinabile già verificata, anziché
avviarlo immediatamente. La scelta è persistita nel progetto del wizard (campo
`queue`, ripristinata al resume). Nella finestra principale la consegna del
wizard, quando l'opzione è attiva, chiama `addToQueue()` senza aprire l'editor;
altrimenti mantiene il comportamento esistente (esecuzione immediata + editor).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`gui-widget-flow` esteso EN/IT — (a) round-trip del progetto: la scelta coda è
salvata e ripristinata (`queueInsteadOfRun` vero dopo resume); (b) consegna reale:
completando il wizard con l'opzione attiva, la tabella della coda guadagna una
riga, l'editor **non** si apre e il prodotto **non** viene generato subito (job
accodato, non eseguito); la coda viene poi ripristinata allo stato precedente per
non alterare le asserzioni di persistenza successive. Suite completa 24/24 passata
con label `gpu` esclusa.

Restano aperti, per M2-08: la correzione interattiva del raggruppamento dei
prodotti rilevati e il salvataggio del risultato per singolo passaggio con
verifica del digest.

## 2026-11-10 — «Avvia demo immagini» → «Generazione automatica»

Richiesta utente: trasformare il pulsante demo in una generazione automatica a
cartella fissa, senza parametri. Il pulsante (già `demoButton`, scorciatoia
Ctrl+Maiusc+D) è stato rinominato **«Generazione automatica»** (EN «Automatic
generation») e ora elabora i sei JPEG `sx_/dx_ MSUGS-VIS-1/2/3.jpg` presenti in
una cartella fissa `AppDataLocation/auto-input`, riusando la stessa pipeline reale
della demo (registrazione fase/ECC, Natural Color, giunzione) tramite
`orbyx::runImageDemo`. Nessun parametro richiesto all'utente.

Comportamento: alla prima pressione la cartella viene creata e riempita con i sei
JPEG inclusi come esempio, così funziona subito; l'utente poi li sostituisce con
le proprie immagini e ripreme. Se mancano i sei file e non è disponibile
l'esempio incluso, un messaggio EN/IT indica il percorso esatto in cui inserirli.
Il percorso è registrato nel log Attività. Il pulsante «Simulazione immagini…»
(cartella scelta manualmente) resta disponibile e invariato.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`gui-widget-flow` esteso EN/IT — il pulsante è rinominato, usa la cartella fissa
`auto-input` (log), produce il composito 1428×1428 mostrato in anteprima, progress
100, nessun errore. Suite completa 24/24 passata con label `gpu` esclusa;
screenshot IT/scuro ispezionato («Generazione automatica» in intestazione).
README aggiornato.

## 2026-11-10 — M2-08 quinto incremento: correzione interattiva del raggruppamento

Continuazione roadmap (M2-08). Il passo «Importa» del wizard ora, oltre a
rilevare i prodotti, li elenca come voci **spuntabili** (`finalProductList`):
ogni prodotto `MSUGS_VIS*` (tre canali) e ogni prodotto già allineato è mostrato
con etichetta EN/IT (VIS/Allineato) e il nome cartella, senza dipendere dai nomi
SX/DX. Tutti partono selezionati; l'utente può **escluderne** alcuni
dall'elaborazione. `selectedProducts()` e la proprietà `selectedProductCount`
espongono la scelta; il riepilogo mostra «rilevati N; M selezionati». Le scelte
di inclusione/esclusione sono conservate fra i cambi pagina e le ri-rilevazioni
(snapshot degli esclusi); nessuna modifica all'esportazione SatDump.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`gui-widget-flow` esteso EN/IT — con una cartella `MSUGS_VIS1/2/3` + un
`MSUGS_VIS-1_Aligned` la lista mostra 4 voci tutte selezionate
(`selectedProductCount==4`); deselezionando `MSUGS_VIS3` il conteggio scende a 3,
`selectedProducts()` non lo contiene più e il file sorgente resta intatto. Suite
completa 24/24 passata con label `gpu` esclusa. Corretto anche il conteggio degli
incrementi di M2-08 nella roadmap (ora cinque).

Resta aperto, per M2-08: il salvataggio del risultato per singolo passaggio con
verifica del digest e, a valle, l'uso del raggruppamento corretto nel percorso di
esecuzione (oggi il motore `generate` è vincolato ai due VIS RDAS).

## 2026-11-10 — M2-08 completato (ambito GUI): verifica digest alla ripresa

Ultimo incremento di M2-08. Il progetto resumibile del wizard salva ora un
**digest degli ingressi** che determinano i risultati: identità della cartella di
origine, satellite, motore e **contenuto del profilo di calibrazione** (SHA-256,
limitato a 1 MiB). Alla ripresa (`applyWizardProject`/`resumeFromSaved`) il digest
viene ricalcolato dallo stato corrente su disco e confrontato con quello salvato;
`resumeDigestMatches` espone l'esito. Se il profilo o la configurazione sono
cambiati dal salvataggio, un avviso EN/IT nel riepilogo import dichiara che i
risultati precedenti non sono affidabili e i passaggi vanno rieseguiti.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`gui-widget-flow` esteso EN/IT — con profilo invariato la ripresa è «affidabile»
(`resumeDigestMatches` vero); modificando il contenuto del profilo la ripresa è
segnalata «non affidabile» (digest diverso) e il riepilogo avvisa l'utente
(«cambiati»/«changed»). Suite completa 24/24 passata con label `gpu` esclusa.

M2-08 passa da «in corso» a «verificato per l'ambito GUI dichiarato» (sei
incrementi). Restano esterni all'ambito GUI: il riuso effettivo dei risultati per
singolo passaggio in esecuzione (già governato dai digest del runner batch già
verificato) e l'uso del raggruppamento corretto nel motore `generate`, oggi
vincolato ai due VIS RDAS. Prossimo nel gate: completare M2-09 (verifica remota)
e M2-10 (blending multibanda), poi M2-11.

## 2026-11-10 — M2-10 chiuso (ambito dichiarato): blending multibanda

Ultimo incremento di M2-10, su richiesta dell'utente. Aggiunta la modalità di
blend per livello **Blending multibanda (visuale)** (`MosaicBlend::Multiband`),
quarta voce del selettore accanto a Nessun blending/Sfumatura/Giunzione.

Implementazione (core, `src/core/compositor.cpp`): una fusione a **piramide
laplaciana** deterministica a profondità nativa, eseguita sul riquadro del livello
(non a tile). La maschera di fusione è una **rampa sulla sovrapposizione** basata
sulla distance transform dai livelli sottostanti, normalizzata sull'estensione
reale dell'overlap: le basse frequenze si fondono su una banda ampia e le alte su
una stretta, nascondendo i salti di esposizione/colore lungo la giunzione. Il
bordo adiacente ai livelli sottostanti sfuma esattamente verso di essi (nessun
gradino), il lato senza nulla sotto resta il livello, e la scrittura avviene solo
dove il livello copre (sorgenti e livelli sottostanti non toccati fuori dalla sua
area). Rispetta ritagli ed esclusioni/maschere poligonali. Oltre 8 Mpx o su
regioni < 4 px ricade in modo deterministico sulla sfumatura esistente. Le
modalità Nessun blending/Sfumatura/Giunzione restano **byte-identiche**.
Serializzazione «multiband» compatibile; selettore GUI EN/IT a quattro voci.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-regression` esteso — il blend multibanda è deterministico (due render
identici), a profondità nativa 16 bit, senza buchi, conserva esatta la regione
non sovrapposta del solo livello sottostante, produce una vera transizione sulla
giunzione e fa round-trip nel progetto. `mosaic-widget-flow` esteso — quarta voce
del selettore applica `Multiband` con i controlli contestuali corretti. Suite
completa 24/24 passata con label `gpu` esclusa. Confronto visuale Seam vs
Multiband su scena con gradino di esposizione ispezionato: il gradino lungo la
giunzione è attenuato. Resta la conferma dell'intero percorso nella CI pulita.

M2-10 passa da «in corso» a «verificato per l'ambito dichiarato» (sette
incrementi). Prossimo nel gate: M2-11 (colore integrato), poi M2-12/M2-13.

## 2026-11-10 — M2-11 primo incremento: report colore/passaggi all'export

Avviato M2-11 (colore integrato) dopo la chiusura di M2-10. Primo incremento:
**report colore e dei passaggi all'export** del compositore. Nuova funzione core
`orbyx::mosaicReport(layers, output, bitDepth)` che produce un report
deterministico `orbyx-mosaic-report/1`: dichiara la profondità di bit effettiva
(8/16), una nota colore esplicita (resa visuale, non colore calibrato
radiometricamente; profondità 8/16 bit preservata senza conversione silenziosa) e
incorpora l'intero progetto riproducibile (`orbyx-mosaic/1`) di tutti i passaggi
non distruttivi. `MosaicDialog::exportTo` scrive il report accanto al PNG come
`<file>.report.json` in modo esclusivo (nessuna sovrascrittura), con la profondità
presa dal risultato reale (nessuna conversione silenziosa 16→8).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-regression` esteso — il report è deterministico, contiene schema, nota
colore e progetto incorporato, registra la profondità (report a 8 e 16 bit
differiscono) e rifiuta profondità non valide. `mosaic-widget-flow` esteso —
l'export scrive `<file>.report.json` con schema, `bit_depth` e progetto
incorporato accanto al PNG 16 bit. Suite completa 24/24 passata con label `gpu`
esclusa.

M2-11 passa da «proposto» a «in corso». Restano: preset con nomi chiari (incluso
canale singolo), istogramma con clipping alte luci/ombre e prima/dopo, e
formalizzazione/test dell'ordine di elaborazione colore.

## 2026-11-10 — M2-11 secondo incremento: istogramma e clipping della composizione

Continuazione M2-11. L'anteprima composita del compositore mostra ora un
**istogramma di luminanza** dell'area coperta e un **indicatore di clipping** di
alte luci e ombre. `updateHistogram` calcola, solo nella vista composita
(inspection 0, non nelle modalità di confronto), la copertura dai contorni dei
livelli, un istogramma a 256 bin della luminanza e il conteggio dei pixel saturati
in alto (un canale a 255) e in basso (pixel neri) entro la copertura; disegna le
barre in un `QLabel` (`mosaicHistogram`) con i bordi 0/255 marcati e mostra le
percentuali EN/IT in `mosaicClipping`. Espone `coveredPixels`,
`highlightClippedPixels`, `shadowClippedPixels`.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-widget-flow` esteso — l'istogramma ha un pixmap non nullo, il readout
clipping contiene «%», `coveredPixels>0`; portando l'esposizione a +10 EV e
applicandola a tutti i livelli il clipping delle alte luci aumenta oltre la
baseline (>0), poi ripristinato. Suite completa 24/24 passata con label `gpu`
esclusa; screenshot del compositore ispezionato (istogramma e readout «alte luci
0.00% · ombre 14.13%» sulla demo).

Restano aperti, per M2-11: i preset colore con nomi chiari (incluso canale
singolo) e la formalizzazione/test dell'ordine di elaborazione colore.

## 2026-11-10 — M2-11 terzo incremento: preset colore (incluso canale singolo)

Continuazione M2-11. Il selettore «Aggiungi 3 canali allineati» del compositore
offre ora cinque preset con nomi chiari: **Natural Color (RGB321)**, **RGB123** e
tre preset **canale singolo (scala di grigi)** CH1/CH2/CH3. Nuova funzione core
`orbyx::composeSingleChannel(channels, destination, index)` che scrive il canale
scelto come prodotto neutro (R=G=B replicato) preservando la profondità 8/16 bit,
lasciando gli input invariati e con pubblicazione atomica esclusiva. Nessun preset
è etichettato «colore vero scientifico» (coerente con «nessun preset definito
colore vero senza calibrazione radiometrica»).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6):
`engine-regression` esteso — `composeSingleChannel` produce un prodotto 3 canali
neutro (R=G=B = canale scelto) a 8 e 16 bit, input invariati, rifiuto di indice
non valido. `mosaic-widget-flow` esteso — il selettore colore ha 5 voci con nomi
chiari (nessuna etichetta «true scientific»); `appendSingleChannel` sul CH2 crea
un livello grigio neutro a 16 bit col valore del canale. Suite completa 24/24
passata con label `gpu` esclusa.

Restano aperti, per M2-11: la formalizzazione/test dell'ordine di elaborazione
colore (allineamento → unione → composizione colore → correzioni visuali).

## 2026-11-10 — M2-11 chiuso (ambito dichiarato): ordine di elaborazione colore

Ultimo incremento di M2-11. **Formalizzato e testato l'ordine di elaborazione
colore.** Il report di export (`mosaicReport`) dichiara ora un campo `pipeline`
con l'ordine formale: allineamento canali a geometria condivisa → unione con la
stessa geometria per R/G/B (una sola trasformazione per livello, quindi nessuna
divergenza per canale né bordi colorati) → composizione colore (RGB321 / RGB123 /
canale singolo) → correzioni visuali non distruttive.

Test di regressione aggiunti: `engine-regression` verifica che la composizione
colore richiede i tre canali con **geometria identica** — rifiuto su **dimensioni
diverse** (oltre al già coperto rifiuto su profondità diversa), così R/G/B non
ricevono mai geometrie diverse; `mosaic-regression` verifica che la pipeline è
dichiarata nel report e che una **traslazione intera** di un livello a colori
sposta i tre canali **insieme** (risultato ritagliato identico alla sorgente,
nessuna divergenza per canale).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen): suite
completa 24/24 passata con label `gpu` esclusa.

M2-11 passa da «in corso» a «verificato per l'ambito dichiarato» (quattro
incrementi: report, istogramma/clipping, preset, ordine colore). La validazione
colorimetrica su scene reali resta in M3-04. Nel gate restano M2-12 (report
qualità e scelta consigliata) e M2-13 (progetto portabile).

## 2026-11-10 — M2-12 primo incremento: indicatori di qualità della composizione

Avviato M2-12 (valutazione qualità). Primo incremento: **indicatori di qualità**
della composizione nel compositore. Nuova funzione core `orbyx::mosaicQuality`
(deterministica, resa a risoluzione ridotta bounded) che calcola: aree
vuote/mancanti entro il riquadro di copertura (`emptyFraction`), copertura della
sovrapposizione con ≥2 livelli (`overlapFraction`), clipping di alte luci e ombre
(`highlightClipped`/`shadowClipped`) e numero di trasformazioni estreme
(`extremeTransforms`: rotazione ≥45° o scala fuori [0.5, 2]). Il desktop espone il
comando «Valutazione qualità…» (`mosaicQuality`) che mostra le evidenze EN/IT e le
proprietà corrispondenti; **non dichiara mai «perfetta» un'immagine e non modifica
il progetto** (solo evidenze, scelta all'utente).

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-regression` esteso — singolo livello pieno (nessuna area vuota, nessuna
sovrapposizione), due livelli sovrapposti (overlapFraction>0), due separati
(emptyFraction>0, overlap 0), esposizione estrema (highlightClipped>0.5),
rotazione 90° (una trasformazione estrema), e determinismo. `mosaic-widget-flow`
esteso — il comando è presente/accessibile, il report dichiara esplicitamente che
non considera perfetta un'immagine, espone le proprietà (`qualityCovered>0`) e non
modifica il progetto. Suite completa 24/24 passata con label `gpu` esclusa.

Restano aperti, per M2-12: differenza/continuità lungo la seam, report comparativo
ROI/visuale/manuale con raccomandazione e casi ambigui, validazione su dataset con
verità nota e prova di usabilità.

## 2026-11-10 — M2-12 secondo incremento: differenza giunzione e raccomandazione

Continuazione M2-12. (A) **Differenza lungo la giunzione**: `orbyx::mosaicQuality`
calcola ora `seamDifference` — ogni livello viene reso da solo sul canvas completo
(gli altri restano visibili ma non contribuiscono, così i limiti coincidono) e le
coppie sovrapposte vengono confrontate in scala di grigi nell'area comune; più alta
= giunzione più visibile. Deterministica e con memoria limitata (resa a risoluzione
ridotta). (B) **Raccomandazione comparativa**: il report «Valutazione qualità…»
mostra la differenza di giunzione e, quando esiste una baseline ROI del profilo,
confronta ROI e stato attuale con un punteggio di «concern» (aree vuote +
giunzione + clipping) e raccomanda la strategia migliore, segnalando i casi
**ambigui**; la scelta resta all'utente (proprietà `qualityRecommended` ∈
profile-roi/current/ambiguous), **nessuna sostituzione automatica**.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-regression` esteso — due livelli diversi sovrapposti hanno
`seamDifference>0.2`, due identici `<0.05` (solo residuo di bordo), deterministica.
`mosaic-widget-flow` esteso — con baseline ROI il report espone
`qualityRecommended` (profile-roi/current/ambiguous) e `qualitySeamDifference` e
non modifica il progetto. Suite completa 24/24 passata con label `gpu` esclusa.

Restano aperti, per M2-12: validazione sui dataset con verità nota (criteri
quantitativi) e prova di usabilità con utenti.

## 2026-11-10 — M2-12 terzo incremento: validazione quantitativa su verità nota

Continuazione M2-12. Aggiunta una **validazione sui criteri quantitativi** con
verità nota in `mosaic-regression` (solo test, nessuna modifica al codice).
Riusando la scena testurizzata già presente, due porzioni sovrapposte tagliate con
offset noto: l'allineamento automatico recupera la geometria entro tolleranza (già
verificato) e `mosaicQuality` distingue quantitativamente i casi:
- allineamento corretto → `seamDifference`≈0,005, overlap≈0,22, aree vuote≈0,12;
- disallineamento di 40/30 px → `seamDifference`≈0,25 (chiaramente maggiore);
- porzione lontana (nessuna sovrapposizione) → overlap 0, aree vuote≈0,47.

I criteri quantitativi (seam basso su allineamento corretto, penalizzazione del
disallineamento, classificazione del no-overlap) sono asseriti con margini ampi.
Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen): suite
completa 24/24 passata con label `gpu` esclusa.

M2-12 passa a «verificato per l'ambito core/GUI dichiarato» (tre incrementi:
indicatori, differenza giunzione + raccomandazione, validazione quantitativa).
Resta fuori dall'ambito automatizzabile la prova di usabilità con utenti reali e
screen reader (coerente con M4-01). Nel gate resta M2-13 (progetto portabile e
recupero del lavoro).

## 2026-11-10 — M2-13 primo incremento: progetto portabile (riferimenti relativi)

Avviato M2-13 (progetto portabile e recupero del lavoro). Primo incremento:
**progetto portabile con riferimenti relativi**. Aggiunte varianti con cartella
base a `orbyx::mosaicProject`/`parseMosaicProject`: le immagini sotto la cartella
del progetto sono salvate con percorso **relativo** e risolte rispetto a quella
cartella alla riapertura; le immagini fuori restano assolute. Sicurezza: i
percorsi relativi che **escono** dalla cartella (es. `../`) sono rifiutati, e un
percorso relativo senza cartella base è rifiutato. Le funzioni originali
delegano alla variante con cartella base vuota, quindi il comportamento assoluto
è **identico** a prima (back-compat). `MosaicDialog::saveProject`/`loadProject`
usano ora la cartella del file di progetto come base.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-regression` esteso — un'immagine sotto la cartella è salvata come percorso
relativo (nessun percorso assoluto nel progetto), riaperta da una **cartella
spostata** risolve l'immagine nella nuova posizione; serializzazione assoluta
invariata; percorso relativo senza base e percorso che esce dalla cartella
rifiutati. `mosaic-widget-flow` esteso — un progetto salvato in una cartella con
la sua immagine usa un percorso relativo e, copiando cartella+progetto+immagine in
una nuova posizione, si riapre risolvendo l'immagine nella nuova cartella. Suite
completa 24/24 passata con label `gpu` esclusa.

Restano aperti, per M2-13: autosave e ripristino dopo arresto anomalo, e l'opzione
pacchetto esplicito.

## 2026-11-10 — M2-13 secondo incremento: autosave e ripristino dopo crash

Continuazione M2-13. **Autosave + ripristino dopo arresto anomalo** nel
compositore. Ogni modifica avvia un autosave con **debounce** (timer 150 ms) che
scrive il progetto corrente in uno slot fisso
`AppDataLocation/compositor/recovery.json` (percorsi assoluti, è recupero locale;
scrittura atomica best-effort). Alla costruzione una nuova sessione rileva
l'eventuale lavoro lasciato da una sessione interrotta (proprietà
`recoveryAvailable`) e il comando **«Recupera lavoro autosalvato…»**
(`mosaicRecover`) lo ripristina nei livelli. Il salvataggio esplicito del progetto
(`saveProject`) cancella lo slot (il lavoro è già persistito) e ferma il timer per
evitare riscritture. L'autosave legge i livelli al momento del fire (dopo la
mutazione), non nello snapshot di `remember()`.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-widget-flow` esteso — dopo una modifica l'autosave scrive il file di
recovery; una nuova sessione lo rileva (`recoveryAvailable`), il comando ripristina
il livello autosalvato; il salvataggio del progetto cancella lo slot e una sessione
successiva non vede più recovery. Suite completa 24/24 passata con label `gpu`
esclusa.

Resta aperto, per M2-13: l'opzione pacchetto esplicito (copia delle immagini
accanto al progetto).

## 2026-11-10 — M2-13 chiuso (ambito dichiarato): pacchetto esplicito

Ultimo incremento di M2-13. **Pacchetto esplicito**: comando «Esporta pacchetto
progetto…» (`MosaicDialog::packageProject`) che crea una cartella autoconsistente
contenente il progetto e le **copie** di tutte le immagini sorgente, referenziate
in modo **relativo** (riuso della serializzazione con cartella base) e
**deduplicate** (sorgenti identiche copiate una sola volta). Sicurezza: non scrive
nulla fuori dalla cartella di destinazione, rifiuta sorgenti symlink/mancanti/vuote
o oltre 256 MiB e una destinazione esistente non vuota; i livelli correnti restano
invariati (opera su una copia). Il pacchetto si riapre risolvendo le immagini al
suo interno.

Verifica effettiva (Ubuntu 24.04.5, Qt 6.4.2, OpenCV 4.6, offscreen):
`mosaic-widget-flow` esteso — tre livelli di cui due dalla stessa immagine →
pacchetto con `project.json` e **due** sole immagini copiate (dedup), progetto con
percorsi relativi `image-000...` (nessun percorso sorgente assoluto), livelli
correnti invariati; riapertura del pacchetto con tutti e tre i livelli e immagini
risolte dentro la cartella del pacchetto; destinazione non vuota rifiutata. Suite
completa 24/24 passata con label `gpu` esclusa.

M2-13 passa a «verificato per l'ambito dichiarato» (tre incrementi: riferimenti
relativi, autosave/ripristino crash, pacchetto esplicito). Con M2-13 si conclude
la serie M2 del compositore (M2-07…M2-13) per l'ambito dichiarato. Restano aperti,
come già dichiarato, i punti che richiedono hardware o prove manuali reali (GPU,
verità a terra, screen reader/utenti reali, firma GPG personale).

## 2026-11-10

Documentazione utente aggiornata con le funzioni della serie M2 (nessuna modifica
al codice). In `docs/COMPOSITOR.md`: preset di canale singolo (CH1/2/3 in scala di
grigi, R=G=B, 8/16 bit preservati); comandi **Strategia consigliata…** e
**Confronta ROI / stato attuale…**; **Blending multibanda (visuale)** con maschera
sulla sovrapposizione reale e fallback deterministico alla sfumatura oltre il limite
di dimensione; istogramma di luminanza e riga di clipping; **Valutazione qualità…**
dichiarata esplicitamente come «solo evidenze», che non definisce mai perfetta
un'immagine e non modifica il progetto; correzione del testo obsoleto sui
«riferimenti assoluti» → progetto portabile con percorsi relativi sotto la cartella
e rifiuto dei percorsi che ne escono; autosalvataggio e **Recupera lavoro
autosalvato…**; **Esporta pacchetto progetto…** autonomo con dedup e rifiuti di
sicurezza; report di export `<nome>.report.json` (profondità, nota colore, progetto
riproducibile). Sezione «Verifica» estesa con le nuove prove `mosaic-regression`
e `mosaic-widget-flow`. In `README.md` aggiornato il riassunto del compositore.
Onestà mantenuta: rese visive e non calibrazioni radiometriche, nessuna
georeferenziazione, nessun verdetto di «perfezione».

README reso più chiaro sull'uso: aggiunta la guida «Quale pulsante premo?» (tabella
situazione→pulsante) in `README.md` (IT) e `README.en.md` (EN), con avviso esplicito
sull'errore più comune — **Crea immagine finale…** accetta solo dati veri di SatDump
(cartelle `MSUGS_VIS*`), non le foto demo, per cui mostra il messaggio rosso «non è
un'esportazione VIS»; per le foto demo si usano **Generazione automatica** o
**Simulazione immagini…**. Chiarita la differenza cartelle-SatDump vs sei JPEG e
aggiornato il pulsante demo EN al nome reale «Automatic generation». Solo
documentazione, nessuna modifica al codice o al comportamento.

Correzione struttura README: il README italiano è `README.it.md` (coppia con
`README.en.md`); `README.md` era una copia italiana più recente che aveva superato
`README.it.md` lasciandolo obsoleto. Consolidato il contenuto italiano aggiornato
(inclusa la guida «Quale pulsante premo?») in `README.it.md`, ripuntati i link di
lingua a `README.it.md` in entrambi i file e rimosso `README.md`. Le due note
precedenti di oggi citano `README.md` solo come registro storico.

Convenzione README finale su richiesta: inglese in `README.md` (pagina iniziale del
repository su GitHub) e italiano in `README.it.md`. Spostato il contenuto inglese da
`README.en.md` a `README.md` (rename, storia conservata), ripuntati i link di lingua
all'inglese su `README.md` in entrambi i file. `README.en.md` rimosso. Restano due
soli README: `README.md` (EN) e `README.it.md` (IT).

## 2026-11-13 — Limiti demo rilassati su richiesta (peso, risoluzione, satellite L3)

Su richiesta dell'utente (errore reale «Demo JPEG size exceeds bounds» nella
Generazione automatica) rilassati i vincoli rigidi della demo, mantenendo la demo
inclusa byte-identica:

- **Peso**: il limite per JPEG demo passa da 1 MiB a **40 MiB** (`src/core/image_demo.cpp`,
  `readDemoJpeg`). Il test del limite in `tests/test_image_demo.cpp` è aggiornato a
  40 MiB + 1 (rifiuto effettivo).
- **Risoluzione libera**: tolto il vincolo fisso 714×2048; si accettano sei JPEG **8 bit
  di qualunque risoluzione uguale tra loro** (limiti di sanità ≤20000 px/lato, ≤64 Mpx).
  La demo inclusa 714×2048 segue un **branch dedicato byte-identico** (crop rampe 80 px,
  framing disco 1428×1428, banda seam 140..1380/±12 invariate, seam misurato = 4); le
  altre dimensioni usano un percorso proporzionale: niente ritaglio del disco, esclusione
  rampe disattivata, soglie geometriche e banda del seam scalate. `demo-report.json`
  espone ora `geometry` (input/output, `bundled_sample`).
- **Riapertura come livelli** (`src/gui/mosaic_dialog.hpp`, `appendDemo`): usa la geometria
  reale del risultato; la demo 714×1968 mantiene offset/crop documentati, le altre aprono
  come due livelli a piena immagine affiancati.
- **Satellite predefinito → L3** (`src/gui/main.cpp`): il default di fabbrica passa da L2
  a L3 (`settings.value("satellite","L3")`); le preferenze salvate restano rispettate.
- **Orientamento Nord-up**: le anteprime MSU-GS VIS sono acquisite Sud-up; il composite
  finale della Generazione automatica viene ora **ruotato di 180°** dopo il ritaglio
  (`cv::rotate(raw,ROTATE_180)`), sia per la demo inclusa sia per il percorso generico;
  `demo-report.json` espone `display.rotated_180`. La rotazione è applicata al frame già
  composto (non alle singole porzioni, che altrimenti si scambierebbero di lato). I canali
  intermedi e i file `*-natural.png` restano nell'orientamento sorgente, quindi la
  riapertura nell'editor manuale mostra per ora le porzioni non ruotate (allineamento
  dell'editor da valutare se richiesto).
- **Immagini grandi senza OOM**: limite risoluzione per JPEG demo alzato da 64 a **150 Mpx**
  (coerente con `readImage`, `engine.cpp:129`; il precedente 64 Mpx era incoerente col limite
  peso di 40 MB). L'allineamento ECC e la giunzione sono ora **stimati su un'immagine ridotta
  ≤2048 px** (`cv::resize` + riscalatura di traslazioni e shift) e applicati a **piena
  risoluzione** in export, per contenere CPU e memoria su input grandi. Il bundled 714×1968 e
  ogni immagine con lato ≤2048 px usano `est=1`, quindi stima e byte restano invariati.
  Prova end-to-end: sei JPEG 3000×8000 → composite **6000×8000 (48 Mpx, 27 MB)** in ~11 s;
  `image-demo-regression` esteso con un caso >2048 px (1400×3000 → 2800×3000) oltre a bundled
  e generico piccolo. L'immagine finale resta a piena risoluzione = 2·larghezza·altezza
  dell'input (nessuna riduzione in export; solo l'anteprima a schermo è limitata a 2048 px).
- **Ingrandimento in esportazione (menu 1×–4×)**: nuovo controllo GUI nell'intestazione
  (`exportScale`, persistito EN/IT «Enlarge»/«Ingrandimento») che ingrandisce il composite
  finale per interpolazione (`cv::INTER_CUBIC`) con fattore 1–4. `runImageDemo` ha un parametro
  `exportScale` (default 1, clampato 1–4 e con tetto 20000 px/lato e ~200 Mpx per restare in
  memoria). È interpolazione **dichiarata** (più pixel, non più dettaglio): la demo inclusa resta
  1428×1428 a 1× e diventa 2856/4284/5712 a 2/3/4×; `demo-report.json` espone `display.export_scale`.
  Default 1×, quindi nessun cambiamento se non lo si tocca. `image-demo-regression` (caso ×2 →
  2856×2856), `gui-widget-flow` e `gui-smoke` verdi.
- **Generazione automatica: PNG oltre a JPEG**: `readDemoImage` (ex `readDemoJpeg`) accetta ora sia
  JPEG sia **PNG** (validazione firma + chunk IHDR per le dimensioni; qualunque profondità →
  normalizzata a 8-bit da `IMREAD_GRAYSCALE`); `runImageDemo` cerca `<stem>.jpg` e in mancanza
  `<stem>.png`. Così si usano le esportazioni SatDump a piena risoluzione **senza ricompressione
  JPEG** (nessuna perdita di dettaglio reale). GUI: `hasDemoImages` riconosce entrambi i formati;
  messaggi EN/IT aggiornati («.jpg o .png — il PNG è senza perdita»). `image-demo-regression` esteso
  con un caso PNG; prova end-to-end: sei PNG 3000×8000 → composite 6000×8000 (48 Mpx).
  `gui-widget-flow`, `mosaic-widget-flow`, `gui-smoke` verdi. Resta il modo più fedele in assoluto il
  flusso «Crea immagine — modalità manuale» coi PNG SatDump, ma ora anche la Generazione automatica
  non impone più la perdita del JPEG.

Verifica effettiva locale (Ubuntu 24.04, OpenCV 4.6.0, Qt 6.4.2, GCC 13.3):
- Core: `image-demo-regression` verde con nuovo caso a risoluzione non-standard
  (500×1400 → composite 1000×1400, canali interi) oltre al percorso bundled byte-identico;
  `engine-regression`, `mosaic-regression`, `color-cli-flow`, `simulation-flow` verdi.
- GUI (offscreen): `gui-widget-flow` (Generazione automatica, log 1428×1428),
  `mosaic-widget-flow`, `gui-product-flow`, `gui-smoke` verdi.
- Prova end-to-end aggiuntiva: sei JPEG 600×1680 → `composite.png` 1200×1680, ECC
  0,96–0,99, seam `edge_correlation` 0,83. È una verifica su immagini sintetiche
  (ridimensionate dalle demo), non una nuova acquisizione reale.

Limiti: allineamento ECC e controllo del seam restano pensati per tre bande della stessa
scena con piccolo disallineamento; immagini molto diverse possono essere legittimamente
rifiutate («Unexpected demo geometry» / «no reliable adjacent-edge alignment»). La resa di
contrasto (black 8 / white 188 / gamma 0,8) resta tarata sulla demo. Nessuna rete o shell
introdotta. CI remota da osservare.
