# Fuzzing / Fuzzing

Date / Data: 2026-11-08.

## Scope / Perimetro

`tests/fuzz_parsers.cpp` is a libFuzzer harness over the untrusted-input
parsing surface reached from the public core API: the bounded PNG snapshot
read, the hand-rolled IHDR validation and dimension budget, and the decoder,
all reached through `inspect()`; plus the localisation catalogue reached
through `translate()`. It is built with Clang and
`-fsanitize=fuzzer,address,undefined`.

`tests/fuzz_parsers.cpp` è un harness libFuzzer sulla superficie di parsing di
input non fidato raggiungibile dall'API pubblica: lettura PNG limitata,
validazione IHDR e budget dimensioni, decoder (via `inspect()`), e il
catalogo di localizzazione (via `translate()`). Compilato con Clang e i
sanitizer address/undefined.

## Build and run / Compilazione ed esecuzione

```bash
cmake -S . -B fuzz -DORBYX_BUILD_GUI=OFF -DORBYX_FUZZ=ON \
      -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build fuzz --target orbyx_fuzz_parsers
ASAN_OPTIONS=detect_leaks=0 ./fuzz/orbyx_fuzz_parsers -max_total_time=60 -max_len=8192
```

`ORBYX_FUZZ` requires Clang; the CMake configuration fails fast with any other
compiler. A bounded CTest group `fuzz-parsers-smoke` (`-runs=8000`) builds the
harness and proves it survives several thousand mutations without a sanitizer
finding; CI runs an extended `-max_total_time=60` campaign on `ubuntu-24.04`.

`ORBYX_FUZZ` richiede Clang; la configurazione CMake fallisce subito con altri
compilatori. Il gruppo CTest `fuzz-parsers-smoke` costruisce l'harness ed
esegue alcune migliaia di mutazioni; la CI esegue una campagna estesa a tempo.

## Verified / Verificato

Local run on Ubuntu 24.04.5 (Clang 18.1.3): 20000 runs completed with no crash,
leak or UB report; the `fuzz-parsers-smoke` CTest group passes.

Esecuzione locale su Ubuntu 24.04.5 (Clang 18.1.3): 20000 run senza crash, leak
o segnalazioni UB; il gruppo CTest `fuzz-parsers-smoke` passa.

## Not yet done / Da fare

- JSON calibration-profile and full `run()` pipeline fuzzing (needs a fixture
  input tree and a parser entry point that is not currently public).
- A persisted seed corpus and a scheduled, long-running campaign.
- Native codecs (OpenCV) are distribution libraries and are not instrumented:
  a fault inside them still aborts the process but their internals are out of
  scope here. GPU/OpenCL performance and benchmark work (the rest of M3-03)
  remain open and require real hardware.

## Estensione profili e generazione / Profile and generation extension

Il medesimo harness ora riproduce byte PNG arbitrari e, per input che iniziano con oggetto/array JSON, li consegna al vero parser di profilo e a `orbyx::run` in modalità generazione CPU. Le immagini sono fixture sintetiche fisse e piccole; vengono esercitati parsing JSON, matrici affine, ROI, staging, pubblicazione e scrittura prodotti. Il CBOR delle fixture è un segnaposto dichiarato: non si tratta di interoperabilità SatDump reale.

Un corpus iniziale versionato include PNG valido, profili validi separati/uniti e profili invalidi; il dizionario aiuta le mutazioni di chiavi, numeri limite e intestazioni. CMake copia i semi nella build; il fuzzer estende solo questa copia. Crash riproducibili e corpus esteso sono conservati come artifact CI. La cartella temporanea del processo è privata e casuale, senza il precedente nome PNG prevedibile; cleanup RAII.

La riproduzione locale di tutti i semi tramite il driver dell'harness è passata. Campagna libFuzzer/ASan/UBSan remota da osservare prima di dichiarare il nuovo percorso coverage-guided verificato. Restano fuori dall'instrumentazione le librerie native distro, e il ramo di stima ECC non è fuzzato da questo harness. Nessuna campagna finita prova l'assenza di vulnerabilità.

The harness now drives structured JSON seeds through the actual profile reader and CPU generation API on fixed small synthetic images, covering affine/ROI validation and transactional product writing. Source seeds remain versioned and unchanged; build-local mutations and reproductions are retained separately. Temporary storage uses a random private directory. Local corpus replay passed; observed sanitizer/coverage-guided CI evidence is required for the extended campaign. Distribution libraries and the ECC estimator are not instrumented/fuzzed here; no absence-of-vulnerabilities guarantee is implied.
