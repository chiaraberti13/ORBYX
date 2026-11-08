# Evidenza RDAS reale / Real RDAS evidence

Esecuzione locale completa di `scripts/ci/real-rdas.sh`, 8 novembre2026, Ubuntu24.04 x86_64, OpenCV4.6. Non è un golden output universale: i digest descrivono questa esecuzione; la calibrazione ECC può dipendere da versione, hardware e scala. Report conservato senza immagini satellitari. / Complete local real-data execution; hashes describe this run, not a universal golden output. Ground-truth geometry is not verified.

```json
{
  "opencv_reference_version": "4.6.0",
  "geolocation_verified": false,
  "scope": "Native-resolution affine application and unmodified source metadata; no ground-truth geometry.",
  "profile_sha256": "52d451d6be2ef9ae86531fb84450a69cca48022504ef81a7b32e32277b7e8d98",
  "products": [
    {
      "vis": 1,
      "channel": 1,
      "input_sha256": "0a8d408d6f26ed5e8c992bb25a7ef401cc1542f839c53a0c6ff612a051a71486",
      "output_sha256": "61de61dca909e7573fc6a159cab08d8db36ad3e6795d0738fcd855d4f99e7429",
      "shape": [
        13594,
        6004
      ],
      "depth": 16,
      "max_affine_reference_difference": 0,
      "cbor_transform_type": 1
    },
    {
      "vis": 1,
      "channel": 2,
      "input_sha256": "a0231012dcfa1d174536848e0c0092d07df289914941f06b699d3e90f80cbdb5",
      "output_sha256": "886fb89960e9895af2567d75dc85d03a1de98a3d8901acf826395ed8712cc469",
      "shape": [
        13594,
        6004
      ],
      "depth": 16,
      "max_affine_reference_difference": 0,
      "cbor_transform_type": 1
    },
    {
      "vis": 1,
      "channel": 3,
      "input_sha256": "ecdf5c1ea1973042e3dd8bfa35f93d427dbef1e7122908b86478dd904166b988",
      "output_sha256": "7f117bae9d7042cccf2961978a62493c808fcce01c06d7e3f20644efb1bb0110",
      "shape": [
        13594,
        6004
      ],
      "depth": 16,
      "max_affine_reference_difference": 0,
      "cbor_transform_type": 1
    },
    {
      "vis": 2,
      "channel": 1,
      "input_sha256": "6d24e24235f44bcca355797ed9d221cbb95cae3ad2756cb27540cc4c0ba00df5",
      "output_sha256": "46c77656a0f68ed5a399e9c9212fe55b6a175dac6786c11c1cccc32faa834101",
      "shape": [
        13594,
        6004
      ],
      "depth": 16,
      "max_affine_reference_difference": 0,
      "cbor_transform_type": 1
    },
    {
      "vis": 2,
      "channel": 2,
      "input_sha256": "ca5e83270b4db5d3768b14d2e161566ec520f860eb3ba75fe02a0dc0bf1e498d",
      "output_sha256": "e1c3a32a62facab77d15d324617576e71a902302212c8bd59e54568234153e3c",
      "shape": [
        13594,
        6004
      ],
      "depth": 16,
      "max_affine_reference_difference": 0,
      "cbor_transform_type": 1
    },
    {
      "vis": 2,
      "channel": 3,
      "input_sha256": "b8f30d2b73cc770797066d90cdf52839f9cfc1946947e57c95c3c15abf2681a3",
      "output_sha256": "99d6c4d6804048e0508b16b512c847f5617ca16fce42f23c78a82eccc20b6638",
      "shape": [
        13594,
        6004
      ],
      "depth": 16,
      "max_affine_reference_difference": 0,
      "cbor_transform_type": 1
    }
  ],
  "source_revision": "de60721671061040fae6af46fb7fec4ac2133368",
  "sample_sha256": "b92ded3c71f1752d12208ed358e0d392e05d2ddbefe97f2e72b69e3c09498d48",
  "satdump_appimage_sha256": "db14d454d5ffd1aa216cafd6e0768225fa1b6cc596c5011b0135471741447ce2",
  "calibration_scale": 0.125,
  "original_hashes_unchanged_after_pipeline": true,
  "satdump_headless_reimport": {
    "VIS1": "loaded all 3 channels, exit 0",
    "VIS2": "loaded all 3 channels, exit 0"
  }
}
```
