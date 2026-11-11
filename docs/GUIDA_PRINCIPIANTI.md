# ORBYX — Guida per principianti

Questa guida accompagna dall’installazione all’immagine finale. Non è necessario conoscere CMake, OpenCV o i comandi interni di ORBYX.

## Prima di iniziare: quale tipo di file hai?

ORBYX ha un solo percorso manuale per creare l'immagine. Riconosce automaticamente
quale tipo di cartella hai scelto.

| File disponibili | Percorso da usare |
|---|---|
| Sei JPEG chiamati `sx_...` e `dx_...` | **Crea immagine — modalità manuale…** oppure **Generazione automatica (1 clic)** |
| Una cartella esportata da SatDump con `MSUGS_VIS1`, `MSUGS_VIS2`, ecc. | **Crea immagine — modalità manuale…** |
| Un file grezzo ricevuto dal satellite, per esempio CADU | Prima deve essere decodificato ed esportato con SatDump |

Non devi scegliere in anticipo un percorso diverso: selezioni la cartella e ORBYX
ti dice se ha riconosciuto i sei JPEG oppure i prodotti VIS di SatDump. Una
cartella generica come Documenti, invece, non contiene immagini elaborabili.

## 1. Installazione su Ubuntu

### Prima installazione

Apri il terminale con **Ctrl + Alt + T** e copia un blocco alla volta:

```bash
git clone https://github.com/chiaraberti13/ORBYX.git
cd ORBYX
bash install.sh
```

Durante l’installazione:

- Ubuntu può chiedere la password. Mentre la scrivi non vengono mostrati caratteri: è normale.
- Non aggiungere `sudo` davanti a `bash install.sh`.
- Se il gestore dei pacchetti è occupato dagli aggiornamenti automatici, ORBYX attende che si liberi.
- Compilazione e test possono richiedere diversi minuti.

L’installazione è terminata correttamente soltanto quando compare:

```text
Installazione completata
```

### Aggiornamento di una copia già installata

Apri il terminale nella cartella ORBYX, poi esegui:

```bash
git pull
bash install.sh
```

Non serve clonare nuovamente il repository.

### Avvio

```bash
bash start.sh
```

Per gli avvii successivi basta `bash start.sh`. L’installazione va ripetuta soltanto dopo un aggiornamento.

## 2. Prova più semplice: genera l’immagine demo

Questo percorso serve anche per sei JPEG personali organizzati come tre immagini di una porzione e tre dell’altra.

1. Apri ORBYX con `bash start.sh`.
2. Premi **Generazione automatica**.
3. Al primo utilizzo ORBYX prepara sei JPEG dimostrativi, li elabora e mostra il risultato.
4. Premi **Modifica il risultato nell’editor manuale…** se vuoi correggere sovrapposizione, posizione, colore o giunzione.
5. Premi **Apri immagini generate** per aprire la cartella contenente il PNG finale.

Il file principale si chiama `composite.png`. ORBYX conserva anche intermedi e report nella stessa sottocartella.

### Usare i tuoi sei JPEG

1. Premi **Apri cartella JPEG**.
2. Sostituisci i sei file presenti con i tuoi.
3. Mantieni esattamente questi nomi:

```text
sx_MSUGS-VIS-1.jpg
sx_MSUGS-VIS-2.jpg
sx_MSUGS-VIS-3.jpg
dx_MSUGS-VIS-1.jpg
dx_MSUGS-VIS-2.jpg
dx_MSUGS-VIS-3.jpg
```

4. Torna in ORBYX e premi nuovamente **Generazione automatica**.
5. Attendi che lo stato diventi **Completato**.
6. Premi **Apri immagini generate**.

Ogni elaborazione crea una nuova sottocartella: i risultati precedenti non vengono sovrascritti.

### Scegliere manualmente una cartella con sei JPEG

1. Premi **Crea immagine — modalità manuale…**.
2. In **Cartella di origine** seleziona la cartella che contiene i sei JPEG.
3. Quando compare **Rilevati sei JPEG**, scegli la nuova destinazione e premi **Avanti**.
4. Completa i passaggi: non sono richiesti SatDump né un file JSON.
5. Premi **Elabora e apri l’editor manuale**. ORBYX crea la prima composizione e apre le due porzioni come livelli modificabili.

## 3. Creare un’immagine finale da un’esportazione SatDump

Usa questo percorso quando SatDump ha già prodotto una struttura simile a questa:

```text
cartella-del-passaggio/
├── MSUGS_VIS1/
│   ├── MSUGS-VIS-1.png
│   ├── MSUGS-VIS-2.png
│   ├── MSUGS-VIS-3.png
│   └── product.cbor
├── MSUGS_VIS2/
│   ├── MSUGS-VIS-1.png
│   ├── MSUGS-VIS-2.png
│   ├── MSUGS-VIS-3.png
│   └── product.cbor
└── eventuali altre porzioni MSUGS_VIS...
```

Devi selezionare personalmente `cartella-del-passaggio`, cioè la cartella che contiene direttamente le cartelle `MSUGS_VIS...`. ORBYX non decide automaticamente quale acquisizione usare. Non selezionare Documenti, una singola cartella `MSUGS_VIS1` o la cartella dei sei JPEG.

### Procedura guidata

1. Apri ORBYX.
2. Premi **Crea immagine — modalità manuale…**.
3. Nella pagina **1. Importa**:
   - in **Cartella di origine**, premi **Seleziona…** e scegli tu la cartella descritta sopra;
   - **Profilo di calibrazione** è già compilato automaticamente con il JSON incluso;
   - se vuoi usare una calibrazione diversa, modifica direttamente il percorso oppure premi il relativo **Seleziona…** e scegli un altro `calibration.json`;
   - lascia la **Nuova cartella di lavoro** proposta;
   - controlla che sotto i campi compaiano i prodotti VIS rilevati;
   - premi **Avanti**.
4. Nella pagina **2. Allinea canali**:
   - scegli il satellite corretto;
   - lascia **CPU / OpenCL automatico**, oppure scegli **Solo CPU** se vuoi escludere OpenCL;
   - premi **Avanti**.
5. Nella pagina **3. Unisci porzioni**, leggi quale punto di partenza verrà usato:
   - se il profilo contiene ROI, ORBYX conserva la vecchia logica e apre quei ritagli come livelli modificabili;
   - senza ROI complete, ORBYX apre le porzioni intere per l’allineamento visuale.
6. Nella pagina **4. Colore**, scegli normalmente **MSU-GS Natural Color (RGB321)**.
7. Nella pagina **5. Esporta**, controlla il riepilogo, spunta la conferma e premi **Allinea e apri l’editor**.
8. Attendi: ORBYX allinea i canali e apre automaticamente il compositore.

Se **Avanti** non procede, leggi il messaggio rosso mostrato nella stessa pagina. Il caso più comune è la selezione della cartella sbagliata.

In sintesi: ORBYX propone automaticamente il profilo JSON, ma non impone quel
file e non sceglie la cartella SatDump al posto tuo. Entrambi i percorsi rimangono
visibili; la cartella di origine e il JSON possono essere cambiati prima di
premere **Avanti**.

## 4. Quando serve la calibrazione

Il campo **Profilo di calibrazione** contiene un percorso reale. Puoi aprire la sua cartella con **Apri calibrazioni**.

Se ORBYX segnala che il profilo non supporta il satellite o i prodotti selezionati:

1. chiudi il percorso **Crea immagine — modalità manuale…**;
2. nella finestra principale scegli la cartella SatDump corretta;
3. seleziona il satellite;
4. premi **Calibrazione guidata…**;
5. lascia il profilo esistente come base;
6. lascia la cartella di destinazione proposta;
7. controlla il satellite e usa la scala proposta, normalmente `0,125` per immagini grandi;
8. conferma il riepilogo;
9. nella finestra principale premi **Avvia elaborazione**.

Al termine ORBYX:

- crea un nuovo `calibration.json`;
- mostra il suo percorso effettivo nel riquadro Attività;
- lo seleziona automaticamente;
- prepara una nuova destinazione per l’allineamento.

A questo punto puoi premere **Crea immagine — modalità manuale…** e seguire il percorso del capitolo precedente.

Non esiste un profilo L4 universale valido per ogni acquisizione. Per L4 può essere necessario calcolare la calibrazione usando i dati effettivi del passaggio.

## 5. Uso essenziale dell’editor

Quando il compositore si apre, le porzioni elaborate sono già caricate come livelli. Per ottenere rapidamente un’immagine finale:

1. Premi **Strategia consigliata…** per leggere la proposta di ORBYX.
2. Se necessario, premi **Allinea automaticamente le sovrapposizioni…** e conferma soltanto le proposte affidabili.
3. Seleziona un livello nell’elenco.
4. Controlla la giunzione scegliendo **Differenza**, **Scacchiera** oppure **Tendina**.
5. Premi **Zoom sulla sovrapposizione selezionata**.
6. Correggi la posizione trascinando il livello nell’anteprima oppure usando i tasti freccia.
7. Scegli la modalità di unione:
   - **Nessun blending** per un taglio netto;
   - **Sfumatura** per una transizione semplice;
   - **Giunzione a costo minimo** per cercare un percorso visualmente meno evidente;
   - **Blending multibanda** per uniformare maggiormente una sovrapposizione complessa.
8. Se luminosità o colore differiscono, premi **Uniforma il livello selezionato all’adiacente…** e controlla la proposta.
9. Premi **Valutazione qualità…**: è un aiuto, non sostituisce il controllo visivo.
10. Salva il lavoro con **Salva progetto modificabile…**.
11. Crea il PNG finale con **Esporta immagine unica…**.

### Progetto e immagine finale non sono la stessa cosa

| Comando | Risultato |
|---|---|
| **Salva progetto modificabile…** | Salva livelli, posizione, ritagli, colore e impostazioni per continuare in seguito |
| **Esporta pacchetto progetto…** | Crea una copia trasportabile con progetto e sorgenti necessari |
| **Esporta immagine unica…** | Crea il PNG finale da condividere o usare fuori da ORBYX |

Esportare il PNG non salva automaticamente il progetto modificabile.

## 6. Dove vengono salvati i file

Non devi scrivere manualmente questi percorsi: usa i pulsanti presenti nell’app.

| Contenuto | Pulsante | Cartella predefinita |
|---|---|---|
| Immagini automatiche, manuali e PNG finali | **Apri immagini generate** | `Immagini/ORBYX/Output Images` |
| Profilo incluso e nuove calibrazioni | **Apri calibrazioni** | `Immagini/ORBYX/Calibration` |
| Sei JPEG usati dalla generazione automatica | **Apri cartella JPEG** | cartella dati interna aperta direttamente dall’app |
| Log delle elaborazioni immagini | dal riquadro Attività o cartella ORBYX | `Immagini/ORBYX/Logs` |
| Log di installazione e avvio | cartella del repository | `logs` |

Su un sistema configurato in inglese, `Immagini` può chiamarsi `Pictures`.

## 7. Errori comuni

### «Nessun prodotto MSU-GS VIS trovato»

Hai scelto una cartella che non contiene direttamente `MSUGS_VIS1`, `MSUGS_VIS2`, ecc. Torna alla pagina Importa e scegli la cartella del passaggio esportata da SatDump.

### «Il file richiesto non esiste: ...calibration.json»

Il percorso salvato apparteneva a una vecchia elaborazione. Aggiorna ORBYX con `git pull` e `bash install.sh`: le versioni aggiornate sostituiscono automaticamente i percorsi obsoleti con il profilo incluso reale.

### «Il profilo non supporta i prodotti selezionati»

Controlla il satellite. Se è corretto, esegui **Calibrazione guidata…** sui dati del passaggio.

### «La destinazione esiste già»

ORBYX non sovrascrive risultati o progetti. Lascia la nuova destinazione proposta oppure scegli un nome nuovo.

### Installazione bloccata da `dpkg` o `unattended-upgrades`

Lascia terminare gli aggiornamenti automatici. `install.sh` attende il blocco; se scade il tempo, ripeti l’installazione più tardi. Non cancellare manualmente i file di blocco di `dpkg`.

### Il programma non si apre

Esegui:

```bash
bash start.sh
```

Poi controlla l’ultimo file nella cartella `logs`. Non avviare manualmente file dentro `build` o `build-desktop`.

## 8. Cosa è completo e quali limiti restano

Il flusso software supportato è implementato: installazione Linux dichiarata, demo, importazione SatDump MSU-GS VIS, calibrazione, allineamento, composizione modificabile, colore visuale, salvataggio progetto ed esportazione PNG.

Restano limiti che non possono essere risolti con un semplice test automatico:

- ORBYX non decodifica direttamente i file grezzi CADU: questa fase resta a SatDump;
- l’allineamento visuale non è una georeferenziazione certificata rispetto a verità a terra;
- Natural Color è una resa visuale, non una calibrazione radiometrica scientifica;
- l’esecuzione GPU richiede una GPU OpenCL reale; in sua assenza ORBYX usa la CPU;
- la verifica con screen reader e utenti reali resta esterna;
- macOS non fa parte della matrice attualmente verificata.

Questi limiti non impediscono di usare ORBYX per il flusso descritto nella guida, ma delimitano ciò che il risultato può certificare scientificamente.

## Checklist finale

Prima di esportare controlla:

- [ ] Ho scelto la cartella corretta in base al tipo di file.
- [ ] Il satellite selezionato è corretto.
- [ ] Tutti i prodotti VIS necessari sono rilevati.
- [ ] Ho controllato ogni sovrapposizione nell’editor.
- [ ] Ho verificato bordi, aree nere e differenze di luminosità.
- [ ] Ho salvato il progetto modificabile.
- [ ] Ho esportato l’immagine unica.
- [ ] So dove si trovano risultato e log.

Per i dettagli tecnici del compositore consulta [COMPOSITOR.md](COMPOSITOR.md). Per problemi di installazione consulta [INSTALLATION.md](INSTALLATION.md).
