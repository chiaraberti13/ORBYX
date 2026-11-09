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
- Stato: verificato — Ubuntu22.04/24.04/26.04, suite core/CLI/GUI, JPEG reali, Natural Color, ASan/UBSan/leak detection, Cppcheck, fuzzing e CodeQL. Sul codice conclusivo `4ea5019`:23gruppi nei build GUI,22eseguiti e gate GPU saltato; sanitizers18gruppi,17eseguiti e GPU saltata, con `detect_leaks=1`. Test di wizard, importazione, aggiornamenti, persistenza coda/profili e demo EN/IT inclusi. Prove e limiti in [docs/TEST_RESULTS.md](docs/TEST_RESULTS.md). Nessun pass GPU implicato.

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
- Stato: verificato in parte — guida iniziale passo passo EN/IT, installer e launcher automatici, demo reale nel desktop con i sei JPEG forniti, registrazione ECC e immagine unica visualizzata; la simulazione scientifica resta un test distinto; base accessibile presente: i nuovi widget della coda e del profilo hanno nomi accessibili EN/IT e scorciatoie da tastiera coerenti col resto della GUI. Testati click e ripresa da tastiera EN/IT, dialoghi profili e demo anche con finestra 960×680 e controlli realmente nel viewport. Wizard interattivo EN/IT implementato a tre passi, con scala ECC, revisione e passaggio al nuovo profilo/allineamento; restano le prove con screen reader/utenti reali

## M4-02 — Aggiornamenti sicuri e manutenzione periodica

- Priorità: P2
- Dipendenze: M3-02
- Responsabile: DevSecOps / AppSec
- Competenze: Firma release, dipendenze
- Deliverable: Release firmate e procedura aggiornamenti
- Test: Verifica firma, rollback, scansioni
- Accettazione: Nessun aggiornamento eseguito senza verifica
- Stato: verificato in parte — checksum SHA-256 riproducibili generati e verificati (`sha256sum -c`) in CI su tutti gli artifact (DEB, RPM, sorgenti, SBOM) come `SHA256SUMS`; meccanica di **firma/verifica** GPG (`scripts/orbyx_sign.py`) provata dal gruppo CTest `sign-regression` con chiave effimera (firma, verifica, rilevamento manomissione). Attestazione autentica Sigstore/GitHub OIDC senza secret personale verificata sul commit0d7bf19 (run42501765172), inclusi vincoli del firmatario e rifiuto di artifact manomessi (`docs/RELEASE_IDENTITY.md`). Aggiornamento nativo utente e rollback implementati con verifica Sigstore/GitHub OIDC vincolata a repository/workflow/main/commit approvato: snapshot privati, verifica prima di estrazione/esecuzione, piattaforma esatta, probe CLI/GUI, attivazione atomica, conservazione release precedenti e ricostruzione da archivio riverificato al rollback. Interfaccia EN/IT e `bash update.sh`; test filesystem/transaction con verifier iniettato e prova crittografica autentica nella workflow di attestazione Ubuntu22/24/26. Nessun aggiornamento automatico non verificato. Firma GPG personale resta una risorsa esterna; OIDC certifica il workflow, non la persona.


## M2-03 — Coda desktop e gestione dei profili

- Priorità: P1
- Dipendenze: M2-02
- Responsabile: Qt Engineer / Application Engineer
- Competenze: Qt models, job states, UX
- Deliverable: coda GUI e creazione/duplicazione profili
- Test: interruzione, ripresa, modifica input, localizzazione
- Accettazione: stato durevole visibile nel desktop e nessuna perdita dati
- Stato: verificato per le funzionalità software dichiarate — coda desktop che delega al runner batch già verificato (stato durevole, ripresa con digest, nessuna sovrascrittura), gruppo coda e pulsanti visibili, duplicazione profili nel core (`orbyx::duplicateProfile`) con dialog GUI. Verificati da test automatici: esecuzione coda (`gui-queue-flow`: prodotti identici alla CLI e ripresa) e duplicazione profili (`engine-regression`); i widget visibili compilano e si avviano (`gui-smoke`, screenshot ispezionato). Verificati anche i click EN/IT, aggiunta/rimozione, ripresa da tastiera e recupero FailedToStart con `gui-widget-flow`; verificata la duplicazione tramite i dialoghi reali con conservazione dell’originale; aggiunto salvataggio atomico immediato di aggiunte/rimozioni/stati e ripristino della coda alla riapertura, senza leggere le acquisizioni; le vecchie conclusioni sono mostrate Da verificare finché il batch non riconferma i digest. ID globalmente univoci e revisioni con lock impediscono sovrascritture silenziose fra istanze. Resta la verifica con screen reader reale (M4-01)

## M2-04 — Monitoraggio cartelle e riconoscimento dati

- Priorità: P2
- Dipendenze: M3-01, M2-02
- Responsabile: Satellite Imaging Specialist / Application Engineer
- Competenze: SatDump metadata, file stabilization, concurrency
- Deliverable: rilevamento affidabile e import automatico
- Test: acquisizione incompleta, scritture concorrenti e metadati ambigui
- Accettazione: nessun job avviato su prodotti incompleti, controllo manuale preservato
- Stato: implementato e verificato per il formato MSU-GS VIS supportato — monitor desktop non modale EN/IT, scansione asincrona, confronto inode/dimensione/mtime/ctime, verifica header/CRC/IEND PNG nativi e parsing CBOR limitato con rifiuto di duplicati/metadati incompatibili. Importazione manuale e accodamento automatico opt-in solo per satellite corrispondente; nessun avvio automatico dei job. Deduplica dei rilevamenti e nuova verifica del fingerprint prima dell’esecuzione batch. Metadati mancanti/ambigui e preview vengono trattenuti, non indovinati. Test Python su formati invalidi/scritture concorrenti e flusso reale dei widget con processo di ispezione, importazione manuale, opt-in e deduplica. I limiti di geolocalizzazione rimangono in M3-01.

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
- Stato: verificato per i pacchetti runtime Ubuntu supportati — generatore deterministico `scripts/orbyx_sbom.py` (soname→percorso→pacchetto→versione su Ubuntu via readelf/ldd/dpkg) con export CycloneDX 1.5 e SPDX 2.3 (`--format`), gruppo CTest `sbom-regression` con integrazione sui binari reali e artifact inventario+CycloneDX+SPDX per-OS in CI (`docs/SBOM.md`). Correlazione Ubuntu OSV implementata: risoluzione esatta pacchetto binario→sorgente/versione tramite dpkg, query batch paginate, snapshot con digest e replay offline, copertura incompleta esplicita. Comando guidato `bash security.sh` e workflow periodica con report. Fixture reale Ubuntu vulnerabile/corretta e test di paginazione/manomissione/risoluzione verificati. Nessuna equivalenza con librerie custom/plugin non inventariati o distribuzioni non supportate.

## Chiusura dello sviluppo software — 9 novembre 2026

Le funzioni precedentemente mancanti sono implementate sul commit `4ea5019`: wizard, importazione/monitoraggio nativo, correlazione OSV, aggiornamento/rollback autenticato e persistenza della coda. Tutte le sette pipeline applicabili sono verdi, inclusi installer Ubuntu26.04 e aggiornamenti OIDC reali su Ubuntu22/24/26. [Esiti conclusivi](docs/TEST_RESULTS.md#verifica-conclusiva-del-codice-4ea5019).

Gli stati parziali rimasti riguardano prove esterne: hardware GPU, riferimento geografico certificato, screen reader/utenti reali e chiave GPG personale. Non sono sostituiti da simulazioni o dichiarati superati. La matrice Linux e i formati documentati delimitano il supporto; non vengono estesi a piattaforme o sensori non verificati.

## M2-05 — Compositore manuale multi-porzione e colore

- Priorità: P1, nuova esigenza utente successiva alla chiusura dello sviluppo precedente
- Dipendenze: M2-01, M3-04
- Responsabile: Computer Vision / Qt / UX / QA / AppSec
- Deliverable: editor non distruttivo a livelli, numero/posizione liberi, colore integrato, progetto e immagine unica
- Accettazione: niente vincolo SX/DX; nessun secondo programma necessario per comporre i prodotti supportati; originali intatti, PNG8/16bit e progetto riapribile
- Stato: verificato per l’ambito dichiarato su `fa06d80` — massimo32porzioni PNG in grigio/RGB8/16bit, composizione da tre canali allineati, trasformazioni manuali, crop, ordine/opacità/sfumatura, livelli/gamma/EV/saturazione/bilanciamento, annulla/ripeti e progetto JSON. Motore condiviso preview/export, limiti risorse ed export atomico esclusivo. Test core e widget EN/IT passati, compresa demo reale modificabile e apertura dal pulsante principale. Tutte le sette workflow finali verdi: Ubuntu22/24/26, sanitizer/fuzz/static, Fedora/ARM64, VM, AppImage, OIDC e RDAS autentico; 25gruppi desktop (24eseguiti, GPU skipped), 19sanitizers (18eseguiti, GPU skipped). [Prove](docs/TEST_RESULTS.md#nuova-esigenza-m2-05-compositore-multi-porzione). Non include un decoder grezzo, riconoscimento automatico delle porzioni o funzioni GIMP complete. [Guida e limiti](docs/COMPOSITOR.md).

## M2-06 — Proposta automatica delle sovrapposizioni

- Priorità: P1, estensione richiesta dopo M2-05
- Dipendenze: M2-05
- Responsabile: Computer Vision / Qt / UX / QA / AppSec
- Deliverable: individuazione visuale delle aree comuni fra porzioni generiche e proposta modificabile di posizione, rotazione e scala
- Accettazione: nessun vincolo SX/DX; riferimento scelto dall’utente; evidenze e conferma prima di applicare; porzioni non affidabili invariate; risultato annullabile e correggibile manualmente
- Stato: verificato per l’ambito dichiarato su `338675d` — motore ORB con corrispondenze reciproche, stima robusta e soglie su inlier, sovrapposizione, errore e confidenza; concatenamento di più porzioni, conferma EN/IT, annullamento senza modifiche, annulla/ripeti e log. Suite locale applicabile24/24, Cppcheck pulito e tutte le pipeline finali verdi; desktop25gruppi (24eseguiti, GPU skipped), sanitizers19gruppi (18eseguiti, GPU skipped). Non è georeferenziazione, non risolve prospettiva o proiezioni diverse e non sostituisce il controllo delle giunzioni. [Prove](docs/TEST_RESULTS.md#estensione-m2-06-sovrapposizione-automatica-modificabile) e [guida](docs/COMPOSITOR.md).

## Evoluzione proposta del compositore — analisi multidisciplinare

Questa sezione contiene proposte, non funzioni dichiarate implementate. L’analisi
considera il percorso reale dell’utente: immagini già esportate da SatDump,
allineamento dei canali, unione delle porzioni, controllo della giunzione, colore
e immagine finale senza ricorrere a un secondo editor.

### Situazione attuale e vincoli da preservare

- La logica storica non è stata rimossa: `generate` usa ancora le ROI
  `SATVIS1`/`SATVIS2` del profilo, ritaglia i sei canali allineati e crea il
  prodotto `MSUGS_VIS_Aligned`. Se una coppia completa di ROI non esiste, salva
  correttamente i due VIS separati.
- Il compositore aggiunto in M2-05/M2-06 è un percorso distinto: accetta porzioni
  generiche, propone la geometria da dettagli visuali e permette correzioni
  manuali. Oggi non importa automaticamente ROI, trasformazioni e risultati del
  normale flusso SatDump.
- L’unione ROI è deterministica e deve rimanere il comportamento compatibile per
  profili già verificati. Il matching visuale è un fallback o una scelta
  esplicita, non un sostituto silenzioso.
- Il prodotto scientifico e l’immagine di presentazione devono restare distinti:
  sui canali destinati a SatDump niente compensazione cromatica o blending che
  alteri i valori senza consenso; sull’immagine finale visuale sono ammesse
  correzioni non distruttive dichiarate.
- Nessuna modalità visuale diventa georeferenziazione. Coordinate assolute o
  GeoTIFF richiedono metadati/GCP e CRS realmente disponibili.

Il confronto tecnico conferma una pipeline modulare: OpenCV separa matching,
stima affine, warping, ricerca della seam, compensazione esposizione e blending;
per immagini assimilabili a scansioni indica il modello affine. Hugin affianca
punti di controllo manuali all’automatismo e usa maschere include/escludi;
Enblend ottimizza la seam e supporta immagini 8/16/32 bit. Sono riferimenti di
progettazione, non dipendenze da integrare automaticamente.

## M2-07 — Coordinatore delle strategie di unione

- Priorità: P0
- Stato: in corso — primo incremento implementato e verificato: la GUI conserva
  il risultato ROI storico e rende disponibili le due porzioni allineate come
  livelli modificabili, inizializzati con gli stessi ritagli e la stessa
  posizione. Il percorso CLI predefinito e i pixel storici restano invariati.
  Restano da unificare nel coordinatore la scelta esplicita tra ROI/visuale,
  l'anteprima comparativa e i casi assistiti con più di due porzioni.
- Dipendenze: M2-01, M2-05, M2-06
- Responsabile: Technical Lead / Computer Vision / Maintainer compatibilità / QA
- Deliverable: un solo servizio core che espone quattro strategie: **Profilo
  ROI**, **Visuale automatica**, **Assistita** e **Manuale**
- Comportamento automatico: valida prima satellite, profilo e ROI; se la coppia
  ROI è completa propone la logica storica come scelta raccomandata; se manca o
  viene rifiutata propone il matching visuale; se la confidenza è insufficiente
  apre l’assistenza manuale senza inventare una posizione
- Compatibilità: CLI `generate`, nomi dei prodotti, CBOR, profondità e pixel del
  percorso ROI devono rimanere identici alla baseline verificata
- Revisione: anteprima comparabile **ROI / Visuale** quando entrambe sono
  disponibili; nessuna scelta applicata o esportata senza conferma
- Test: golden test byte-per-byte del percorso ROI; ROI mancanti/parziali/non
  valide; matching riuscito/fallito; più di due porzioni; input 8/16 bit;
  originali e CBOR immutati
- Accettazione: la nuova architettura non modifica alcun risultato storico e
  rende esplicito nel progetto quale strategia e quali parametri sono stati usati

## M2-08 — Procedura guidata “Da SatDump all’immagine finale”

- Priorità: P0
- Stato: in corso — primo incremento implementato e verificato: una sola azione
  desktop guida **Importa**, **Allinea canali**, **Unisci porzioni**, **Colore**
  ed **Esporta**, valida origine/profilo/destinazione, avvia il percorso reale e
  apre automaticamente il compositore senza cercare file intermedi. Restano da
  integrare la coda/ripresa durevole nello stesso wizard, il riconoscimento di
  gruppi generici oltre ai due VIS RDAS e il salvataggio dello stato dei singoli
  passaggi.
- Dipendenze: M2-07, M3-04
- Responsabile: Product Designer / Qt UX / SatDump Specialist / Accessibility / QA
- Deliverable: procedura EN/IT accessibile dal desktop con percorso base in
  cinque passaggi: **Importa**, **Allinea canali**, **Unisci porzioni**,
  **Colore**, **Esporta**
- Importazione: riconosce cartelle `MSUGS_VIS*`, gruppi di tre canali e prodotti
  già allineati; mostra cosa è stato trovato e permette di correggere il
  raggruppamento senza dipendere dai nomi SX/DX
- Semplicità: un’azione principale **Crea immagine finale**; le opzioni tecniche
  restano in **Impostazioni avanzate** e non bloccano il percorso consigliato
- Continuità: trasferisce automaticamente canali allineati, ROI o proposta
  visuale nel compositore; l’utente non deve cercare cartelle intermedie né
  riaprire manualmente i file prodotti
- Sicurezza: destinazione sempre nuova, originali intatti, riepilogo prima
  dell’avvio, annullamento controllato e log apribile dall’interfaccia
- Ripresa: salva un progetto modificabile e riprende dal passaggio completato
  senza considerare affidabili risultati con digest cambiato
- Accettazione: un principiante deve ottenere una singola immagine PNG 8/16 bit
  partendo da sei immagini SatDump, senza terminale e senza software esterno

## M2-09 — Controllo professionale dell’allineamento

- Priorità: P0
- Stato: proposto, non implementato
- Dipendenze: M2-07, M2-08
- Responsabile: Computer Vision / Scientific Imaging UX / Qt / QA
- Deliverable: strumenti mirati alla giunzione, senza trasformare ORBYX in un
  editor grafico generico
- Ispezione: zoom dedicato sulla seam, sovrapposizione con opacità, lampeggio,
  tendina prima/dopo, scacchiera e mappa della differenza; selezione rapida della
  coppia di porzioni da confrontare
- Assistenza manuale: coppie di punti di controllo aggiungibili/spostabili,
  ricalcolo robusto della trasformazione, visualizzazione residui e rimozione
  degli outlier; trascinamento e valori numerici esistenti restano disponibili
- Preparazione: ritaglio automatico proposto per intestazioni, rampe e grandi
  bordi uniformi, sempre reversibile; possibilità di escludere tali aree dal
  matching senza cancellare pixel sorgente
- Diagnostica: numero di corrispondenze/inlier, copertura spaziale, errore
  mediano/massimo e avviso quando i punti sono concentrati in un’unica zona
- Accettazione: l’utente può riconoscere e correggere una doppia costa o una
  giunzione spostata senza uscire da ORBYX; il risultato resta annullabile

## M2-10 — Seam, maschere e blending per l’immagine visuale

- Priorità: P1
- Stato: proposto, non implementato
- Dipendenze: M2-09
- Responsabile: Computer Vision / Imaging / Performance / AppSec / QA
- Deliverable: tre modalità dichiarate: **Nessun blending** per dati scientifici,
  **Sfumatura** compatibile con l’editor attuale e **Seam ottimizzata** per la
  presentazione finale
- Seam: valutare graph-cut su colore/gradiente e blending multibanda usando i
  componenti OpenCV, con fallback deterministico alla sfumatura esistente
- Maschere: poligoni include/escludi non distruttivi per artefatti, bordi o zone
  da preferire; salvataggio nel progetto e anteprima della seam scelta
- Aspetto: compensazione opzionale di luminosità/colore soltanto sull’output
  visuale; mai sui canali SatDump o sul prodotto scientifico senza scelta
  esplicita e tracciata
- Prestazioni: preview a risoluzione ridotta, export a profondità nativa, tile e
  budget memoria; annullamento cooperativo delle elaborazioni lunghe
- Test: seam sintetiche note, bordi/maschere, scene 8/16 bit, differenze di
  luminosità, input senza sovrapposizione, determinismo e consumo massimo
- Accettazione: la modalità visuale riduce le discontinuità senza buchi o pixel
  non inizializzati; la modalità scientifica conserva esattamente i valori

## M2-11 — Colore integrato e controllato

- Priorità: P1
- Stato: proposto, non implementato
- Dipendenze: M2-08, M2-10, M3-04
- Responsabile: Remote Sensing / Color Science / UX / QA
- Deliverable: composizione colore nello stesso progetto e nella stessa
  anteprima dell’unione
- Preset: **Natural Color MSU-GS RGB321**, **RGB123** e canale singolo, con nomi
  chiari; nessun preset definito “colore vero scientifico” senza calibrazione
  radiometrica
- Ordine elaborazioni: formalizzare e testare allineamento canali → unione con
  la medesima geometria per i tre canali → composizione colore → correzioni
  visuali; evitare trasformazioni diverse fra R/G/B e bordi colorati
- Controlli: istogramma, clipping alte luci/ombre e prima/dopo; livelli, gamma,
  esposizione, saturazione e bilanciamento già esistenti diventano non distruttivi
  e riproducibili nel progetto
- Export: PNG 8/16 bit, profilo colore dichiarato e report dei passaggi; nessuna
  conversione silenziosa da 16 a 8 bit
- Accettazione: dalle sei immagini dell’acquisizione l’utente ottiene direttamente
  una singola immagine a colori e può tornare a ogni fase senza ricominciare

## M2-12 — Valutazione qualità e scelta consigliata

- Priorità: P1
- Stato: proposto, non implementato
- Dipendenze: M2-07, M2-09, M2-10, M2-11
- Responsabile: QA Lead / Computer Vision Validation / Remote Sensing / UX Research
- Deliverable: report comprensibile che confronta ROI, visuale e intervento
  manuale senza dichiarare automaticamente “perfetta” un’immagine
- Indicatori: errore geometrico sui punti, copertura dell’overlap, continuità dei
  bordi, differenza lungo la seam, aree vuote, clipping e trasformazioni estreme
- Raccomandazione: propone la strategia con evidenze e segnala i casi ambigui;
  l’utente può confrontare le alternative e scegliere, senza sostituzione
  automatica del progetto
- Dataset: fixture sintetiche con verità nota, demo reale dei sei JPEG, prodotto
  Elektro-L4 autentico e ulteriori acquisizioni con licenza/provenienza; includere
  porzioni verticali, irregolari, senza overlap e con nuvole mutate
- Usabilità: prova guidata con utenti non esperti; tastiera completa, nomi
  accessibili e successiva verifica con screen reader reale separata dalle prove
  automatiche
- Accettazione: criteri quantitativi superati sui dataset con verità nota e
  revisione umana registrata sulle scene reali; nessuna generalizzazione a
  satelliti, proiezioni o geometrie non provate

## M2-13 — Progetto portabile e recupero del lavoro

- Priorità: P2
- Stato: proposto, non implementato
- Dipendenze: M2-08
- Responsabile: Desktop Architecture / UX / AppSec / QA
- Deliverable: progetto con riferimenti relativi o pacchetto esplicito, autosave
  e ripristino dopo arresto anomalo
- Sicurezza: nessuna estrazione fuori destinazione, limiti a numero/dimensione
  degli asset, rifiuto symlink e percorsi ambigui, verifica digest delle sorgenti
- Accettazione: un progetto può essere spostato insieme alle immagini e riaperto
  senza perdere livelli, maschere, punti, strategia, colore e cronologia essenziale

### Ordine proposto e gate decisionali

1. **Compatibilità e percorso unico:** M2-07, poi M2-08. È il minimo necessario
   per rendere semplice il flusso senza perdere la logica originale.
2. **Controllo dell’utente:** M2-09. Va completato prima di aggiungere algoritmi
   che possano nascondere una giunzione geometricamente errata.
3. **Qualità visuale:** M2-10 e M2-11, mantenendo separato l’output scientifico.
4. **Validazione e robustezza:** M2-12; M2-13 può seguire senza bloccare il primo
   rilascio del nuovo flusso.

Nessuno dei task proposti è autorizzato allo sviluppo dalla sola presenza in
roadmap. Prima dell’implementazione occorre scegliere l’ampiezza del primo
incremento e conservare come gate obbligatorio la regressione byte-per-byte del
percorso ROI storico.

### Riferimenti tecnici valutati

- OpenCV stitching: <https://docs.opencv.org/4.13.0/d1/d46/group__stitching.html>
- OpenCV modalità affine/scans: <https://docs.opencv.org/4.13.0/d8/d19/tutorial_stitcher.html>
- Hugin, punti di controllo: <https://hugin.sourceforge.io/docs/manual/Control_points.html>
- Hugin, crop e maschere: <https://hugin.sourceforge.io/docs/manual/Hugin_Mask_tab.html>
- Enblend e seam: <https://hugin.sourceforge.io/docs/manual/Enblend.html>
- QGIS, distinzione dalla georeferenziazione con GCP/CRS: <https://docs.qgis.org/3.44/en/docs/user_manual/managing_data_source/georeferencer.html>
- SatDump, compositi: <https://docs.satdump.org/md_docs_2pages_2ImageProductExpression.html>
- SatDump, proiezioni: <https://docs.satdump.org/md_docs_2pages_2QuickStart.html>
