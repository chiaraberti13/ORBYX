# Test verificati — 9 novembre 2026

Codice verificato: **b86103f**. Le modifiche sono state pubblicate direttamente
su `main` in tre incrementi: `52bebf6`, `8103ae6`, `b86103f`.

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

Il riconoscitore di cartelle è stato inoltre eseguito sul campione reale
`assets/sample-elektro-l4`: classificazione strutturale/stabilità `complete`,
tutti i9 file invariati prima/dopo la scansione. Questo verifica la struttura,
non la correttezza dei metadati o l’idoneità della preview all’elaborazione nativa.

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

La roadmap contiene inoltre funzionalità ancora da implementare: correlazione
SBOM con feed di vulnerabilità, import automatico/disambiguazione metadati,
wizard di calibrazione e aggiornamento/rollback firmato. Questo report chiude
le verifiche descritte, non dichiara il repository interamente terminato.
