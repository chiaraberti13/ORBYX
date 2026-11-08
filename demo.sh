#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
set -Eeuo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/scripts/lib/session.sh"
open_desktop=true
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-open) open_desktop=false;shift ;;
    --lang) orbyx_lang=${2:?Expected it or en};shift 2 ;;
    -h|--help) printf '%s\n' 'bash demo.sh [--no-open] [--lang it|en]';exit 0 ;;
    *) printf 'Opzione non valida / Invalid option: %s\n' "$1" >&2;exit 2 ;;
  esac
done
[[ "$orbyx_lang" == it || "$orbyx_lang" == en ]]
orbyx_logs demo
orbyx_phase=prerequisiti
prefix=$(orbyx_prefix)
if [[ ! -f "$orbyx_repo/build-desktop/.orbyx-install-success" || ! -x "$prefix/bin/orbyx" ]]; then
  orbyx_message 'Esegui prima: bash install.sh' 'Run first: bash install.sh' >&2;exit 2
fi
exec 9> "$orbyx_repo/.orbyx-install.lock"
if ! flock -n -s 9; then
  orbyx_message 'Installazione in corso: attendi.' 'Installation is running: wait.' >&2;exit 2
fi
orbyx_message 'Demo con immagini SINTETICHE: non sono acquisizioni satellitari reali.' \
  'Demo with SYNTHETIC images: these are not actual satellite acquisitions.'
[[ ! -L "$orbyx_repo/demo-output" ]]
mkdir -p "$orbyx_repo/demo-output"
work="$orbyx_repo/demo-output/$(date -u +%Y%m%dT%H%M%SZ)-$$"
orbyx_phase=demo-calibrazione-allineamento
orbyx_run python3 "$orbyx_repo/scripts/orbyx_simulate.py" --orbyx "$prefix/bin/orbyx" --satellite L4 --out "$work"
orbyx_phase=demo-natural-color
channels="$work/aligned/MSUGS_VIS-1_Aligned"
orbyx_run "$prefix/bin/orbyx" natural-color --ch1 "$channels/MSUGS-VIS-1.png" \
  --ch2 "$channels/MSUGS-VIS-2.png" --ch3 "$channels/MSUGS-VIS-3.png" \
  --output "$work/natural-color.png" --lang "$orbyx_lang"
orbyx_message 'Demo completata. Risultati nella cartella demo-output; diagnostica nella cartella logs.' \
  'Demo completed. Results are in demo-output; diagnostics are in logs.'
if "$open_desktop" && [[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]]; then
  orbyx_phase=demo-desktop
  orbyx_run "$prefix/bin/orbyx-gui" --demo --lang "$orbyx_lang" --input "$work/acquisition" \
    --config "$work/calibration/calibration.json" --satellite L4 --output "$work/new-output"
else
  orbyx_message 'Nessuna finestra aperta. Su un desktop puoi usare: bash demo.sh' \
    'No window opened. From a desktop session you can use: bash demo.sh'
fi
