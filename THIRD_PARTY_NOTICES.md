# Third-party notices

ORBYX is an independent derivative of [RDAS Layer Aligner](https://github.com/Digitelektro/rdas_layer_aligner), by Digitelektro and its contributors, snapshot `6aee5b4ba96b528389890ea6442fc96a7344a6ff`.

The original sources, calibration coefficients, README and full GPLv3 text are preserved under `third_party/rdas_layer_aligner`. ORBYX refactors the alignment workflow and adds validation, transactional output, tests and a desktop interface. Original example photographs are not redistributed. The dormant TPS kernel is archived for provenance and is not built or executed. Upstream README image references describe upstream assets and do not refer to bundled photographs.

The derivative is distributed under GNU GPL version 3; see LICENSE. No claim of endorsement or affiliation is made. The upstream text does not clearly select an “or later” grant; this project does not infer one from the example appendix.

Build dependencies are supplied by the operating system, not vendored here:

| Component | Role | Licence family (verify exact packaged version before release) |
|---|---|---|
| OpenCV | Image codecs, affine warp, ECC, optional OpenCL backend | Apache-2.0 for 4.5+; third-party codecs have separate terms |
| JsonCpp | Calibration JSON | MIT / public-domain grant according to upstream distribution |
| Qt 6 base | Optional desktop interface | LGPLv3 / GPLv2+ / commercial options, depending on component |
| C++ runtime and system libraries | Execution | Distribution-specific notices and runtime exceptions |

A binary release must include the corresponding source and an inventory of actual linked libraries, codec notices and applicable licences. This source inventory is not a binary SBOM or a completed legal audit of future packages.
