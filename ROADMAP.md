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
- Stato: verificato su Ubuntu 24.04 x86_64 con dataset sintetici; validazione dati reali M3-01 aperta

## M1-02 — Regressioni, sanitizers e CI

- Priorità: P0
- Dipendenze: M1-01
- Responsabile: QA / DevSecOps
- Competenze: CMake, CTest, Linux
- Deliverable: Suite, workflow e Dependabot
- Test: Build e CTest locali; CI remota
- Accettazione: Risultati registrati senza inventare compatibilità
- Stato: verificato — matrice Ubuntu 22.04/24.04/26.04, 7 gruppi CTest, sanitizers con leak detection e Cppcheck; CodeQL completato sul commit 4190002

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
- Stato: bloccato — dataset RDAS reali redistribuibili e ambiente SatDump non disponibili nella sessione

## M3-02 — Packaging e matrice piattaforme

- Priorità: P1
- Dipendenze: M2-01
- Responsabile: Linux Systems / Release Engineer
- Competenze: deb, rpm, AppImage, ARM64
- Deliverable: Pacchetti e build riproducibili
- Test: Ubuntu 22.04/24.04/26.04; smoke pacchetti
- Accettazione: Solo piattaforme effettivamente testate dichiarate
- Stato: in corso — CPack TGZ, installazione locale, DEB (CI) e ora RPM: generazione, metadati (licenza GPLv3, gruppo, URL) e requires automatici verificati con `rpmbuild` su Ubuntu e aggiunti alla CI (`docs/PACKAGING.md`). Installazione su distribuzione RPM reale (Fedora), AppImage, ARM64 e installazione su VM pulita ancora da verificare

## M3-03 — Prestazioni, GPU e fuzzing

- Priorità: P1
- Dipendenze: M1-02
- Responsabile: GPU Engineer / Security Auditor
- Competenze: OpenCL, ASan, fuzz, benchmark
- Deliverable: Benchmark CPU/GPU e harness
- Test: Corpora invalidi e confronto CPU/GPU
- Accettazione: Nessuna regressione numerica o vulnerabilità nota ignorata
- Stato: verificato in parte — harness libFuzzer `tests/fuzz_parsers.cpp` sulla superficie di input non fidata (`inspect`/PNG e `translate`) con `-fsanitize=fuzzer,address,undefined`, opzione `ORBYX_FUZZ` (solo Clang), gruppo CTest `fuzz-parsers-smoke` e job CI a tempo limitato (`docs/FUZZING.md`); 20000 run locali senza findings. Fuzzing del profilo JSON e della pipeline `run()`, corpus persistente e benchmark/equivalenza GPU-OpenCL restano aperti (richiedono hardware reale).

## M4-01 — Accessibilità, onboarding e calibrazione guidata

- Priorità: P2
- Dipendenze: M2-02
- Responsabile: Accessibility / Interaction Designer
- Competenze: Keyboard, screen reader, UX
- Deliverable: Flussi guidati e componenti rifiniti
- Test: Test tastiera e utenti
- Accettazione: Flussi completi EN/IT
- Stato: pianificato

## M4-02 — Aggiornamenti sicuri e manutenzione periodica

- Priorità: P2
- Dipendenze: M3-02
- Responsabile: DevSecOps / AppSec
- Competenze: Firma release, dipendenze
- Deliverable: Release firmate e procedura aggiornamenti
- Test: Verifica firma, rollback, scansioni
- Accettazione: Nessun aggiornamento eseguito senza verifica
- Stato: pianificato


## M2-03 — Coda desktop e gestione dei profili

- Priorità: P1
- Dipendenze: M2-02
- Responsabile: Qt Engineer / Application Engineer
- Competenze: Qt models, job states, UX
- Deliverable: coda GUI e creazione/duplicazione profili
- Test: interruzione, ripresa, modifica input, localizzazione
- Accettazione: stato durevole visibile nel desktop e nessuna perdita dati
- Stato: verificato in parte — coda desktop che delega al runner batch già verificato (stato durevole, ripresa con digest, nessuna sovrascrittura), gruppo coda e pulsanti visibili, duplicazione profili nel core (`orbyx::duplicateProfile`) con dialog GUI. Verificati da test automatici: esecuzione coda (`gui-queue-flow`: prodotti identici alla CLI e ripresa) e duplicazione profili (`engine-regression`); i widget visibili compilano e si avviano (`gui-smoke`, screenshot ispezionato). Resta da verificare l'interazione completa dei widget e lo screen reader (si lega a M4-01)

## M2-04 — Monitoraggio cartelle e riconoscimento dati

- Priorità: P2
- Dipendenze: M3-01, M2-02
- Responsabile: Satellite Imaging Specialist / Application Engineer
- Competenze: SatDump metadata, file stabilization, concurrency
- Deliverable: rilevamento affidabile e import automatico
- Test: acquisizione incompleta, scritture concorrenti e metadati ambigui
- Accettazione: nessun job avviato su prodotti incompleti, controllo manuale preservato
- Stato: pianificato

## M3-04 — Natural Color validato

- Priorità: P2
- Dipendenze: M3-01
- Responsabile: Computer Vision Engineer
- Competenze: 16-bit radiometry, color management
- Deliverable: composizione e conversione 8/16 bit documentata
- Test: scene reali e fixture cromatiche
- Accettazione: nessuna conversione implicita o perdita dei canali originali
- Stato: pianificato

## M1-03 — Inventario dipendenze, SBOM e scansione vulnerabilità runtime

- Priorità: P1
- Dipendenze: M1-02
- Responsabile: DevSecOps / Open Source Licensing Specialist
- Competenze: SBOM, distro advisories, codec licences
- Deliverable: inventario esatto dei binari e scansione versionata
- Test: corrispondenza librerie caricate/SBOM e vulnerabilità campione
- Accettazione: scansione riproducibile dei pacchetti runtime, oltre a Dependabot Actions
- Stato: verificato in parte — generatore deterministico `scripts/orbyx_sbom.py` (soname→percorso→pacchetto→versione su Ubuntu via readelf/ldd/dpkg) con export CycloneDX 1.5 e SPDX 2.3 (`--format`), gruppo CTest `sbom-regression` con integrazione sui binari reali e artifact inventario+CycloneDX+SPDX per-OS in CI (`docs/SBOM.md`). Resta aperta solo la correlazione automatica con un feed di vulnerabilità (passo documentato, non eseguito offline, richiede il feed della distribuzione).
