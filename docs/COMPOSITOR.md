# Comporre più porzioni e regolare il colore

Nell’app premi **Componi porzioni e colore…**. Il compositore lavora con più
porzioni indipendenti: non presume due metà SX/DX, un ordine geografico o un
numero fisso di parti. Le modifiche sono non distruttive.

Per provare subito i sei JPEG inclusi premi **Aggiungi porzioni demo modificabili**.
L’app esegue la registrazione dei canali e apre le due porzioni come livelli
modificabili, con il ritaglio e il contrasto iniziali del campione. Questo esempio
non limita il numero o la disposizione dei livelli delle altre acquisizioni.

## Dai prodotti SatDump all’immagine unica

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
5. I livelli iniziano sovrapposti nell’origine della scena. Seleziona un livello
   nella lista: il suo contorno azzurro appare nell’anteprima. Trascina per
   spostarlo, oppure imposta **X/Y** in pixel della sorgente. Puoi disporre le
   porzioni in orizzontale, verticale o su una griglia irregolare.
6. Regola **Scala**, **Rotazione**, **Ritaglio**, **Opacità** e **Sfumatura bordi**.
   I valori del ritaglio sono nelle coordinate della sorgente; larghezza e altezza
   entrambe 0 significano immagine intera. Per eliminare intestazioni usa il
   ritaglio: il compositore non indovina le zone da scartare. X/Y individuano
   l’origine del livello ritagliato; scala e rotazione si applicano attorno a
   quell’origine. Sono ammesse coordinate negative e il canvas include
   automaticamente l’estensione dei livelli visibili.
7. **Alza/Abbassa** cambia l’ordine di sovrapposizione. La lista va dal fondo alla
   cima. La sfumatura attenua tutti i bordi del livello, non soltanto una giunzione:
   serve sovrapposizione reale per una transizione continua. Non ricostruisce
   parti mancanti e non elimina automaticamente le cuciture.
8. Correggi i colori del livello selezionato: nero/bianco normalizzati 0–1,
   gamma, esposizione EV, saturazione e guadagni rosso/verde/blu. Per una
   regolazione comune usa **Applica colori selezionati a tutti**. **Ripristina
   colori selezionati** torna ai valori neutri. Valori troppo forti possono
   saturare o tagliare le alte luci: controlla il risultato.
9. **Annulla/Ripeti** conserva fino a 50 stati modificabili. **Salva progetto
   modificabile…** conserva livelli e parametri in JSON; **Apri progetto…** li
   ricarica. Il progetto usa riferimenti assoluti, non incorpora le immagini:
   conserva originali e porzioni generate nelle loro posizioni. Le porzioni
   colore generate sono conservate nell’area dati dell’app, senza cancellazione
   automatica che renderebbe inutilizzabili i progetti salvati.
10. Premi **Esporta immagine unica…** e scegli un nuovo PNG. Gli originali non
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

L’unione è manuale nelle coordinate della scena. Non è un GIS, non risolve
automaticamente proiezioni diverse, non riconosce sovrapposizioni inesistenti e
non certifica la geografia. La geometria assoluta resta M3-01. Non è un editor
GIMP completo: niente pennelli, maschere dipinte, selezioni libere, deformazioni
prospettiche o filtri generici. Non è inclusa una nuova decodifica dei file grezzi.

## Verifica

`mosaic-regression`: tre porzioni su righe/colonne diverse, origine negativa,
ritaglio/scala/rotazione, ordine, opacità e sfumatura, oracoli colore, bit bassi16 bit
conservati a trasformazioni neutre, JSON round-trip, parametri invalidi, link e
limiti, export atomico senza sovrascritture.

`mosaic-widget-flow`: widget reali EN/IT, tre livelli, modifica numerica e drag,
annulla/ripeti, colore comune, progetto riaperto, composizione Natural Color
da tre canali e PNG GUI confrontato esattamente col renderer nativo.

Queste prove non certificano automaticamente la qualità di una cucitura su ogni
scena reale; la valutazione dell’operatore rimane parte del flusso.

Errori e operazioni del compositore sono registrati in un log locale dedicato,
apribile con **Apri log compositore**. I log non vengono caricati automaticamente.
