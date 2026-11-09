<p align="center"><img src="assets/banner.svg" alt="ORBYX — Satellite Imaging Studio" width="100%"></p>

<p align="center"><a href="README.md">🇮🇹 Italiano</a> · <a href="README.en.md">🇬🇧 English</a></p>

<p align="center">
<a href="https://github.com/chiaraberti13/ORBYX/actions/workflows/ci.yml"><img src="https://github.com/chiaraberti13/ORBYX/actions/workflows/ci.yml/badge.svg" alt="Build and regression"></a>
<img src="https://img.shields.io/badge/status-sviluppo_iniziale-F2C94C?style=flat-square" alt="Sviluppo iniziale">
<img src="https://img.shields.io/badge/category-SATELLITE_IMAGING-22D3EE?style=flat-square" alt="Immagini satellitari">
<img src="https://img.shields.io/badge/stack-C%2B%2B17_%7C_Qt_6-8B949E?style=flat-square" alt="C++17 e Qt 6">
<img src="https://img.shields.io/badge/languages-EN_%7C_IT-8B5CF6?style=flat-square" alt="Inglese e italiano">
<img src="https://img.shields.io/badge/licence-GPL--3.0-2EA043?style=flat-square" alt="GNU GPL versione 3">
</p>

> Allinea i canali. Rivela la Terra. Un ambiente desktop e headless indipendente per allineare i canali satellitari RDAS.

<p align="center"><a href="ROADMAP.md">Roadmap</a> · <a href="PROGRESS.md">Verifiche</a> · <a href="docs/AUDIT.md">Audit</a> · <a href="SECURITY.md">Sicurezza</a> · <a href="THIRD_PARTY_NOTICES.md">Attribuzioni</a></p>

## Start here — Ubuntu

ORBYX is a desktop application for aligning satellite images and exporting Natural Color. No CMake knowledge or personal paths are required. Run one command block at a time and stop if a step fails.

### 1. Open a terminal

Press Ctrl+Alt+T. For an existing checkout, open its folder in the file manager and choose Open in Terminal.

### 2. Download or update

First time:

```bash
git clone https://github.com/chiaraberti13/ORBYX.git
cd ORBYX
```

If Git is missing, install it and retry:

```bash
sudo apt-get -o DPkg::Lock::Timeout=900 install -y git
```

Already downloaded? Run this from the project folder instead of cloning again:

```bash
git pull
```

### 3. Install

```bash
bash install.sh --lang en
```

Enter your Ubuntu password when asked; invisible password characters are normal. Do not run the whole script with sudo. Setup waits for automatic package updates; never remove locks or kill package managers. Configuration, compilation, tests and user installation are automatic and may take several minutes.

Proceed only after **Installation completed** appears. Otherwise check **logs**.

### 4. Open the desktop

```bash
bash start.sh
```

Reuse this command on later launches. For updates, close the application, run `git pull`, then rerun the installer.

### No data yet? Try the demo

```bash
bash demo.sh --lang en
```

Press **Run image demo** in the app (Ctrl+Shift+D). Six bundled real JPEG previews, three SX and three DX, are registered and composed into one image. No manual paths are needed. The command opens the same built-in demo; `bash demo.sh --no-open` runs it offscreen into demo-output, with logs. These are 8-bit previews, not absolute geometry or radiometrically calibrated true colour. [Source details](assets/demo-jpeg/SOURCE.md).

### 5. Process your data

You need RDAS products already decoded by SatDump, including both VIS sensors and their channels. ORBYX does not receive satellite signals. You can install/open it without data.

1. Select the decoded input folder.
2. Choose the satellite and suitable calibration profile.
3. Choose a new output directory; existing outputs are never overwritten.
4. Select Align channels, then Start processing.
5. Wait for Completed and inspect the preview/activity panel.

For L4, first run Calculate calibration into a new destination. Select the resulting calibration.json with the profile chooser, switch to Align channels and use another new destination. Merge regions still need manual validation; see [real-data calibration](docs/REAL_RDAS.md).

### 6. Export Natural Color

Click Export Natural Color…, choose one aligned VIS product and a new PNG name. Depth stays 8/16-bit. This is visual rendering, not scientifically calibrated true colour.

### 7. Troubleshooting

- Busy package manager: wait. If setup times out, retry later; do not remove locks.
- Missing CMake/CTest/dependencies: run the full installer, not just CMake installation.
- Incomplete installation/missing executable: complete setup, then use `bash start.sh`.
- Existing output: choose a new destination/name.
- Other errors: open logs and find the latest complete log and matching .errors.log.

Logs stay local, are ignored by git and are not automatically uploaded. [Installation details](docs/INSTALLATION.md).

## Optional CLI

```bash
bash orbyx.sh --help
bash orbyx.sh --version
```

Beginners can choose inputs and outputs in the desktop instead.

## Project information

[Roadmap](ROADMAP.md) · [Observed progress](PROGRESS.md) · [Real validation status](docs/VERIFICATION_STATUS.md) · [Batch](docs/BATCH.md) · [Natural Color](docs/NATURAL_COLOR.md) · [Security](SECURITY.md).

Actual GPU execution and absolute geometry remain uncertified. ORBYX derives from [Digitelektro's RDAS Layer Aligner](https://github.com/Digitelektro/rdas_layer_aligner), using C++17/OpenCV and Qt6. [GPLv3](LICENSE); [third-party notices](THIRD_PARTY_NOTICES.md). Originals remain separate from results.
