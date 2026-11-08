# RDAS reale: campione Elektro-L4 / Real Elektro-L4 acquisition

## Italiano

Il campione pubblico è ora disponibile nella [release rdas-cadu-l4](https://github.com/chiaraberti13/ORBYX/releases/tag/rdas-cadu-l4): `elektro_rdas.cadu`, 769938432 byte, SHA256 `b92ded3c71f1752d12208ed358e0d392e05d2ddbefe97f2e72b69e3c09498d48`. Proviene dall'[issue 2 dell'upstream](https://github.com/Digitelektro/rdas_layer_aligner/issues/2), contributore `eswnl`. La data di acquisizione non è conosciuta. La pubblicazione non fornisce una licenza esplicita di redistribuzione: il raw resta esterno a git e agli archivi sorgenti. Il manifest versionato registra origine, dimensioni e digest; il download esplicito rifiuta payload corrotti/eccessivi e non sovrascrive file esistenti.

### Verifiche eseguite

SatDump `v2.0.0-alpha-1aedd60`, AppImage ufficiale SHA256 `db14d454d5ffd1aa216cafd6e0768225fa1b6cc596c5011b0135471741447ce2`, decodifica realmente il CADU con `pipeline elektro_rdas cadu … --satellite_number=4 --apply_correction=false`. Produce VIS1/VIS2, tre PNG ciascuno, **6004×17200, uint16**, contenuto sensore dichiarato a 10 bit. I due CBOR sono autentici: `instrument=msugs_vis`, `type=image`, tre canali con `transform.type=1` (NONE). Nessuna geometria di proiezione è disponibile nel campione.

Calibrazione CPU a scala esplicita 0,125 riuscita: 5,36 s e picco RSS 863364 KiB nell'ambiente locale Ubuntu24.04/OpenCV4.6/GCC13. Generazione a risoluzione originale riuscita: 12,58 s, 1865716 KiB; sei PNG **6004×13594 a 16 bit**, ritaglio storico di 3606 righe. Confronto con `cv2.warpAffine` su OpenCV4.6: differenza massima **zero su tutti i pixel dei sei canali**. I CBOR per-sensore restano identici ai rispettivi originali. Il comando reale `satdump process product.cbor OUTPUT` ha letto i tre canali di entrambi i prodotti allineati ed è terminato con successo. È una verifica del caricamento prodotti headless, non un test interattivo del visualizzatore.

**Limite geometrico:** questo confronto certifica l'applicazione dell'affine stimata e la conservazione dei canali/metadati, non la precisione dell'affine rispetto a una verità a terra. SatDump segnala correttamente l'assenza di calibrazione/proiezione (`Using Dummy`, `Not a standard projection`). Nessuna geolocalizzazione assoluta è dichiarata verificata. Confrontando scale 0,125 e 0,25, lo scarto massimo delle trasformazioni ai quattro angoli del dominio d'uscita è circa 0,95/1,25 pixel (VIS1 CH2/CH3) e 5,83/5,98 pixel (VIS2): la scelta della scala può influenzare il risultato. Il profilo ottenuto è evidenza di test, **non** una calibrazione L4 universale distribuita come predefinita. ROI e unione dei due VIS richiedono verifica dedicata.

Report completo locale conservato in [REAL_RDAS_EVIDENCE.md](REAL_RDAS_EVIDENCE.md), con digest per ciascun canale e revisione del codice.

### Riproduzione

Occorrono Linux x86_64, curl, timeout, Python con OpenCV4.6/numpy/cbor2 e un binario ORBYX CPU costruito con OpenCV4.6. Prevedere diversi GiB liberi. Nessuna GPU o chiave privata richiesta.

```sh
python3 scripts/fetch_real_rdas.py /percorso/esistente/elektro_rdas.cadu
bash scripts/ci/real-rdas.sh /percorso/assoluto/orbyx /nuova/cartella/test
```

Il secondo comando scarica esplicitamente campione e SatDump, verifica entrambi prima dell'uso, decodifica, calibra, genera a piena risoluzione, confronta i pixel e riapre entrambi i prodotti in SatDump. La cartella finale deve essere nuova; l'AppImage viene estratta senza FUSE e carica soltanto il plugin necessario. Configurazione isolata, auto-processing e aggiornamento Kepler disattivati nel file di configurazione della prova. Il pin della nightly è intenzionale: se l'upstream cambia i byte il test fallisce, senza accettare automaticamente il nuovo binario. Report/log/profilo sono raccolti in `evidence`; immagini e raw non vengono caricati come artifact CI.

Il [workflow dedicato](../.github/workflows/real-rdas.yml) esegue questa stessa procedura su Ubuntu24.04, conserva i risultati 14 giorni e resta avviabile manualmente. Verifica remota completata con successo sul commit `de60721671061040fae6af46fb7fec4ac2133368`: [run42514688645](https://github.com/chiaraberti13/ORBYX/actions/runs/42514688645). Report, log e profilo sono negli artifact; il report locale è conservato anche in git.

### Calibrazione con memoria contenuta

`calibrate --calibration-scale FLOAT` e `RunOptions::calibrationScale` selezionano esplicitamente una scala `(0,1]`; il valore predefinito **1** preserva la risoluzione ECC originale. I canali convertiti a 8 bit per ECC vengono ridotti con INTER_AREA; l'offset iniziale tiene conto dell'altezza effettiva arrotondata. L'affine stimata viene riportata alle coordinate originali tramite `S^-1 W S`, incluse le convenzioni dei centri dei pixel di OpenCV; gli offset nativi vengono poi ripristinati. `generate` continua a usare i canali originali a 8/16 bit. Nessuna riduzione silenziosa. Oltre 16 Mpx i due fit ECC sono serializzati per ridurre il picco di memoria; non è un limite massimo di RAM garantito.

Regressioni sintetiche aggiuntive: traslazione nota con larghezza dispari e banda di guardia (entro 1 pixel), affine nota con scala/shear/traslazioni su fixture16bit (errore <0,75 pixel in due punti del contenuto), valori non finiti/fuori intervallo, modalità errata. Una fixture priva di margini mostrava un bias da taglio dei dettagli ai bordi nella stima ridotta: non è stata usata come prova di precisione. Non confrontare interpolazioni tra versioni maggiori: OpenCV5 produce differenze rispetto a OpenCV4.6 su questo campione; il controllo richiede esplicitamente 4.6.

## English

The pinned public release asset is an actual Elektro-L4 CADU acquisition, not a generated fixture. The explicit downloader verifies its exact size/SHA256 and publishes exclusively. Source attribution and unknown acquisition time/redistribution licence are recorded in the manifest; raw data is excluded from git/source packages.

Official SatDump v2.0.0-alpha-1aedd60 decodes six 6004×17200 uint16 channels and genuine per-VIS CBOR. CPU ECC at explicit scale0.125 and native-resolution generation have completed. Six 6004×13594 outputs match an independent OpenCV4.6 affine reconstruction exactly; both products load in the actual SatDump headless product processor. CBOR transforms are NONE and per-sensor metadata is preserved.

This proves real-input execution, native-depth affine application and product loading. It does not establish ground-truth alignment accuracy, absolute geolocation, VIS merge ROIs or Natural Color quality. Source metadata has no usable projection/calibration; SatDump's warnings are retained. Estimates at scales0.125/0.25 differ by up to about6 native pixels at output corners, so the generated profile is not a universal L4 default.

The commands above reproduce the complete networked check. Both asset hashes are checked before use; a changed nightly fails closed. The workflow retains evidence for14days, excluding raw/decoded imagery. `--calibration-scale` is opt-in, defaults to1, uses rounded resize dimensions/pixel-centre-aware affine lifting, and never changes native generation depth or silently downsamples. Large ECC fits run serially. Reference interpolation is pinned to OpenCV4.6; cross-major pixel equivalence is not claimed.
