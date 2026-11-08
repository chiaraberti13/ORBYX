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
