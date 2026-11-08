# Synthetic simulation / Simulazione sintetica

Date / Data: 2026-11-08.

## What it is / Che cos'è

`scripts/orbyx_simulate.py` generates a synthetic MSU-GS-shaped acquisition
(two VIS directories, three 8-bit grayscale PNG channels each with the real
CH2=3606 / CH3=1804 vertical offsets and a placeholder `product.cbor`), injects
a **known** horizontal shift into the misaligned channels, then drives the real
ORBYX engine through a full `calibrate` → `generate` cycle and checks that the
known offsets and shift are recovered (within 1 px) and that aligned products
are produced. The PNGs are written by a hand-rolled encoder, so the run also
exercises the decoder on third-party-encoded input.

`scripts/orbyx_simulate.py` genera un'acquisizione sintetica in forma MSU-GS,
inietta uno spostamento orizzontale **noto**, esegue l'intero ciclo reale
`calibrate` → `generate` del motore e verifica che gli offset noti e lo
spostamento siano recuperati entro 1 px e che i prodotti allineati siano creati.

## Run / Esecuzione

```sh
cmake --build build
scripts/orbyx_simulate.py --orbyx build/orbyx           # auto temp dir, cleaned on success
scripts/orbyx_simulate.py --orbyx build/orbyx --out demo  # keep the generated dataset
```

It also runs as the `simulation-flow` CTest group. Verified on 2026-11-08
(Ubuntu 24.04.5): calibration recovered CH2 ty≈3606, CH3 ty≈1804 and the
injected shift, and both `MSUGS_VIS-1_Aligned` and `MSUGS_VIS-2_Aligned` were
produced.

## What it does NOT prove / Che cosa NON dimostra

This is a **simulation on synthetic data**. It proves the end-to-end pipeline
mechanics (dataset layout, PNG decoding, ECC calibration recovering known
offsets, warp/crop generation, product and CBOR handling). It does **not**
verify:

- radiometric or geometric accuracy on **real** RDAS acquisitions;
- calibration quality against genuine satellite passes;
- **SatDump** reimport geometry (the CBOR is copied unchanged, never validated).

È una **simulazione su dati sintetici**: non verifica la precisione su
acquisizioni reali, la qualità di calibrazione su passaggi satellitari veri, né
l'interoperabilità geometrica con **SatDump**.

These require real, redistributable RDAS datasets with provenance and a SatDump
environment, and remain the open acceptance criteria of roadmap **M3-01** (and
the dependent M3-04 Natural Color and M2-04 folder monitoring). The upstream
project ships only example **output** composites (`img/*.jpg`), not raw input
datasets, so no real input data is available here.
