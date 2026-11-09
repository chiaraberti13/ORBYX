# ORBYX — Roadmap operativa

Stati: pianificato, in corso, verificato, bloccato. Pubblicare ogni task verificato direttamente su main senza force push.

## M0-01 — Audit sorgenti, licenze e capacità originali

- Priorità: P0
- Dipendenze: —
- Responsabile: Technical Lead / Secure Code Auditor
- Competenze: C++, OpenCV, licensing
- Deliverable: docs/AUDIT.md; inventario upstream
- Test: Revisione di tutti i sorgenti e kernel
- Accettazione: Baseline fissata a commit e limiti espliciti
- Stato: verificato (revisione statica; validazione runtime successiva)

## M0-02 — Valutazione linguaggi e architettura

- Priorità: P0
- Dipendenze: M0-01
- Responsabile: Languages Architect
- Competenze: C++, Rust, memoria, Qt
- Deliverable: docs/ADR-001-language.md
- Test: Confronto documentato
- Accettazione: Nessuna migrazione senza regressioni di riferimento
- Stato: verificato (revisione statica; validazione runtime successiva)

## M0-03 — Identità, README bilingui e attribuzioni

- Priorità: P1
- Dipendenze: M0-01
- Responsabile: Product Designer / Technical Writer
- Competenze: SVG, localizzazione, GPL
- Deliverable: banner, README, documenti legali
- Test: XML SVG e link locali
- Accettazione: Stile coerente e capacità dichiarate reali
- Stato: verificato (SVG XML e link locali; banner originale)

## M1-01 — Core sicuro e CLI indipendente

- Priorità: P0
- Dipendenze: M0-02
- Responsabile: Computer Vision Engineer / AppSec
- Competenze: ECC, affine, JSON, filesystem
- Deliverable: Core e comandi generate/calibrate
- Test: Fixture sintetiche 8/16 bit e input invalidi
- Accettazione: Canali corretti, errori gestiti, originali preservati
- Stato: verificato su regressioni sintetiche e pipeline RDAS reale Elektro-L4 a16bit; precisione assoluta/geolocalizzazione restano in M3-01

## M1-02 — Regressioni, sanitizers e CI

- Priorità: P0
- Dipendenze: M1-01
- Responsabile: QA / DevSecOps
- Competenze: CMake, CTest, Linux
- Deliverable: Suite, workflow e Dependabot
- Test: Build e CTest locali; CI remota
- Accettazione: Risultati registrati senza inventare compatibilità
- Stato: verificato — Ubuntu22.04/24.04/26.04, suite core/CLI/GUI, JPEG reali, Natural Color, ASan/UBSan/leak detection, Cppcheck, fuzzing e CodeQL verdi su `b86103f`. Nei build GUI:20gruppi,19eseguiti e gate GPU saltato correttamente; nel job sanitizers15gruppi con `detect_leaks=1`. Test di coda/profili/demo visibili EN/IT e decoder JPEG aggiunti. Prove e limiti in [docs/TEST_RESULTS.md](docs/TEST_RESULTS.md). Nessun pass GPU implicato.

## M2-01 — Desktop bilingue con elaborazione asincrona

- Priorità: P1
- Dipendenze: M1-01
- Responsabile: Qt Engineer / UX Designer
- Competenze: Qt6, accessibilità, concorrenza
- Deliverable: GUI con import, parametri, preview e log
- Test: Build, smoke offscreen, revisione UI
- Accettazione: GUI fluida, lingua e tema persistenti
- Stato: verificato (build Qt 6.4, smoke offscreen; revisione IT/scuro ed EN/chiaro, layout 960x680 e 1280x860)

## M2-02 — Profili, batch, code e ripresa

- Priorità: P1
- Dipendenze: M2-01
- Responsabile: Application Engineer
- Competenze: Job persistence, atomicità
- Deliverable: Workflow e profili versionati
- Test: Interruzione, ripresa e input modificato
- Accettazione: Nessun output parziale scambiato per completo
- Stato: verificato per batch headless sequenziale EN/IT, profili e ripresa con digest; coda GUI separata in M2-03

## M3-01 — Dataset reali e interoperabilità SatDump

- Priorità: P0
- Dipendenze: M1-02
- Responsabile: Satellite Imaging Specialist
- Competenze: MSU-GS, CBOR, SatDump
- Deliverable: Dataset con provenienza e report
- Test: Confronto pixel e apertura in SatDump
- Accettazione: Precisione e metadati verificati su acquisizioni reali
- Stato: verificato in parte su dati reali — campione Elektro-L4 disponibile nella release `rdas-cadu-l4`, dimensioni/SHA256 fissati; decodifica SatDump autentica, calibrazione ECC a scala esplicita, generazione 16bit a risoluzione originale, confronto pixel esatto con riferimento OpenCV4.6 e riapertura headless dei due prodotti SatDump riusciti (`docs/REAL_RDAS.md`, [run42514688645](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688645), commitde60721). Restano aperte precisione rispetto a verità a terra, geolocalizzazione assoluta e ROI di merge: il campione non contiene proiezione/calibrazione utilizzabili. Nessuna calibrazione universale L4 dichiarata.

## M3-02 — Packaging e matrice piattaforme

- Priorità: P1
- Dipendenze: M2-01
- Responsabile: Linux Systems / Release Engineer
- Competenze: deb, rpm, AppImage, ARM64
- Deliverable: Pacchetti e build riproducibili
- Test: Ubuntu 22.04/24.04/26.04; smoke pacchetti
- Accettazione: Solo piattaforme effettivamente testate dichiarate
- Stato: verificato per la matrice Linux dichiarata — Ubuntu22.04/24.04/26.04 x86_64, ARM64 Ubuntu24.04 nativo, Fedora44 RPM in userspace pulito e VM KVM con kernel proprio6.19.10-300.fc44.x86_64, AppImage x86_64 in userspace Ubuntu22.04 pulito. Installazione/esecuzione CLI/GUI/Natural Color, equivalenza e disinstallazione con input preservati verificate nuovamente su `b86103f`, inclusa demo reale installata: [VM42577236705](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236705), [piattaforme42577236864](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236864), [AppImage42577236837](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236837). Non esteso a sessione grafica interattiva/FUSE/macOS o distro non testate. Riepilogo in [docs/TEST_RESULTS.md](docs/TEST_RESULTS.md).

Installer Ubuntu: `install.sh` con dipendenze complete, attesa APT, stop alla prima failure, installazione utente e launcher senza percorsi manuali. Log/errori locali esclusi da git e archivi. Verifica effettiva Ubuntu26.04: [run42519134799](https://github.com/chiaraberti13/ORBYX/actions/runs/42519134799), successo (`docs/INSTALLATION.md`).

## M3-03 — Prestazioni, GPU e fuzzing

- Priorità: P1
- Dipendenze: M1-02
- Responsabile: GPU Engineer / Security Auditor
- Competenze: OpenCL, ASan, fuzz, benchmark
- Deliverable: Benchmark CPU/GPU e harness
- Test: Corpora invalidi e confronto CPU/GPU
- Accettazione: Nessuna regressione numerica o vulnerabilità nota ignorata
- Stato: verificato in parte — harness libFuzzer `tests/fuzz_parsers.cpp` sulla superficie di input non fidata (`inspect`/PNG e `translate`) con `-fsanitize=fuzzer,address,undefined`, opzione `ORBYX_FUZZ` (solo Clang), gruppo CTest `fuzz-parsers-smoke` e job CI a tempo limitato (`docs/FUZZING.md`); 20000 run locali senza findings. Benchmark CPU reale misurato (`scripts/orbyx_benchmark.py`, `docs/PERFORMANCE.md`): generate ~81 ms, calibrate ~32 s su 1,54 Mpx (ECC dominante). Profili JSON e percorso run/generate ora inclusi nell’harness con corpus versionato e dizionario; replay locale e campagna sanitizer estesa superati in CI (104219run/61s nel run42520331102; dettagli/limiti in docs/FUZZING.md). Equivalenza/benchmark GPU-OpenCL richiedono ancora hardware GPU reale. Aggiunto gate reale `orbyx_gpu_verify`: kernel esplicito su dispositivo GPU, uscita77/Skipped quando assente; nessuna prova hardware locale superata. ECC dichiarato CPU, non GPU (`docs/GPU_REAL.md`).

## M4-01 — Accessibilità, onboarding e calibrazione guidata

- Priorità: P2
- Dipendenze: M2-02
- Responsabile: Accessibility / Interaction Designer
- Competenze: Keyboard, screen reader, UX
- Deliverable: Flussi guidati e componenti rifiniti
- Test: Test tastiera e utenti
- Accettazione: Flussi completi EN/IT
- Stato: verificato in parte — guida iniziale passo passo EN/IT, installer e launcher automatici, demo reale nel desktop con i sei JPEG forniti, registrazione ECC e immagine unica visualizzata; la simulazione scientifica resta un test distinto; base accessibile presente: i nuovi widget della coda e del profilo hanno nomi accessibili EN/IT e scorciatoie da tastiera coerenti col resto della GUI. Testati click e ripresa da tastiera EN/IT, dialoghi profili e demo anche con finestra 960×680 e controlli realmente nel viewport. Restano da completare wizard interattivo e prove con screen reader/utenti reali

## M4-02 — Aggiornamenti sicuri e manutenzione periodica

- Priorità: P2
- Dipendenze: M3-02
- Responsabile: DevSecOps / AppSec
- Competenze: Firma release, dipendenze
- Deliverable: Release firmate e procedura aggiornamenti
- Test: Verifica firma, rollback, scansioni
- Accettazione: Nessun aggiornamento eseguito senza verifica
- Stato: verificato in parte — checksum SHA-256 riproducibili generati e verificati (`sha256sum -c`) in CI su tutti gli artifact (DEB, RPM, sorgenti, SBOM) come `SHA256SUMS`; meccanica di **firma/verifica** GPG (`scripts/orbyx_sign.py`) provata dal gruppo CTest `sign-regression` con chiave effimera (firma, verifica, rilevamento manomissione). Attestazione autentica Sigstore/GitHub OIDC senza secret personale verificata sul commit0d7bf19 (run42501765172), inclusi vincoli del firmatario e rifiuto di artifact manomessi (`docs/RELEASE_IDENTITY.md`). Restano aperti firma GPG personale e aggiornamento/rollback firmato; OIDC certifica il workflow, non la persona.


## M2-03 — Coda desktop e gestione dei profili

- Priorità: P1
- Dipendenze: M2-02
- Responsabile: Qt Engineer / Application Engineer
- Competenze: Qt models, job states, UX
- Deliverable: coda GUI e creazione/duplicazione profili
- Test: interruzione, ripresa, modifica input, localizzazione
- Accettazione: stato durevole visibile nel desktop e nessuna perdita dati
- Stato: verificato in parte — coda desktop che delega al runner batch già verificato (stato durevole, ripresa con digest, nessuna sovrascrittura), gruppo coda e pulsanti visibili, duplicazione profili nel core (`orbyx::duplicateProfile`) con dialog GUI. Verificati da test automatici: esecuzione coda (`gui-queue-flow`: prodotti identici alla CLI e ripresa) e duplicazione profili (`engine-regression`); i widget visibili compilano e si avviano (`gui-smoke`, screenshot ispezionato). Verificati anche i click EN/IT, aggiunta/rimozione, ripresa da tastiera e recupero FailedToStart con `gui-widget-flow`; verificata la duplicazione tramite i dialoghi reali con conservazione dell’originale; resta lo screen reader (M4-01)

## M2-04 — Monitoraggio cartelle e riconoscimento dati

- Priorità: P2
- Dipendenze: M3-01, M2-02
- Responsabile: Satellite Imaging Specialist / Application Engineer
- Competenze: SatDump metadata, file stabilization, concurrency
- Deliverable: rilevamento affidabile e import automatico
- Test: acquisizione incompleta, scritture concorrenti e metadati ambigui
- Accettazione: nessun job avviato su prodotti incompleti, controllo manuale preservato
- Stato: verificato in parte — riconoscimento prodotti RDAS `scripts/orbyx_detect.py`: classifica completo/incompleto/instabile/non-prodotto (struttura MSUGS_VIS1/2 + stabilità delle dimensioni), rifiuta symlink/file vuoti, scansione di sola lettura che **elenca** i prodotti pronti senza avviare nulla né toccare i dati (controllo manuale preservato); gruppo CTest `detect-regression` su dataset simulati completi/incompleti/instabili. Verificata anche scansione strutturale/stabilità del campione reale ridotto `assets/sample-elektro-l4`, con9file conservati byte per byte; non valida i metadati né rende la preview un prodotto nativo elaborabile. Import automatico e disambiguazione di metadati SatDump reali/ambigui restano aperti (dipendono da M3-01, dati reali)

## M3-04 — Natural Color validato

- Priorità: P2
- Dipendenze: M3-01
- Responsabile: Computer Vision Engineer
- Competenze: 16-bit radiometry, color management
- Deliverable: composizione e conversione 8/16 bit documentata
- Test: scene reali e fixture cromatiche
- Accettazione: nessuna conversione implicita o perdita dei canali originali
- Stato: verificato per resa visiva Natural Color — core/CLI e worker desktop EN/IT, RGB321 MSU-GS con regole hue legacy in floating point, 8/16bit senza quantizzazione intermedia a8bit, originali immutati e PNG atomico esclusivo. Fixture cromatiche con oracolo scalare indipendente, limiti delle strisce e bit bassi16bit verificati; CLI e GUI producono gli stessi byte. Due scene VIS reali Elektro-L4 esportate a6004×13594×3 uint16: oltre25000campioni confrontati con colorsys, scarto≤0,48 livelli, confini hue conteggiati separatamente; scena ispezionata. Non è colore vero calibrato radiometricamente e non corregge geometria/artefatti sorgenti (`docs/NATURAL_COLOR.md`). Verifica remota riuscita: [RDAS/Natural Color42516055833](https://github.com/chiaraberti13/ORBYX/actions/runs/42516055833), commit a531cec; regressioni delle piattaforme successive verdi anche su9a52e07.

## M1-03 — Inventario dipendenze, SBOM e scansione vulnerabilità runtime

- Priorità: P1
- Dipendenze: M1-02
- Responsabile: DevSecOps / Open Source Licensing Specialist
- Competenze: SBOM, distro advisories, codec licences
- Deliverable: inventario esatto dei binari e scansione versionata
- Test: corrispondenza librerie caricate/SBOM e vulnerabilità campione
- Accettazione: scansione riproducibile dei pacchetti runtime, oltre a Dependabot Actions
- Stato: verificato in parte — generatore deterministico `scripts/orbyx_sbom.py` (soname→percorso→pacchetto→versione su Ubuntu via readelf/ldd/dpkg) con export CycloneDX 1.5 e SPDX 2.3 (`--format`), gruppo CTest `sbom-regression` con integrazione sui binari reali e artifact inventario+CycloneDX+SPDX per-OS in CI (`docs/SBOM.md`). Resta aperta solo la correlazione automatica con un feed di vulnerabilità (passo documentato, non eseguito offline, richiede il feed della distribuzione).
