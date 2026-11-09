# Sei JPEG della demo desktop

Questi sono i sei file `sx_MSUGS-VIS-{1,2,3}.jpg` e
`dx_MSUGS-VIS-{1,2,3}.jpg` forniti dall'utente per la demo ORBYX.
Sono anteprime JPEG a 8 bit di 714 × 2048 pixel. Gli originali sono conservati
senza modifiche; i digest sono in SHA256SUMS.

L'app misura lo scarto fra i canali con correlazione di fase e ECC affine,
esclude le prime 80 righe con le rampe, compone RGB321 / Natural Color e
accosta SX e DX correggendo lo scarto verticale misurato sui bordi adiacenti.
Non è stata verificata una sovrapposizione fra le metà: nessun pixel viene
inventato per nascondere la giunzione. La cornice finale è 1428 × 1428.

`composite-raw.png` conserva i livelli della composizione; `composite.png`
applica solo un contrasto comune dichiarato nel report (nero 8, bianco 188,
esponente 0,8). Gli eventuali difetti delle sorgenti rimangono visibili.
Il report registra trasformazioni, correlazioni e ritaglio.

Queste anteprime non hanno CBOR associato e non certificano geometria assoluta,
precisione radiometrica, GPU o qualità del prodotto RDAS originale a 16 bit.
Data e satellite specifico non sono attestati dai JPEG. Diritti sulle immagini
ai rispettivi titolari; la licenza GPL del codice non attribuisce una nuova
licenza alle immagini satellitari.
