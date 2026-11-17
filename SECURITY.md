# Security / Sicurezza

Treat satellite images and calibration profiles as untrusted input. Run with ordinary user permissions, keep OpenCV, Qt and image-codec libraries updated, and use a dedicated output directory. ORBYX does not need network access or elevated privileges.

Report reproducible security defects through the repository's private GitHub vulnerability reporting facility when available. If unavailable, contact the repository owner privately using a verified channel; do not publish sensitive image data or exploit details in a public issue. Include revision, dependencies, minimal synthetic reproducer and observed impact. No response SLA is promised.

The archived upstream snapshot is reference material, not the supported runtime. Only `src/` is built by default. See docs/AUDIT.md for original findings and PROGRESS.md for verification status. Input limits reduce resource exhaustion but do not sandbox native image decoders. Real-data SatDump and GPU equivalence remain release gates.

Usare privilegi ordinari e dati di output separati. Per segnalazioni riservate usare il canale privato GitHub, se disponibile, oppure un contatto privato verificato del proprietario. Non allegare dati sensibili alle issue pubbliche.

## Percorso operativo di segnalazione

1. Consulta [la sezione sicurezza del repository](https://github.com/chiaraberti13/ORBYX/security) e usa la funzione di segnalazione privata se disponibile. La presenza di questo documento non implica che tale funzione sia abilitata.
2. Se la funzione non è disponibile, usa un canale privato già verificato con la manutentrice. In assenza di un contatto, chiedi soltanto l'attivazione di un canale riservato in una issue, senza dettagli tecnici, nomi di soggetti coinvolti o allegati sensibili. Se le issue sono disabilitate, usa un contatto verificato già disponibile; attendi un canale riservato prima di condividere dettagli.
3. Nel canale riservato indica versione o commit, componente, impatto, prerequisiti, riproduzione minima con dati sintetici ed eventuale mitigazione.
4. Concorda la pubblicazione dei dettagli dopo la valutazione e la correzione. Restano valide le eventuali tempistiche specifiche già indicate sopra; questa integrazione non introduce nuove garanzie sui tempi.

I bug ordinari e le richieste di accessibilità possono usare i modelli delle issue. Non allegare credenziali, dati di produzione o dati personali, neppure ai report tecnici quando non necessari.
