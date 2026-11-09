# Verifica GPU reale / Real GPU verification

## Italiano

`orbyx_gpu_verify` è un eseguibile diagnostico costruito con i test. Conta piattaforme/dispositivi OpenCL, richiede un dispositivo di tipo **GPU** disponibile e selezionato, compila ed esegue un kernel OpenCL esplicito su4096float e controlla ogni risultato. Un dispositivo CPU OpenCL non soddisfa il requisito; `setUseOpenCL(true)` da solo non è considerato prova hardware.

```sh
OPENCV_OPENCL_DEVICE=:GPU: ./build/orbyx_gpu_verify
ctest --test-dir build -R opencl-gpu -V
```

Codici:0 = kernel GPU reale verificato e confronto UMat completato;77 = hardware/backend GPU assente o non selezionato (**test saltato, nessuna verifica GPU superata**);1 = errore reale di compilazione/esecuzione/calcolo sul percorso disponibile. CTest registra77 come `Skipped`, non converte un guasto GPU in un successo. L'output JSON conserva piattaforma/dispositivo/driver e ambito effettivo.

Dopo il kernel esplicito, un'affine8/16bit richiesta a OpenCV/UMat viene confrontata alla CPU, tolleranza1livello. OpenCV può fare fallback per una singola operazione: questo confronto non dimostra da solo il dispatch dell'affine su GPU, non è un benchmark e non certifica il motore end-to-end. Il report mantiene `whole_engine_gpu_verified=false` e `ecc_gpu_verified=false`. Servono ancora profilazione del percorso di generazione effettivo e benchmark/equivalenza su una macchina con GPU reale.

La calibrazione ORBYX usa `Mat`/`findTransformECC`: è CPU anche se esiste OpenCL. Il log ora lo dichiara esplicitamente e non seleziona OpenCL durante la calibrazione. Generazione UMat e correzione cromatica sono percorsi distinti: Natural Color corrente è CPU.

### Risultato nell'ambiente locale

8novembre2026, Ubuntu24.04 x86_64, OpenCV4.6:0piattaforme,0dispositivi,0GPU, `have_opencl=false`, uscita77/`skipped`. Non è stato simulato hardware e non è stato acquistato un runner GPU. M3-03 resta aperto per le prove hardware.

## English

The dedicated CTest executable requires an actual available OpenCL GPU, executes an explicit kernel and checks all4096results. CPU devices/fallback never count as GPU proof. Exit77 means unavailable hardware and is recorded as a skipped test; actual kernel or numeric failures return1. An additional requested-UMat affine8/16bit comparison is scoped separately because OpenCV can internally fall back per operation. It does not certify full engine dispatch or performance.

Local execution found no OpenCL platform/device and skipped correctly. ECC is a CPU Mat estimator; the engine now logs this explicitly. Genuine generation profiling/equivalence and GPU benchmarks remain open until actual hardware is available. No GPU result is inferred from a green standard CPU CI run.

## Fallback automatico del motore su GPU divergente / Automatic engine fallback on a divergent GPU

Da un log utente reale (GPU «Intel(R) Graphics», driver 26.05.037020) la stessa
affine richiesta a OpenCL divergeva dalla CPU di **157 livelli** su 8 bit. Poiché
`generate` esegue `warpAffine` su `UMat` (quindi su GPU quando OpenCL è attivo),
un dispositivo con questa divergenza avrebbe prodotto prodotti allineati errati.
Il motore ora verifica la GPU prima di usarla: `openclAffineStatus()` enumera i
dispositivi e confronta l'affine CPU/GPU a 8 e 16 bit (stessa tolleranza di un
livello del verificatore). La GPU viene usata **solo** se riproduce la CPU;
altrimenti `generate` resta su CPU e logga «OpenCL GPU affine diverges from CPU;
using CPU backend». La scelta riguarda il backend per correttezza: trasformazioni
e algoritmo non cambiano. Nessuna GPU presente ⇒ «Backend: CPU» come prima.

Di conseguenza il test `opencl-gpu` è una **diagnostica**, non un gate di
installazione: ha `LABELS gpu`, `install.sh` esegue il gate funzionale con
`ctest --label-exclude gpu` e la verifica GPU a parte (`orbyx_gpu_diagnostic`),
che riporta verificata/assente/divergente in EN/IT senza mai interrompere
l'installazione. Una GPU vecchia o divergente non blocca più un'installazione
CPU funzionante, e i risultati restano corretti grazie al fallback del motore.

Since a real user log showed the requested OpenCL affine diverging from the CPU
by 157 levels on an Intel integrated GPU, and `generate` runs `warpAffine` on a
`UMat`, the engine now probes the GPU once per process and uses it only when it
reproduces the CPU affine within one level; otherwise it stays on the CPU and
logs the fallback. The `opencl-gpu` test is therefore a diagnostic (`LABELS gpu`)
that `install.sh` runs separately and never treats as an install blocker.

## Inventario remoto aggiuntivo / Additional remote inventory

[Run42517297975](https://github.com/chiaraberti13/ORBYX/actions/runs/42517297975), commitf2bf5b5: chiamate native Apple OpenCL su runner standard pubblico macos-15-intel hanno restituito **0GPU disponibili**. La fase successiva di build/esecuzione del verifier è stata saltata. È un inventario hardware reale, non un pass GPU e non una prova di compatibilità ORBYX/macOS. Nessun runner large/xlarge o servizio a pagamento è stato attivato. / Native Apple OpenCL inventory returned zero available GPUs; project verification was skipped, with no GPU or macOS compatibility claim.

Su Fedora/OpenCV4.13 l'enumerazione può generare eccezione se il runtime manca: corretto il controllo `haveOpenCL()` prima della chiamata. La CI Fedora/VM successiva registra correttamente lo skip e prosegue i test applicativi.
