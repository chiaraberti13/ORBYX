# ORBYX — Avanzamento

## 2026-11-08

Repository inizialmente vuoto. Roadmap creata prima dello sviluppo. Upstream di riferimento: Digitelektro/rdas_layer_aligner, commit 6aee5b4ba96b528389890ea6442fc96a7344a6ff. Audit in corso. Nessuna compatibilità ancora verificata.

M0-01 e M0-02: audit statico completo e ADR-001 pubblicabili. Sorgenti originali e config conservati in third_party/rdas_layer_aligner; immagini dimostrative non importate. Push git HTTPS privo di credenziali: pubblicazione tramite connettore GitHub autorizzato, mantenendo main e controllo del parent.

M0-03: analizzati i README dei tre repository di riferimento. Banner ORBYX originale, README EN/IT, licenza GPLv3 preservata e avvisi di provenienza completati. XML del banner valido. Nessuna fotografia originale redistribuita.

M1-01: core C++17 e CLI EN/IT implementati. Build GCC 13.3 / OpenCV 4.6 / JsonCpp 1.9.5 su Ubuntu 24.04 x86_64. Suite sintetica: 8/16 bit, affine con traslazioni note, calibrazione ECC (<0,1 pixel), rigenerazione (differenza massima <=1), ROI, padding zero, CBOR distinti VIS1/VIS2, rollback, PNG/header/file grandi, JSON duplicati, dimensioni incoerenti e symlink. Cinque gruppi CTest normali passati (include smoke GUI preliminare).

ASan+UBSan: quattro gruppi headless passati con detect_leaks=0. LeakSanitizer non eseguibile nel runtime locale perché /proc/task non accessibile; il primo tentativo con detect_leaks=1 ha fallito per questa limitazione ambientale, non è una verifica leak superata. Le librerie distro non sono strumentate. Build e test usano dipendenze Ubuntu estratte localmente poiché l'installazione globale non è disponibile. Nessuna prova su GPU o dati reali.
