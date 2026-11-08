# Natural Color MSU-GS

## Italiano

L'esportazione è disponibile nel core, nella CLI e nel desktop. Usa **RGB321**: CH3→rosso, CH2→verde, CH1→blu. La precedente API `composeRgb` mantiene la mappatura generica CH1→rosso, CH2→verde, CH3→blu; i due comandi hanno quindi scopi distinti.

```sh
orbyx natural-color --ch1 /VIS_allineato/MSUGS-VIS-1.png \
  --ch2 /VIS_allineato/MSUGS-VIS-2.png --ch3 /VIS_allineato/MSUGS-VIS-3.png \
  --output /destinazione/nuovo-natural-color.png --lang it
```

Nel desktop scegliere **Esporta Natural Color…** (Ctrl+Shift+N), selezionare la cartella di un singolo prodotto VIS allineato e un nuovo file PNG. Il worker opera fuori dal thread grafico e impedisce avvii concorrenti o chiusura durante l'elaborazione. La cartella scelta deve contenere i tre canali; non occorre fondere VIS1 e VIS2. La funzione C++ è `orbyx::composeNaturalColor`.

### Colore e precisione

La correzione riprende le regole del codice GPL originale, applicandole a HSV in virgola mobile: rosso H≥340° oppure H≤40° spostato di+60°; giallo 40°<H≤100° spostato di−6°. La prima regola ha precedenza; il risultato combina60% RGB321 originale e40% corretto. Il commento upstream indica−7,2°, ma il codice effettivo usa−3 unità HSV8, cioè−6°: ORBYX segue il codice. Non replica la quantizzazione HSV8 originale: il percorso floating point evita di ridurre implicitamente i canali16bit a8bit. I pixel finali vengono arrotondati alla profondità originale. La correzione cromatica modifica intenzionalmente i valori del composito, mentre i canali sorgenti restano immutati.

Il prodotto è una **resa visiva Natural Color**, non una misura di colore vero calibrata radiometricamente. Non introduce gamma, equalizzazione, contrasto automatico, profili ICC o ridimensionamento impliciti. Non corregge righe perse, rampe di calibrazione, geometria o artefatti dell'acquisizione. L'output mantiene dimensioni e profondità8/16bit; la memoria floating point è limitata a strisce di massimo128righe/1Mpx. Il frame finale e i tre input rimangono comunque in RAM: non è un decoder completamente streaming. Pubblicazione PNG atomica esclusiva da cartella privata, senza sostituzione di file esistenti e con pulizia automatica in caso di errore.

### Verifiche effettive

- Fixture8/16bit: RGB321, rosso/giallo, colori non modificati, grigi/nero/bianco, confronto con formula scalare double indipendente da `cvtColor`, limiti delle strisce, preservazione dei bit bassi su grigio10001, input invariati, dimensioni errate, PNG richiesto e nessuna sovrascrittura.
- CLI reale su fixture sintetiche: output relativo, opzioni strettamente validate, errori italiani e no-overwrite. Worker desktop EN/IT: byte identici alla CLI, destinazione esistente rifiutata senza dialogo bloccante nella modalità di test.
- Acquisizione reale Elektro-L4: due immagini6004×13594×3 uint16, **12684** e **12679** campioni confrontati con `colorsys` Python (RGB/HSV scalare indipendente), scarto massimo0,48 livelli su65535. Sei e undici campioni entro0,001° dai confini discontinui delle regole sono conteggiati separatamente: l'arrotondamento hue float32 può scegliere il lato opposto al double. Non sono stati nascosti errori nel resto del dominio. Il test non certifica tutti i pixel del composito rispetto all'oracolo scalare: usa una griglia regolare di campionamento; la suite sintetica controlla invece ogni pixel delle proprie fixture.
- Ispezione visiva della scena reale: bordo terrestre, nubi e coste riconoscibili; rampe e discontinuità di scansione presenti nel dato mantenute. Nessuna certificazione scientifica del colore o della geolocalizzazione.

Il [workflow RDAS reale](../.github/workflows/real-rdas.yml) ora esporta e verifica entrambi i compositi, conserva report JSON/digest e log, senza redistribuire raw o immagini. `scripts/check_natural_color.py` riproduce il confronto sui compositi esistenti. Risultati remoti registrati in `PROGRESS.md` dopo il completamento del run.

## English

Core, CLI and asynchronous desktop export now implement MSU-GS RGB321 (CH3→R, CH2→G, CH1→B). Generic `composeRgb` retains its existing CH1→R mapping. The command above or the desktop export button processes one aligned VIS product and writes a new PNG exclusively.

The legacy hue rules are applied in normalized floating-point HSV: red+60°, yellow−6°, first matching rule wins, then60% original/40% corrected blend. This follows upstream code rather than its inconsistent−7.2° comment. There is no intermediate8bit quantization for16bit inputs, automatic contrast/gamma/resizing or input mutation. Output rounding preserves the original depth. This is visual Natural Color rendering, not radiometrically calibrated true colour; source scan defects and calibration ramps are retained.

Independent scalar synthetic8/16bit oracles, tiled boundaries, neutral low bits, strict CLI and real asynchronous GUI/CLI byte-equivalence tests have passed locally. Both authentic Elektro-L4 VIS outputs retain6004×13594×3 uint16. A regular grid of12684/12679 samples matches Python coloursys within0.48 native levels;6/11 samples extremely close to discontinuous hue boundaries are explicitly counted separately. Real-scene visual inspection confirms recognizable Earth/cloud/coast features and retained source artefacts. It does not establish scientific colour or absolute geometry.

The real-data workflow now renders both VIS products and records reference reports and digests, excluding imagery from uploaded artifacts. See `PROGRESS.md` for observed remote results; workflow configuration alone is not a passing run.

## Riferimenti / References

- [Algoritmo GPL originale conservato](../third_party/rdas_layer_aligner/imageproc.cpp)
- [Regole e merge RGB321 upstream](../third_party/rdas_layer_aligner/main.cpp)
- [Configurazione ufficiale SatDump MSU-GS](https://github.com/SatDump/SatDump/blob/master/resources/instrument_cfgs/msugs_vis.json): RGB321; non viene dichiarata equivalenza byte-identica ai filtri di SatDump.
