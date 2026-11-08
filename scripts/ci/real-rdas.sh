#!/usr/bin/env bash
# Explicit networked integration test, opt-in locally; no raw imagery in git.
set -euo pipefail
if [[ $# != 2 ]]; then
  echo 'Usage: real-rdas.sh ABSOLUTE_ORBYX_BINARY NEW_WORK_DIRECTORY' >&2
  exit 2
fi
repo=$(cd "$(dirname "$0")/../.." && pwd)
binary=$(realpath "$1")
work=$(realpath -m "$2")
mkdir "$work"
python3 "$repo/scripts/fetch_real_rdas.py" "$work/elektro_rdas.cadu" > "$work/download.json"
curl --fail --location --retry 3 --max-time 900 --silent --show-error \
  https://github.com/SatDump/SatDump/releases/download/nightly/SatDump.AppImage \
  --output "$work/SatDump.AppImage"
printf '%s  %s\n' db14d454d5ffd1aa216cafd6e0768225fa1b6cc596c5011b0135471741447ce2 "$work/SatDump.AppImage" | sha256sum --check
# A mutable nightly URL is safe only while its pinned bytes match. Never update
# the digest automatically or execute a newly replaced upstream asset.
chmod +x "$work/SatDump.AppImage"
(cd "$work" && ./SatDump.AppImage --appimage-extract > extract.log)
mkdir -p "$work/runtime/plugins" "$work/config"
ln -s "$work/squashfs-root/usr/share/satdump/resources" "$work/runtime/resources"
ln -s "$work/squashfs-root/usr/share/satdump/pipelines" "$work/runtime/pipelines"
ln -s "$work/squashfs-root/usr/lib/x86_64-linux-gnu/satdump/plugins/libelektro_arktika_support.so" "$work/runtime/plugins/"
python3 - "$work" <<'PY'
import re,sys
from pathlib import Path
work=Path(sys.argv[1])
text=(work/'squashfs-root/usr/share/satdump/satdump_cfg.json').read_text()
text=re.sub(r'("auto_process_products"\s*:\s*\{.*?"value"\s*:)\s*true',r'\1 false',text,flags=re.S)
text=re.sub(r'("kepler_update_interval"\s*:\s*\{.*?"value"\s*:)\s*"[^"]*"',r'\1 "Never"',text,flags=re.S)
(work/'runtime/satdump_cfg.json').write_text(text)
(work/'empty.json').write_text('{}\n')
PY
satdump="$work/squashfs-root/usr/bin/satdump"
# Keep the ORBYX native library environment independent of the AppImage bundle.
satdump_libraries="$work/squashfs-root/usr/lib/x86_64-linux-gnu/satdump/plugins:$work/squashfs-root/usr/lib/x86_64-linux-gnu:$work/squashfs-root/usr/lib"
(
  cd "$work/runtime"
  env XDG_CONFIG_HOME="$work/config" LD_LIBRARY_PATH="$satdump_libraries" \
    timeout 600 "$satdump" pipeline elektro_rdas cadu "$work/elektro_rdas.cadu" "$work/decoded" \
    --satellite_number=4 --apply_correction=false > "$work/decode.log" 2>&1
)
(cd "$work/decoded" && find MSUGS_VIS1 MSUGS_VIS2 -type f ! -name '.*' -print0 | sort -z | xargs -0 sha256sum) > "$work/decoded.sha256"
"$binary" calibrate -s L4 -d "$work/decoded" --config "$work/empty.json" \
  --output "$work/calibrated" --cpu --calibration-scale 0.125 > "$work/calibrate.log" 2>&1
"$binary" generate -s L4 -d "$work/decoded" --config "$work/calibrated/calibration.json" \
  --output "$work/aligned" --cpu > "$work/generate.log" 2>&1
for vis in 1 2; do
  channels="$work/aligned/MSUGS_VIS-${vis}_Aligned"
  "$binary" natural-color --ch1 "$channels/MSUGS-VIS-1.png" \
    --ch2 "$channels/MSUGS-VIS-2.png" --ch3 "$channels/MSUGS-VIS-3.png" \
    --output "$work/natural-vis${vis}.png" > "$work/natural-vis${vis}.log" 2>&1
  python3 "$repo/scripts/check_natural_color.py" --channels "$channels" \
    --image "$work/natural-vis${vis}.png" --report "$work/natural-vis${vis}.json"
done
(cd "$work/decoded" && sha256sum --check "$work/decoded.sha256")
python3 "$repo/scripts/check_real_rdas.py" --decoded "$work/decoded" --aligned "$work/aligned" \
  --profile "$work/calibrated/calibration.json" --report "$work/validation.json"
for vis in 1 2; do
  (
    cd "$work/runtime"
    env XDG_CONFIG_HOME="$work/config" LD_LIBRARY_PATH="$satdump_libraries" \
      timeout 240 "$satdump" process "$work/aligned/MSUGS_VIS-${vis}_Aligned/product.cbor" \
      "$work/reimport-vis${vis}" > "$work/reimport-vis${vis}.log" 2>&1
  )
  # A zero exit alone is insufficient: confirm all actual PNG channels were read.
  for channel in 1 2 3; do
    grep -Fq "Loading MSUGS-VIS-${channel}.png" "$work/reimport-vis${vis}.log"
  done
  grep -Fq 'Exiting SatDump! Bye!' "$work/reimport-vis${vis}.log"
done
mkdir "$work/evidence"
cp "$work/"*.log "$work/decoded.sha256" "$work/download.json" "$work/validation.json" "$work/calibrated/calibration.json" "$work/evidence/"
cp "$work/"natural-vis*.json "$work/evidence/"
printf '%s\n' 'Real RDAS decode, native-resolution output and SatDump product load completed.' \
  'Absolute geolocation and ground-truth calibration are NOT certified by this sample.'
