# ADR-001 — Linguaggio del motore e confini della migrazione

Stato: decisione iniziale, riesaminabile dopo la baseline algoritmica. Data: 8 novembre 2026.

Responsabili: Technical Lead, Programming Languages Architect, Secure Code Auditor, Computer Vision Engineer, Linux Systems Engineer.

## Contesto

RDAS Layer Aligner usa C++17/OpenCV/JsonCpp. I rischi iniziali osservati riguardano input non validati, output indeterminato, eccezioni nei thread e I/O non affidabile. Il requisito primario è preservare ECC, trasformazioni, precisione e interoperabilità SatDump prima di una sostituzione del linguaggio.

## Confronto

| Opzione | Sicurezza e concorrenza | CPU/GPU e interoperabilità | Ubuntu/macOS e costo |
|---|---|---|---|
| C++17 con RAII e validazione | Nessuna memory safety garantita; controlli, ownership chiara, futures e sanitizers mitigano rischi concreti | Chiamate native OpenCV, UMat e OpenCL; nessun nuovo FFI | Minima variazione di toolchain e baseline; resta debito di sicurezza C++ |
| C++20 | `jthread`, stop token e span aiutano alcuni confini; non eliminano UB né validano automaticamente Mat/ROI | Integrazione OpenCV invariata | Valutabile con toolchain delle distro; migrazione standard da verificare nella matrice |
| C++23 | Ulteriori utility e gestione errori disponibili secondo toolchain; memory safety non garantita | Integrazione nativa invariata | Rischio aggiuntivo sui compilatori di Ubuntu 22.04, senza beneficio immediato misurato |
| Rust parziale | Ownership e Send/Sync proteggono il nuovo codice safe; FFI OpenCV e driver restano confini unsafe | Binding o bridge C++ necessari; duplicazioni di buffer e packaging da misurare | Due toolchain e due modelli errori; utile soltanto per un confine isolato con vantaggio dimostrato |
| Rust completo | Memory safety del codice safe ma non del backend nativo; nessuna garanzia automatica di precisione | Rischio di differenze algoritmiche e copertura binding; OpenCL richiede audit specifico | Riscrittura e validazione costose; alto rischio funzionale prima dei dataset |
| Python orchestrazione | Evita molti errori di gestione memoria nello script; decoder e librerie native restano esposti | Utile per dataset/test/automazione, non riscrittura dei loop pixel | Dipendenze e distribuzione aggiuntive; mantenere fuori dal core runtime se non necessario |

## Decisione

Mantenere inizialmente C++17 per il motore, correggere i difetti concreti e separare core, application, CLI, GUI e infrastructure. Usare Qt 6 solo nel livello GUI, lasciando il motore headless. Nessuna dipendenza Qt richiesta per la CLI.

Adottare C++20 soltanto con un vantaggio circoscritto (ad esempio cancellazione cooperativa dei job), test equivalenti e build sulle distro supportate; non selezionare C++23 come requisito iniziale. Non avviare una riscrittura Rust prima di dataset e benchmark. Valutare Rust successivamente per parsing/orchestrazione isolati se riduce un rischio misurato senza duplicare OpenCV. Python è ammesso per generazione dataset, verifiche e automazioni di sviluppo riproducibili.

OpenCL deve diventare opzionale, con CPU di riferimento. Il fatto che OpenCV esponga UMat non dimostra che ogni operazione venga accelerata. Registrare backend e versioni per rendere riproducibili benchmark e regressioni.

## Conseguenze e condizioni di riesame

Il progetto conserva integrazione nativa e costi contenuti ma richiede analisi statica, sanitizers, limiti sugli input e revisione dell'ownership. Migrare solo dopo confronto di errore geometrico, pixel nelle regioni valide, memoria di picco, throughput CPU/GPU, dimensione pacchetto e complessità di manutenzione. Le soglie vanno fissate nel task di baseline con dati reali e sintetici, evitando numeri arbitrari privi di misura.

Questa decisione non certifica compatibilità Ubuntu o macOS e non attesta prestazioni non misurate. Riferimenti e difetti iniziali sono registrati in [AUDIT.md](AUDIT.md).
