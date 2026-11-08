# Changelog

## 0.1.0 — Initial development, 2026-11-08

- Pinned upstream audit and preserved original source/calibration baseline under GPLv3.
- Added operational roadmap, language ADR, original banner and bilingual README.
- Introduced independent core/CLI and optional asynchronous Qt 6 desktop.
- Corrected raw-channel fallback, VIS2 CBOR provenance, uninitialized merge padding, input bounds and worker exception propagation.
- Added separate transactional output, synthetic regressions and CI configuration.
- Added durable bilingual batch execution, verified resume and 14 Python regressions.
- Verified GUI worker output against CLI byte-for-byte, drag/drop and channel previews.
- Passed CI builds/tests and experimental DEB install smoke on Ubuntu22.04/24.04/26.04 x86_64.
- Release validation with real satellite data, SatDump and the complete platform matrix remains open.
