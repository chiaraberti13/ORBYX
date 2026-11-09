# Comporre più porzioni e regolare il colore

Nell’app premi **Componi porzioni e colore…**. Il compositore lavora con più
porzioni indipendenti: non presume due metà SX/DX, un ordine geografico o un
numero fisso di parti. Le modifiche sono non distruttive.

Per provare subito i sei JPEG inclusi premi **Aggiungi porzioni demo modificabili**.
L’app esegue la registrazione dei canali e apre le due porzioni come livelli
modificabili, con il ritaglio e il contrasto iniziali del campione. Questo esempio
non limita il numero o la disposizione dei livelli delle altre acquisizioni.

Per il percorso più semplice premi invece **Crea immagine finale…** nella
finestra principale. La procedura mostra cinque passaggi: **Importa**, **Allinea
canali**, **Unisci porzioni**, **Colore** ed **Esporta**. Controlla che la
cartella SatDump sia completa, che il profilo sia un JSON valido e che la
destinazione sia nuova ed esterna agli originali. Dopo la conferma avvia davvero
l’allineamento e apre da sola il compositore con Natural Color o RGB123: non devi
cercare né riaprire le cartelle intermedie. L’esportazione resta intenzionalmente
nell’editor, così puoi controllare e correggere le giunzioni prima del PNG finale.

## Dai prodotti SatDump all’immagine unica

### Apertura diretta dopo l’elaborazione ORBYX

Quando avvii **Genera** dall’interfaccia principale, ORBYX continua a creare il
prodotto storico `MSUGS_VIS_Aligned` se il profilo contiene entrambe le ROI.
In più conserva, solo per il flusso grafico, le due acquisizioni allineate
separate. Premendo poi **Componi porzioni e colore…** queste vengono aperte come
due livelli modificabili con gli stessi ritagli e la stessa disposizione usati
dalla fusione storica. L’immagine storica non viene alterata: trascinamenti,
rotazione, scala, ritaglio, colore e sfumatura riguardano il nuovo progetto del
compositore.

Se il profilo non contiene una coppia completa di ROI, il compositore carica le
porzioni allineate intere e permette di proporne la sovrapposizione visuale o di
posizionarle manualmente. Il funzionamento non presume che siano una parte
sinistra e una destra. Il comando CLI `generate` mantiene per impostazione
predefinita gli stessi file e lo stesso comportamento precedenti.

Questo collegamento diretto riguarda al momento l’elaborazione singola avviata
dal pulsante principale o da **Crea immagine finale…**. L’integrazione
equivalente nella coda e la ripresa dei singoli passaggi sono ancora tracciate
in M2-08.

1. Allinea prima i canali di ciascuna acquisizione con il normale flusso ORBYX.
   Il compositore non sostituisce la calibrazione dei canali.
2. Nel compositore scegli **MSU-GS Natural Color (RGB321)** per il relativo
   strumento, oppure **RGB123** per la composizione generica già prevista dal
   core. Natural Color non è una calibrazione radiometrica e non è applicabile
   automaticamente a qualsiasi sensore.
3. Premi **Aggiungi 3 canali allineati…** e scegli una cartella con
   `MSUGS-VIS-1.png`, `MSUGS-VIS-2.png`, `MSUGS-VIS-3.png`. Ripeti per le altre
   porzioni. L’app crea una porzione a colori preservando 8/16 bit.
4. Se hai già porzioni PNG a colori o in grigio, usa **Aggiungi porzioni PNG…**.
   Sono accettati PNG in grigio/RGB a 8/16 bit, senza alpha; non JPEG/TIFF o
   qualunque immagine generica. Convertire un JPEG non recupera precisione persa.
5. Seleziona nella lista una porzione affidabile da lasciare ferma e premi
   **Allinea automaticamente le sovrapposizioni…**. ORBYX cerca dettagli visivi
   comuni anche attraverso una catena di più porzioni e propone traslazione,
   rotazione e scala uniforme. Prima di applicare mostra corrispondenze valide,
   percentuale di sovrapposizione, errore e confidenza. **Annulla** non modifica
   nulla; **Applica** crea uno stato completamente modificabile e annullabile.
   Le porzioni prive di una corrispondenza affidabile restano invariate.
   Se il progetto è stato aperto dal normale flusso con ROI, puoi usare
   **Ripristina ROI del profilo** per tornare in qualunque momento alla geometria
   storica esatta. Puoi quindi provare la proposta visuale e recuperare il punto
   di partenza senza rielaborare i canali.
6. Controlla sempre ogni giunzione. Seleziona un livello: il suo contorno azzurro
   appare nell’anteprima. Trascina per spostarlo oppure imposta **X/Y**. Se
   l’automatismo non trova abbastanza dettagli, ritaglia intestazioni e grandi
   zone nere e riprova, oppure procedi manualmente. Puoi disporre le porzioni in
   orizzontale, verticale o su una griglia irregolare.
   Per una correzione precisa fai clic sull’anteprima e usa le frecce: ogni
   pressione sposta il livello selezionato di1pixel; con `Shift` lo spostamento
   è di10pixel. Anche questi movimenti sono annullabili e ripetibili. Attiva
   **Lampeggia livello selezionato (solo anteprima)** per alternare la porzione
   corrente e vedere più facilmente coste, nuvole o dettagli sdoppiati. Il
   lampeggio non modifica i livelli e non compare nel PNG esportato.
   Nel menu di ispezione scegli **Differenza: selezionato e adiacente** per
   visualizzare in falsi colori soltanto le differenze dentro l’area comune tra
   il livello selezionato e quello immediatamente sotto, oppure quello sopra se
   è il primo. Il nero indica le zone fuori sovrapposizione. Anche questa vista
   è soltanto diagnostica e non modifica progetto o esportazione.
   **Scacchiera: selezionato e adiacente** alterna invece tasselli delle due
   porzioni, utile per seguire continuità di coste, bordi e strutture senza
   perdere il contesto delle immagini originali. Anche questa modalità resta
   esclusa dal PNG finale.
   **Tendina: selezionato e adiacente** divide la stessa coppia lungo una linea
   verticale. Sposta il cursore da0 a100% per scegliere dove termina una
   porzione e inizia l’altra; la linea colorata indica il confine della vista.
   **Zoom sulla sovrapposizione selezionata** centra e ingrandisce l’area comune
   della coppia corrente. Se i poligoni trasformati non si intersecano, ORBYX lo
   segnala senza cambiare zoom o geometria.
7. Regola **Scala**, **Rotazione**, **Ritaglio**, **Opacità** e **Sfumatura bordi**.
   I valori del ritaglio sono nelle coordinate della sorgente; larghezza e altezza
   entrambe 0 significano immagine intera. Per eliminare intestazioni usa il
   ritaglio: il compositore non indovina le zone da scartare. X/Y individuano
   l’origine del livello ritagliato; scala e rotazione si applicano attorno a
   quell’origine. Sono ammesse coordinate negative e il canvas include
   automaticamente l’estensione dei livelli visibili.
8. **Alza/Abbassa** cambia l’ordine di sovrapposizione. La lista va dal fondo alla
   cima. La sfumatura attenua tutti i bordi del livello, non soltanto una giunzione:
   serve sovrapposizione reale per una transizione continua. Non ricostruisce
   parti mancanti e non elimina automaticamente le cuciture.
9. Correggi i colori del livello selezionato: nero/bianco normalizzati 0–1,
   gamma, esposizione EV, saturazione e guadagni rosso/verde/blu. Per una
   regolazione comune usa **Applica colori selezionati a tutti**. **Ripristina
   colori selezionati** torna ai valori neutri. Valori troppo forti possono
   saturare o tagliare le alte luci: controlla il risultato.
10. **Annulla/Ripeti** conserva fino a 50 stati modificabili. **Salva progetto
   modificabile…** conserva livelli e parametri in JSON; **Apri progetto…** li
   ricarica. Il progetto usa riferimenti assoluti, non incorpora le immagini:
   conserva originali e porzioni generate nelle loro posizioni. Le porzioni
   colore generate sono conservate nell’area dati dell’app, senza cancellazione
   automatica che renderebbe inutilizzabili i progetti salvati.
11. Premi **Esporta immagine unica…** e scegli un nuovo PNG. Gli originali non
    vengono modificati e un file di destinazione esistente viene rifiutato.
    Il PNG è il risultato appiattito; salva anche il progetto per conservare
    l’editabilità. Alla chiusura l’app chiede come gestire modifiche non salvate.

Tutto questo avviene nell’interfaccia ORBYX, senza passare a GIMP o tornare in
SatDump per la composizione colore supportata. Rimane possibile esportare
singolarmente Natural Color dal pulsante precedente.

## Risoluzione, precisione e limiti

L’anteprima è limitata a 1200 pixel sul lato maggiore e visualizzata a8 bit, con
zoom 25–400% e pulsante **Adatta**. L’export usa lo stesso motore e parametri, a risoluzione di scena
originale:16 bit se almeno una porzione è16 bit, altrimenti8 bit. Un input8 bit
promosso a16 bit non acquista precisione aggiuntiva. L’interpolazione geometrica
è bilineare; le regolazioni colore sono esplicite, non modifiche radiometriche
certificate. Nessun ridimensionamento implicito dell’export per imitare la preview.

Massimo32 livelli; ogni PNG mantiene i limiti del reader core (150 Mpx e 1 GiB
compresso); canvas massimo 200 Mpx e 100000 pixel per lato. Un controllo del budget
di buffer (output, immagine decodificata più grande e scratch) rifiuta operazioni
oltre 2 GiB. Questi limiti non garantiscono un tetto all’RSS del processo: decoder,
encoder e librerie possono richiedere memoria aggiuntiva. Per grandi acquisizioni
servono RAM sufficiente e ritagli appropriati. Gli input sono letti in due passaggi
limitati e il rendering usa tile; non vengono conservati tutti i livelli decodificati.

L’allineamento automatico usa esclusivamente dettagli visivi e stima una
trasformazione di similarità: spostamento, rotazione e scala uniforme. Non altera
la porzione scelta come riferimento, non applica risultati sotto le soglie di
affidabilità e non interpreta nomi come SX/DX. Può fallire correttamente con
porzioni senza area comune, troppo scure o uniformi, scene riprese in tempi
diversi, nuvole cambiate, prospettiva/parallasse o proiezioni differenti. Non è
una georeferenziazione e la confidenza statistica non certifica la correttezza
geografica: la geometria assoluta resta M3-01 e la verifica visiva è obbligatoria.

ORBYX non è un GIS e non è un editor GIMP completo: niente pennelli, maschere
dipinte, selezioni libere, deformazioni
prospettiche o filtri generici. Non è inclusa una nuova decodifica dei file grezzi.

## Verifica

`mosaic-regression`: tre porzioni su righe/colonne diverse, origine negativa,
ritaglio/scala/rotazione, ordine, opacità e sfumatura, oracoli colore, bit bassi16 bit
conservati a trasformazioni neutre, JSON round-trip, parametri invalidi, link e
limiti, export atomico senza sovrascritture.
Verifica inoltre sovrapposizioni automatiche su tre ritagli concatenati, recupero
di rotazione/scala e rifiuto di una porzione uniforme o priva di corrispondenza.

`mosaic-widget-flow`: widget reali EN/IT, tre livelli, modifica numerica e drag,
annulla/ripeti, colore comune, progetto riaperto, composizione Natural Color
da tre canali e PNG GUI confrontato esattamente col renderer nativo.
Verifica anche proposta automatica EN/IT, evidenze mostrate, annullamento senza
modifiche, applicazione esplicita, annulla/ripeti e messaggio di revisione.
Verifica inoltre l’apertura di due prodotti allineati con le ROI del profilo,
ritagli e posizione iniziale esatti, il ripristino dopo una modifica manuale e
la loro persistenza in un progetto.

`engine-regression` verifica che l’attivazione delle porzioni modificabili non
cambi neppure un pixel del prodotto ROI storico, che il comportamento
predefinito non aggiunga file e che ROI non valide continuino a essere rifiutate.

`gui-widget-flow` esegue il percorso completo dal pulsante **Genera**: controlla
il prodotto storico, le due porzioni conservate e la loro apertura automatica
nel compositore EN/IT con la geometria del profilo.

Queste prove non certificano automaticamente la qualità di una cucitura su ogni
scena reale; la valutazione dell’operatore rimane parte del flusso.

Errori e operazioni del compositore sono registrati in un log locale dedicato,
apribile con **Apri log compositore**. I log non vengono caricati automaticamente.
