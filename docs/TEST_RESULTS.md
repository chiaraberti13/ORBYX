# Test verificati — 9 novembre 2026

Baseline dello sviluppo precedente: **4ea5019025d8707b1d2e629729caf020ac08712a**.
Ogni incremento è stato pubblicato direttamente su `main`. I risultati della
demo precedente rimangono riportati sotto; le nuove funzioni sono descritte
nella sezione successiva.

## Nuova esigenza M2-05: compositore multi-porzione

Codice: **fa06d806726349d805d570011916f2be9324a94a**. Incrementi pubblicati
su main: `ea30ebe` (motore), `4f307e8` (editor) e `fa06d80` (compatibilità Qt
recente e rendering delle sole aree occupate). Questa funzionalità è stata
richiesta dopo la chiusura dei task software precedenti.

Il nuovo pulsante **Componi porzioni e colore…** apre l’editor non distruttivo:
1–32porzioni in posizioni libere, senza vincolo SX/DX; composizione colore dai
canali allineati, drag e trasformazioni numeriche, crop, ordine, opacità,
sfumatura, livelli/gamma/esposizione/saturazione e guadagni RGB. Annulla/ripeti,
progetto riapribile, avviso modifiche non salvate, preview/zoom/Adatta e export
PNG unico8/16bit. I sei JPEG reali sono apribili come porzioni modificabili.
[Guida, formati e limiti](COMPOSITOR.md).

Tutte le sette esecuzioni seguenti sono concluse con esito **success**.

| Prova sul codice conclusivo del compositore | Esecuzione |
| --- | --- |
| Ubuntu22.04/24.04/26.04, CTest, sanitizers, fuzz e Cppcheck | [42615323472](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323472) |
| Fedora44 e ARM64 nativo | [42615323543](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323543) |
| Fedora44 in VM completa | [42615323571](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323571) |
| AppImage in ambiente pulito | [42615323468](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323468) |
| Provenienza e aggiornamenti/rollback OIDC reali | [42615323406](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323406) |
| CodeQL | [42615323424](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323424) |
| Acquisizione autentica Elektro-L4 e SatDump | [42615323626](https://github.com/chiaraberti13/ORBYX/actions/runs/42615323626) |

Nei build desktop:25gruppi CTest,24eseguiti e gate GPU saltato perché manca
l’hardware; sanitizers19gruppi,18eseguiti e GPU saltata, leak detection attiva.
`mosaic-regression` confronta porzioni su righe/colonne, origine negativa,
trasformazioni, opacità/sfumatura e colore con oracoli specifici, bit bassi16bit,
progetto round-trip e input invalidi. `mosaic-widget-flow` esercita l’editor
reale EN/IT con tre porzioni, drag, annulla/ripeti, progetto riaperto e export
confrontato pixel per pixel con il renderer nativo; compone anche Natural Color
16bit da tre PNG e apre i sei JPEG reali come livelli modificabili.
`gui-widget-flow` apre il compositore dal pulsante di produzione.

Il primo passaggio dell’editor falliva la compilazione del nuovo test su Qt
recente: `findChild` richiede Q_OBJECT nel tipo cercato. Corretto cercando il
QLabel nominato e controllando il tipo concreto; nessun test escluso.
Screenshot della demo modificabile ispezionato con intera scena nel viewport.
Nessuna prova di geolocalizzazione assoluta o valutazione universale delle
cuciture viene dedotta dai test di compositing. La decodifica dei grezzi rimane
una funzionalità distinta, non aggiunta dall’editor.

## Estensione M2-06: sovrapposizione automatica modificabile

Motore: **7d87b42936cdbe27959606c14fe9e39ed085aa1d**. Interfaccia, test e
guida conclusivi: **338675dc356fb260ae60e5076f42b8280ff2fd58**.

Il livello selezionato resta fermo come riferimento. Il motore cerca dettagli
visivi comuni, richiede corrispondenze reciproche e stima con RANSAC una
trasformazione limitata a traslazione, rotazione e scala uniforme. Inlier,
percentuale di area comune, errore mediano e confidenza devono superare soglie
esplicite. Le porzioni possono essere collegate in catena; quelle uniformi o
senza corrispondenza affidabile restano invariate.

L’interfaccia mostra le evidenze e richiede **Applica** o **Annulla**, con
annullamento predefinito. Una proposta applicata resta modificabile con tutti i
controlli manuali e con annulla/ripeti. Il test core usa tre ritagli concatenati,
verifica il recupero di rotazione/scala e rifiuta porzioni uniformi o senza area
comune. Il test widget EN/IT verifica il dialogo reale, le evidenze, annullamento
senza cambiamenti, applicazione esplicita, annulla/ripeti e salvataggio progetto.

Tutte le esecuzioni applicabili sono concluse con esito **success**.

| Prova conclusiva | Esecuzione |
| --- | --- |
| Ubuntu22.04/24.04/26.04, CTest, sanitizers, fuzz e Cppcheck | [42620629575](https://github.com/chiaraberti13/ORBYX/actions/runs/42620629575) |
| Fedora44 e ARM64 nativo | [42620629613](https://github.com/chiaraberti13/ORBYX/actions/runs/42620629613) |
| Fedora44 in VM completa | [42620629591](https://github.com/chiaraberti13/ORBYX/actions/runs/42620629591) |
| AppImage in ambiente pulito | [42620629586](https://github.com/chiaraberti13/ORBYX/actions/runs/42620629586) |
| Provenienza e aggiornamenti/rollback OIDC reali | [42620629514](https://github.com/chiaraberti13/ORBYX/actions/runs/42620629514) |
| CodeQL | [42620629494](https://github.com/chiaraberti13/ORBYX/actions/runs/42620629494) |
| Acquisizione autentica Elektro-L4 e SatDump sul motore invariato | [42620271984](https://github.com/chiaraberti13/ORBYX/actions/runs/42620271984) |

Nei build desktop:25gruppi CTest,24eseguiti e GPU fisica skipped; nei
sanitizers19gruppi,18eseguiti e GPU skipped. In locale sono passati24/24gruppi
applicabili e Cppcheck completo. Questi risultati non trasformano il matching
visivo in georeferenziazione, non provano prospettiva/proiezioni differenti e
non sostituiscono il controllo umano di ogni giunzione.

## Funzioni completate

- **Calibrazione guidata EN/IT**: tre passaggi, controllo degli input nativi,
  scala ECC esplicita, revisione e selezione del nuovo profilo dopo il successo.
- **Importazione SatDump e monitor cartelle**: verifica PNG/CBOR, acquisizioni
  incomplete visibili ma trattenute, selezione manuale e accodamento automatico
  facoltativo solo con metadati e profilo compatibili. Il batch ricontrolla il
  fingerprint prima di elaborare; nessun job parte automaticamente.
- **Coda desktop persistente**: salvataggio immediato, ripristino, migrazione,
  protezione fra finestre concorrenti. Le vecchie conclusioni diventano
  **Da verificare**, finché il batch non riconferma i digest.
- **Scansione vulnerabilità runtime Ubuntu**: SBOM binaria, pacchetti sorgente e
  versioni esatte, interrogazione OSV paginata, report e replay con digest.
  [Scansione reale e replay riusciti](https://github.com/chiaraberti13/ORBYX/actions/runs/42584973253).
  Un controllo riuscito non significa zero vulnerabilità: i findings rimangono
  nel report. Una copertura incompleta è un errore esplicito.
- **Aggiornamenti e rollback autenticati**: verifica Sigstore/GitHub OIDC prima
  dell'estrazione, vincoli repository/workflow/commit/piattaforma, attivazione
  atomica e rollback da archivio riverificato. Installer e updater condividono
  un lock; la release gestita non viene modificata in-place.

Prova crittografica autentica di installazione, rollback e rifiuto di un
archivio manomesso già superata su Ubuntu22.04/24.04/26.04 sul commit065adb1:
[run42587526146](https://github.com/chiaraberti13/ORBYX/actions/runs/42587526146).
Il rollback remoto riattiva una copia riverificata dello stesso build firmato;
le transizioni fra contenuti diversi sono provate dai test transazionali.
Questi ultimi usano un verificatore iniettato e non vengono presentati come
prove di autenticità crittografica.

Regressione locale conclusiva: 22gruppi CTest, 21eseguiti con successo e GPU
saltata per assenza di dispositivo; GPG escluso localmente perché l'ambiente
non consente il socket dell'agente. I test remoti includono la meccanica GPG
con chiave effimera. Test specifici: 11importazione, 10updater e 5correlazione
avvisi; widget EN/IT con wizard, importazione, aggiornamenti e persistenza.
La regressione locale non sostituisce le prove remote di piattaforma.

## Verifica conclusiva del codice 4ea5019

Tutte le esecuzioni seguenti sono concluse con esito **success**.

| Prova | Esecuzione |
| --- | --- |
| Build Ubuntu22.04/24.04/26.04, CTest, sanitizers, fuzz e Cppcheck | [42589266485](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266485) |
| Installer completo Ubuntu26.04 come utente normale | [42589266383](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266383) |
| Aggiornamento, rollback e manomissione con OIDC autentico sulle tre Ubuntu | [42589266318](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266318) |
| Fedora44 e ARM64 nativo, installazione ed esecuzione | [42589266400](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266400) |
| Fedora44 in VM completa | [42589266441](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266441) |
| AppImage in ambiente pulito | [42589266435](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266435) |
| CodeQL | [42589266430](https://github.com/chiaraberti13/ORBYX/actions/runs/42589266430) |

Nei build GUI remoti:23gruppi,22eseguiti con successo e GPU saltata.
Sanitizers:18gruppi,17eseguiti con successo e GPU saltata, leak detection attiva.

## Esiti reali

| Verifica | Esito e prova |
| --- | --- |
| Ubuntu 22.04, 24.04, 26.04: build, CTest e pacchetti | [Verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236764). Nei build GUI: 20 gruppi, 19 eseguiti e uno GPU saltato perché manca il dispositivo. |
| ASan, UBSan e LeakSanitizer | [Verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236764), job129027592874: 15 gruppi, GPU saltata; `detect_leaks=1`. Il nuovo `image-demo-regression` passa anche senza GUI. |
| Cppcheck, libFuzzer e CodeQL | [Build/analisi/fuzz verdi](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236764), [CodeQL verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236733). Nessun controllo disabilitato per ottenere il risultato. |
| Fedora44 e Ubuntu24.04 ARM64 nativo | [Verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236864), compresa demo dell’app installata. |
| AppImage x86_64 in Ubuntu22.04 pulito | [Verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236837), compresa demo installata, senza Qt/OpenCV di sistema o FUSE. |
| VM Fedora44 completa | [Verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236705): kernel proprio6.19.10-300.fc44.x86_64, RPM installato, demo, equivalenza CLI/GUI/Natural Color, disinstallazione e originali conservati. Cloud-init termina senza errori. |
| Installer guidato Ubuntu26.04 | [Verde su8103ae6](https://github.com/chiaraberti13/ORBYX/actions/runs/42576626057), inclusa la nuova demo installata e verificata. |
| Acquisizione reale Elektro-L4 e SatDump | [Verde su8103ae6](https://github.com/chiaraberti13/ORBYX/actions/runs/42576626065): pipeline reale e Natural Color a16bit. Il terzo incremento aggiunge test e corregge solo la preparazione della VM. |
| Provenienza autentica degli artifact | [Verde](https://github.com/chiaraberti13/ORBYX/actions/runs/42577236585), con identità del workflow OIDC e rifiuto degli artifact manomessi. La meccanica GPG usa una chiave di test effimera. |

## Cosa verificano i nuovi test

`gui-widget-flow` usa i widget di produzione, non una seconda interfaccia:

- Click reali sui pulsanti di aggiunta/rimozione/esecuzione coda EN/IT.
- Scorrimento e controllo che il pulsante sia nel viewport, anche a960×680.
- Ripresa da tastiera e digest del prodotto invariato.
- Errore reale di avvio del processo: controlli riabilitati e stato Errore,
  senza recuperare un vecchio stato Completato.
- Backend che termina con exit0 ma lascia uno stato `running`: mai Completato.
- Duplicazione profilo tramite i tre dialoghi reali: file nuovo corretto,
  selezionato nell’app, originale immutato.
- Pulsante **Avvia demo immagini** EN/IT: sei JPEG elaborati e immagine
  effettivamente mostrata, non solo screenshot precaricato.

`image-demo-regression` esercita il core anche sotto sanitizzatori:

- Sei JPEG reali → immagine RGB321/Natural Color1428×1428.
- Originali invariati e canale di riferimento conservato pixel per pixel.
- Correlazione ECC sopra0,94 e scarto verticale misurato fra le metà.
- File troncati, marker JPEG incompleti, dimensione oltre1MiB, symlink e canali
  mancanti rifiutati; rollback senza prodotti parziali.
- Destinazione esistente conservata integralmente.

`scripts/ci/check_image_demo.py` controlla anche il programma **installato**:
PNG RGB a8bit di dimensioni corrette, sei canali intermedi, report delle
registrazioni e versione con contrasto distinta da quella con livelli originali.

Il precedente riconoscitore strutturale di cartelle era stato eseguito sul campione reale
`assets/sample-elektro-l4`: classificazione strutturale/stabilità `complete`,
tutti i9 file invariati prima/dopo la scansione. Questo verifica la struttura,
non la correttezza dei metadati o l’idoneità della preview all’elaborazione nativa.
Il nuovo importatore nativo è più restrittivo e trattiene questa preview:
non la considera un'acquisizione RDAS utilizzabile dal motore.

## Prova la demo nell’app

Dalla cartella ORBYX, dopo l’aggiornamento:

```bash
git pull
bash install.sh
bash start.sh
```

Attendi **Installazione completata** prima di avviare l’app.
Premi **Avvia demo immagini**, oppure **Ctrl+Maiusc+D**. Non devi inserire
percorsi. L’immagine unica appare nell’anteprima; il riquadro Attività indica
risultato e log. Per aprire direttamente la stessa modalità:

```bash
bash demo.sh
```

La demo conserva anche intermedi e report; `composite-raw.png` ha i livelli
originali della composizione, `composite.png` un contrasto comune dichiarato.
Le righe e la giunzione presenti nelle sorgenti non vengono nascoste con dati
inventati. [Provenienza dei JPEG](../assets/demo-jpeg/SOURCE.md).

## Limiti ancora aperti

Non sono dichiarati superati: GPU fisica OpenCL; precisione geografica assoluta
con verità a terra; screen reader e prove con utenti/tecnologie assistive reali;
firma con una chiave GPG personale di release. OIDC attesta il workflow.

Correlazione SBOM, importazione controllata, wizard e aggiornamento/rollback
autenticato sono ora implementati. Le verifiche esterne sopra non sono sviluppo
software mancante e non vengono segnate come superate senza le risorse reali.
L'updater copre i pacchetti nativi utente Ubuntu dichiarati; non aggiorna APT,
RPM o AppImage e non inventa una firma personale GPG.
