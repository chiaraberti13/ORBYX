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
