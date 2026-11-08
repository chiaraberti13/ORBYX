# Initial RDAS Layer Aligner audit

[English](AUDIT.en.md) · [Italiano](AUDIT.md)

Date: 8 November 2026. Functional owners: Technical Lead, Computer Vision Engineer, Secure Code Auditor, Open Source Licensing Specialist.

## Provenance and scope

Reference: [Digitelektro/rdas_layer_aligner](https://github.com/Digitelektro/rdas_layer_aligner), commit `6aee5b4ba96b528389890ea6442fc96a7344a6ff` (16 August 2026). Static review of `main.cpp`, `parameterparser.*`, `configstorage.*`, `imageproc.*`, `CMakeLists.txt`, `README.md`, `LICENSE`, `config.json` and `kernels/tps.cl`. This document describes upstream code; it does not certify the absence of vulnerabilities or the correctness of the ORBYX derivative.

The engine uses C++17, OpenCV and JsonCpp. Linux CMake requires OpenCL even though the application uses UMat, which can run on CPU. The examined upstream scope contains no Qt GUI, test suite, CI, packaging or application update mechanism. The examined application sources do not appear to invoke the TPS kernel.

## Functional contract to preserve

| Area | Upstream behavior | Required verification in the derivative |
|---|---|---|
| Input | SatDump live directory containing `MSUGS_VIS1` and `MSUGS_VIS2`, each with three `MSUGS-VIS-N.png` files | Valid PNGs, consistent dimensions, depth and channels; unchanged inputs |
| Satellites | Help lists L2/L3/L4/M1/M2; JSON coefficients cover L2/L3/L5/M1/M2; parser accepts any nonempty string | Preserve L5 present in the data; do not claim L4 coefficients are already available |
| Calibration | Affine ECC, 500 iterations, epsilon 1e-7; pre-shift CH2=3606 and CH3=1804 | Synthetic and real datasets, geometric error and matrix regression |
| Generation | Linear affine warp with inverse mapping, unchanged CH1, removal of the final 3606 rows | Equality within the valid region and 8/16-bit preservation |
| Merge | Left/right ROIs from config; fallback when ROIs are absent | ROI bounds, deterministic content and correct CBOR provenance |
| Output | PNG and `product.cbor` for SatDump reimport | Actual verification in SatDump, beyond copying bytes |
| Natural Color | HSV function exists but its call is explicitly disabled in main | New functionality requiring validation; do not describe it as already functional upstream |
| CLI | `-s/--sat`, `-m/--mode`, `-d/--directory`, `-n/--nc` | Preserve syntax; make help and defaults effective |

## Static review findings

Priorities are internal project priorities, not CVSS scores. Evidence requires targeted tests before each correction can be considered closed.

| ID | Priority | Upstream evidence | Consequence and closure criterion |
|---|---|---|---|
| A01 | High | `mergeLeftRight` creates an uninitialized UMat and fills only the ROIs | Indeterminate remaining pixels, potential memory disclosure: zero initialization or exact dimensions, boundary tests |
| A02 | High | `alignChannels` uses `rows - 3606` without checks; JSON ROIs are not validated | Assertions/crashes and malformed inputs: reject small images, negative or out-of-bounds ROIs before warping |
| A03 | High | ECC runs in `std::thread` without catching exceptions | `std::terminate` upon non-convergence: controlled propagation, guaranteed joins, readable error |
| A04 | High | No explicit PNG dimension or allocation limits | Memory exhaustion from large images: pixel/memory budgets and adversarial input tests; the decoder remains a dependency requiring updates |
| A05 | Medium | The no-ROI branch saves `VIS*_channels`, not `VIS*_alignedChannels` | Outputs appear aligned but contain originals: verify transformed pixels in the fallback |
| A06 | Medium | VIS2 fallback copies CBOR from `VIS1_directory` | Incorrect product metadata: tests with distinct CBOR for the two sensors |
| A07 | Medium | `parseArgs` reads pairs and ignores the last argument; declared default mode does not work | Invalid options ignored and unusable help: validate options, values, duplicates and defaults; no processing for help/version |
| A08 | Medium | JSON config converted without schema, range or finiteness checks | Invalid matrices and inconsistent configuration: validate types, finite coefficients and ROIs before use |
| A09 | Medium | Config saved by direct truncation; write error only printed | Configuration loss and false success: local atomic replacement, propagated errors and I/O failure tests |
| A10 | Medium | `imread` and `imwrite` results unchecked; exceptions unhandled in main | Partial outputs, crashes: missing/corrupt inputs and unwritable filesystems must produce nonzero exit codes |
| A11 | Medium | Existing output overwritten, CBOR removed before copying | Interruption can lose output; explicitly define overwrite policy and atomic publication |
| A12 | Low | `CMakeLists.txt` first searches for optional OpenCL, then requires it on Linux; OpenCV is not REQUIRED | CPU builds unnecessarily blocked: OpenCV REQUIRED and optional OpenCL backend with CPU tests |
| A13 | Low | Hue wrapping corrects only once by ±180 | Large shifts produce incorrect hue: validated ranges or full modulo; function currently not exposed by main |
| A14 | Low | TPS uses pointer casts to reinterpret floats and unchecked index arithmetic | Dormant kernel must not be enabled without bounds audit and OpenCL tests; prefer `as_uint/as_float` builtins |

The examined application code invokes no shell processes, downloads or network services. No evidence of injection or use-after-free emerged in the reviewed paths; this does not replace sanitizers and fuzzing. Images, JSON and the filesystem are the main trust boundaries. Symlinks, output paths and interprocess concurrency require dedicated verification before folder monitoring and batch processing.

## Licensing and distribution

Upstream includes the GNU GPL version 3 text. The mere presence of the phrase "or any later version" in the license's final example section does not establish that upstream selected GPL-3.0-or-later. Preserve the full text and attributions, avoid a more permissive license for the derivative, and record origin and modifications. Identify ORBYX as an independent derivative without suggesting affiliation.

Binary license compatibility depends on the actual distributed versions and artifacts of OpenCV, JsonCpp, Qt, the OpenCL runtime and associated codecs. Inventory licenses, notices and corresponding sources during packaging; do not infer dependency licenses from names alone. Upstream demonstration images are unnecessary for the engine and must not be reused without checking provenance.

## Verification strategy

Before refactoring: preserve snapshots of the original code/config and create reproducible synthetic datasets (identity, known translation, absent ROIs, invalid ROI, 8/16-bit images, corrupt files). Maintain an algorithmic baseline over the valid region: outputs affected by A01/A05/A06 must not become correctness oracles. Add redistributable real datasets with consent and metadata, explicitly retaining the limitation while they remain unavailable.

Run unit/integration tests, AddressSanitizer and UndefinedBehaviorSanitizer, static analysis and parser/config fuzzing. Measure CPU/OpenCL using the same dataset and documented tolerances; do not promise GPU acceleration without benchmarks. Ubuntu 22.04/24.04/26.04, macOS and other distributions still require separate build and execution verification, beyond theoretical compilability.
