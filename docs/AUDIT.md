# Audit iniziale RDAS Layer Aligner

Data: 8 novembre 2026. Responsabili funzionali: Technical Lead, Computer Vision Engineer, Secure Code Auditor, Open Source Licensing Specialist.

## Provenienza e perimetro

Riferimento: [Digitelektro/rdas_layer_aligner](https://github.com/Digitelektro/rdas_layer_aligner), commit `6aee5b4ba96b528389890ea6442fc96a7344a6ff` (16 agosto 2026). Revisione statica di `main.cpp`, `parameterparser.*`, `configstorage.*`, `imageproc.*`, `CMakeLists.txt`, `README.md`, `LICENSE`, `config.json` e `kernels/tps.cl`. Questo documento descrive il codice upstream e non certifica l'assenza di vulnerabilità né la correttezza della derivazione ORBYX.

Il motore è C++17, OpenCV e JsonCpp, con OpenCL richiesto dal CMake Linux benché il percorso applicativo impieghi UMat, che può funzionare su CPU. Non esistono GUI Qt, suite di test, CI, packaging o aggiornamenti applicativi nel perimetro upstream esaminato. Il kernel TPS non risulta invocato dai sorgenti applicativi esaminati.

## Contratto funzionale da preservare

| Area | Comportamento upstream | Verifica necessaria nella derivazione |
|---|---|---|
| Input | Directory SatDump live con `MSUGS_VIS1` e `MSUGS_VIS2`, tre `MSUGS-VIS-N.png` ciascuna | PNG validi, dimensioni, profondità e canali coerenti; input immutati |
| Satelliti | Help L2/L3/L4/M1/M2; coefficienti JSON L2/L3/L5/M1/M2; parser accetta qualunque stringa non vuota | Preservare anche L5 presente nei dati; nessuna affermazione di coefficienti L4 già disponibili |
| Calibrazione | ECC affine, 500 iterazioni, epsilon 1e-7; pre-shift CH2=3606 e CH3=1804 | Dataset sintetico e reale, errore geometrico e regressione matrici |
| Generazione | Warp affine lineare con inverse-map, CH1 invariato, rimozione 3606 righe finali | Uguaglianza su regione valida e conservazione 8/16 bit |
| Merge | ROI sinistra/destra da config; fallback senza ROI | Bounds ROI, contenuto deterministico e corretta provenienza CBOR |
| Output | PNG e `product.cbor` per reimportazione SatDump | Verifica effettiva in SatDump, non solo copia byte |
| Colore naturale | Funzione HSV presente ma chiamata disabilitata esplicitamente nel main | Funzionalità nuova da validare, non descriverla come già funzionante upstream |
| CLI | `-s/--sat`, `-m/--mode`, `-d/--directory`, `-n/--nc` | Conservare sintassi; help e default devono essere resi effettivi |

## Risultati della revisione statica

Le priorità sono interne al progetto, non punteggi CVSS. Le evidenze richiedono test mirati prima di considerare chiusa ciascuna correzione.

| ID | Priorità | Evidenza upstream | Conseguenza e criterio di chiusura |
|---|---|---|---|
| A01 | Alta | `mergeLeftRight` crea UMat senza inizializzazione e riempie soltanto le ROI | Pixel residui indeterminati, potenziale esposizione di memoria: inizializzazione a zero o dimensioni esatte, test bordi |
| A02 | Alta | `alignChannels` usa `rows - 3606`, senza controlli; ROI JSON non validate | Assertion/crash e input malformati: rifiutare immagini piccole, ROI negative o fuori bounds prima del warp |
| A03 | Alta | ECC eseguito in `std::thread` senza cattura eccezioni | `std::terminate` su mancata convergenza: propagazione controllata, join garantiti, errore leggibile |
| A04 | Alta | Nessun limite esplicito a dimensioni PNG e allocazioni | Esaurimento memoria da immagini grandi: budget pixel/memoria e test input avversi; il decoder resta dipendenza da aggiornare |
| A05 | Media | Ramo senza ROI salva `VIS*_channels`, non `VIS*_alignedChannels` | Output apparentemente allineati ma originali: verificare pixel trasformati nel fallback |
| A06 | Media | Nel fallback VIS2, CBOR copiato da `VIS1_directory` | Metadati del prodotto errati: test con CBOR distinti per i due sensori |
| A07 | Media | `parseArgs` legge coppie e ignora argomento finale; default mode dichiarato ma non funzionante | Opzioni errate ignorate e help inutilizzabile: validazione opzioni, valori, duplicati e default; nessuna elaborazione su help/version |
| A08 | Media | Config JSON convertito senza schema, range o controllo finitezza | Matrici invalide e configurazione incoerente: validare tipi, coefficienti finiti e ROI prima dell'uso |
| A09 | Media | Config salvata per troncamento diretto; errore di scrittura solo stampato | Perdita configurazione e successo falso: sostituzione atomica locale, errore propagato e test fallimento I/O |
| A10 | Media | `imread` e `imwrite` non verificati, eccezioni main non gestite | Output parziali, crash: input mancanti/corrotti e filesystem non scrivibile devono produrre exit nonzero |
| A11 | Media | Output esistente sovrascritto, CBOR rimosso prima della copia | Interruzione può perdere output; definire esplicitamente politica overwrite e pubblicazione atomica |
| A12 | Bassa | `CMakeLists.txt` ricerca OpenCL opzionale, poi obbligatorio su Linux; OpenCV non REQUIRED | Build CPU inutilmente bloccata: OpenCV REQUIRED e backend OpenCL opzionale con test CPU |
| A13 | Bassa | Hue wrap corregge soltanto una volta ±180 | Shift ampio produce hue errato: range validati o modulo completo; funzione attualmente non esposta dal main |
| A14 | Bassa | TPS usa cast puntatori per reinterpretare float e aritmetica indici non validata | Kernel dormiente da non abilitare senza audit bounds e test OpenCL; preferire builtin `as_uint/as_float` |

Nessun processo shell, download o servizio di rete è invocato dal codice applicativo esaminato. Non è emersa una prova di injection o use-after-free nei percorsi letti; questo non sostituisce sanitizers e fuzzing. Le immagini, i JSON e il filesystem sono i principali confini di fiducia. Symlink, percorsi di output e concorrenza fra processi richiedono verifiche dedicate prima del monitoraggio cartelle e del batch.

## Licenza e distribuzione

Upstream include il testo GNU GPL versione 3. La semplice presenza, nella sezione esemplificativa finale della licenza, della formula "or any later version" non dimostra che upstream abbia selezionato GPL-3.0-or-later. Conservare il testo integrale e le attribuzioni, evitare una licenza più permissiva per il derivato e registrare origine e modifiche. Identificare ORBYX come derivazione indipendente, senza suggerire affiliazione.

La compatibilità legale dei binari dipende dalle versioni e dagli artefatti effettivamente distribuiti di OpenCV, JsonCpp, Qt, runtime OpenCL e relativi codec. Inventariare licenze, avvisi e sorgenti corrispondenti in fase di packaging; non dedurre la licenza delle dipendenze dal solo nome. Le immagini dimostrative upstream non sono necessarie al motore e non vanno riutilizzate senza verifica di provenienza.

## Strategia di verifica

Prima del refactor: preservare snapshot del codice/config originale e creare dataset sintetici riproducibili (identità, traslazione nota, assenza ROI, ROI non valida, immagini 8/16 bit, file corrotti). Mantenere una baseline algoritmica su regione valida: gli output affetti da A01/A05/A06 non devono diventare oracle di correttezza. Aggiungere dataset reali redistribuibili con consenso e metadati, mantenendo esplicito il limite finché mancanti.

Eseguire unit/integration test, AddressSanitizer e UndefinedBehaviorSanitizer, analisi statica e fuzzing parser/config. Misurare CPU/OpenCL su stesso dataset e tolleranze documentate; non promettere accelerazione GPU senza benchmark. Ubuntu 22.04/24.04/26.04, macOS e altre distribuzioni restano da verificare separatamente mediante build ed esecuzione, non dalla sola compilabilità teorica.
