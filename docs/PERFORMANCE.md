# CPU performance / Prestazioni CPU

Date / Data: 2026-11-08.

## Benchmark

`scripts/orbyx_benchmark.py` generates a synthetic MSU-GS dataset and times the
real `calibrate` and `generate` CLI runs end-to-end on the CPU backend:

```sh
cmake --build build
scripts/orbyx_benchmark.py --orbyx build/orbyx --iterations 3
```

## Measured / Misurato

Real CPU wall-clock, Ubuntu 24.04.5 x86_64 (session host), image 384×4022
(1.54 Mpx), 3 iterations, `--cpu`:

| Stage | min | median |
|---|---|---|
| calibrate | ~32.1 s | ~33.6 s |
| generate | ~81 ms | ~85 ms |

Observations: `generate` (affine warp + crop + encode) is fast; `calibrate` is
dominated by the ECC solver (500 iterations, preserved from upstream) and scales
with image area, so on real ~80 Mpx acquisitions calibration is expected to take
substantially longer. This is a preserved-algorithm cost, not a regression.

Osservazioni: `generate` è veloce; `calibrate` è dominato dall'ECC (500
iterazioni, preservate dall'upstream) e cresce con l'area, quindi su acquisizioni
reali (~80 Mpx) la calibrazione sarà molto più lenta. È un costo dell'algoritmo
preservato, non una regressione.

## Scope / Limiti

These are **real CPU measurements** on the host, not a GPU/OpenCL comparison and
not a real-acquisition benchmark. GPU/OpenCL equivalence and throughput
benchmarks (roadmap M3-03) require real GPU hardware and are not performed here.
The benchmark is a documented tool, not a CI gate (its ECC cost makes it too slow
for every CI run).

Sono **misure CPU reali**, non un confronto GPU né un benchmark su dati reali.
L'equivalenza GPU/OpenCL (M3-03) richiede hardware GPU reale e non è svolta qui.
Il benchmark è uno strumento documentato, non un gate di CI.
