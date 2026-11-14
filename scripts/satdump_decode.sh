#!/usr/bin/env bash
# Decode an Elektro RDAS CADU into MSU-GS VIS PNGs using the CURRENT SatDump nightly.
#
# Unlike the pinned CI test (scripts/ci/real-rdas.sh), this user-facing helper always
# fetches the LATEST SatDump AppImage, so it keeps working when the nightly changes.
# It does not hard-fail on a fixed SHA256; instead it verifies the download is a real,
# non-trivial ELF/AppImage (not an HTML error page) and records the obtained SHA256 so
# the exact binary stays traceable. ORBYX itself never decodes the CADU: this is an
# explicit, user-run step, outside the ORBYX GUI workflows.
#
# Usage: bash scripts/satdump_decode.sh CADU_FILE OUTPUT_DIR [SATELLITE_NUMBER]
#   e.g. bash scripts/satdump_decode.sh ~/elektro_rdas.cadu ~/decoded 4
set -euo pipefail
if [[ $# -lt 2 || $# -gt 3 ]]; then
  echo 'Usage: satdump_decode.sh CADU_FILE OUTPUT_DIR [SATELLITE_NUMBER]' >&2
  exit 2
fi
cadu=$(realpath "$1")
out=$(realpath -m "$2")
sat="${3:-4}"
[[ -f "$cadu" ]] || { echo "CADU file not found: $cadu" >&2; exit 1; }
mkdir -p "$out"
work="$out/.satdump"
mkdir -p "$work"
appimage="$work/SatDump.AppImage"

if [[ ! -x "$appimage" ]]; then
  echo "Downloading the current SatDump nightly AppImage…"
  curl --fail --location --retry 3 --max-time 1200 --silent --show-error \
    https://github.com/SatDump/SatDump/releases/download/nightly/SatDump.AppImage \
    --output "$appimage"
  size=$(stat -c%s "$appimage")
  # Reject an HTML error page / truncated download instead of a real AppImage.
  [[ "$size" -ge 50000000 ]] || { echo "Downloaded SatDump too small (${size} bytes); aborting." >&2; exit 1; }
  head -c4 "$appimage" | grep -q $'\x7fELF' || { echo "Downloaded file is not an ELF/AppImage; aborting." >&2; exit 1; }
  sha256sum "$appimage" | tee "$work/SatDump.sha256"   # recorded, not pinned
  chmod +x "$appimage"
fi

# Extract without FUSE and keep the ORBYX library environment independent.
(cd "$work" && [[ -d squashfs-root ]] || ./SatDump.AppImage --appimage-extract > extract.log)
satdump="$work/squashfs-root/usr/bin/satdump"
share="$work/squashfs-root/usr/share/satdump"
libs="$work/squashfs-root/usr/lib/x86_64-linux-gnu/satdump/plugins:$work/squashfs-root/usr/lib/x86_64-linux-gnu:$work/squashfs-root/usr/lib"

# SatDump needs its resources/pipelines and a config in the working directory.
# Mirror the CI test's isolated setup and disable auto-processing / Kepler updates.
runtime="$work/runtime"
mkdir -p "$runtime/plugins" "$work/config"
ln -sfn "$share/resources" "$runtime/resources"
ln -sfn "$share/pipelines" "$runtime/pipelines"
ln -sfn "$work/squashfs-root/usr/lib/x86_64-linux-gnu/satdump/plugins/libelektro_arktika_support.so" "$runtime/plugins/" 2>/dev/null || true
python3 - "$share" "$runtime" <<'PY'
import re,sys
from pathlib import Path
share,runtime=Path(sys.argv[1]),Path(sys.argv[2])
text=(share/'satdump_cfg.json').read_text()
text=re.sub(r'("auto_process_products"\s*:\s*\{.*?"value"\s*:)\s*true',r'\1 false',text,flags=re.S)
text=re.sub(r'("kepler_update_interval"\s*:\s*\{.*?"value"\s*:)\s*"[^"]*"',r'\1 "Never"',text,flags=re.S)
(runtime/'satdump_cfg.json').write_text(text)
PY

echo "Decoding the CADU with SatDump (satellite ${sat})… this can take several minutes."
( cd "$runtime" && env XDG_CONFIG_HOME="$work/config" LD_LIBRARY_PATH="$libs" \
    "$satdump" pipeline elektro_rdas cadu "$cadu" "$out/decoded" \
    --satellite_number="$sat" --apply_correction=false )

echo "Done. Full-resolution channels are in: $out/decoded (MSUGS_VIS1 / MSUGS_VIS2)."
echo "For ORBYX automatic generation, copy them as sx_/dx_ MSUGS-VIS-1/2/3.png into your auto-input folder."
