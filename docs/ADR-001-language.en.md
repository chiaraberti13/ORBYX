# ADR-001 — Engine language and migration boundaries

[English](ADR-001-language.en.md) · [Italiano](ADR-001-language.md)

Status: initial decision, subject to review after the algorithmic baseline. Date: 8 November 2026.

Owners: Technical Lead, Programming Languages Architect, Secure Code Auditor, Computer Vision Engineer, Linux Systems Engineer.

## Context

RDAS Layer Aligner uses C++17/OpenCV/JsonCpp. Initial observed risks concern unvalidated inputs, indeterminate output, thread exceptions and unreliable I/O. The primary requirement is to preserve ECC, transformations, precision and SatDump interoperability before replacing the language.

## Comparison

| Option | Safety and concurrency | CPU/GPU and interoperability | Ubuntu/macOS and cost |
|---|---|---|---|
| C++17 with RAII and validation | No guaranteed memory safety; checks, clear ownership, futures and sanitizers mitigate concrete risks | Native OpenCV, UMat and OpenCL calls; no new FFI | Minimal toolchain and baseline changes; C++ security debt remains |
| C++20 | `jthread`, stop tokens and spans help some boundaries; they neither eliminate UB nor automatically validate Mat/ROI | Unchanged OpenCV integration | Assess against distribution toolchains; standard migration requires matrix verification |
| C++23 | Additional utilities and error handling depend on the toolchain; memory safety is not guaranteed | Unchanged native integration | Additional risk with Ubuntu 22.04 compilers, without a measured immediate benefit |
| Partial Rust | Ownership and Send/Sync protect new safe code; OpenCV FFI and drivers remain unsafe boundaries | Bindings or a C++ bridge required; buffer duplication and packaging need measurement | Two toolchains and error models; useful only for an isolated boundary with demonstrated benefit |
| Full Rust | Memory safety for safe code, not for the native backend; no automatic precision guarantee | Risk of algorithmic differences and binding coverage gaps; OpenCL requires a specific audit | Expensive rewrite and validation; high functional risk before datasets exist |
| Python orchestration | Avoids many script memory-management errors; decoders and native libraries remain exposed | Useful for datasets/tests/automation, rather than rewriting pixel loops | Additional dependencies and distribution costs; keep outside the core runtime unless needed |

## Decision

Initially retain C++17 for the engine, fix concrete defects and separate core, application, CLI, GUI and infrastructure. Use Qt 6 only in the GUI layer, keeping the engine headless. The CLI must not require Qt.

Adopt C++20 only for a specific benefit (such as cooperative job cancellation), equivalent tests and builds on supported distributions; do not select C++23 as an initial requirement. Do not start a Rust rewrite before datasets and benchmarks. Later assess Rust for isolated parsing/orchestration if it reduces a measured risk without duplicating OpenCV. Python is permitted for dataset generation, verification and reproducible development automation.

OpenCL must become optional, with CPU as the reference. OpenCV exposing UMat does not establish that every operation is accelerated. Record backends and versions to make benchmarks and regressions reproducible.

## Consequences and review conditions

The project retains native integration and limited costs but requires static analysis, sanitizers, input limits and ownership review. Migrate only after comparing geometric error, pixels in valid regions, peak memory, CPU/GPU throughput, package size and maintenance complexity. Thresholds must be established in the baseline task using real and synthetic data, avoiding arbitrary unmeasured numbers.

This decision does not certify Ubuntu or macOS compatibility or attest unmeasured performance. References and initial defects are recorded in [AUDIT.en.md](AUDIT.en.md).
