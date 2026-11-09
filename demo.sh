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
orbyx_message 'Demo nell’app: sei JPEG reali diventano un’unica immagine.' \
  'In-app demo: six real JPEGs become one image.'
[[ ! -L "$orbyx_repo/demo-output" ]]
mkdir -p "$orbyx_repo/demo-output"
work="$orbyx_repo/demo-output/$(date -u +%Y%m%dT%H%M%SZ)-$$"
orbyx_phase=demo-desktop
if "$open_desktop" && [[ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]]; then
  orbyx_run "$prefix/bin/orbyx-gui" --demo --lang "$orbyx_lang" --output "$work"
else
  orbyx_run env QT_QPA_PLATFORM=offscreen "$prefix/bin/orbyx-gui" --demo --run-and-exit --lang "$orbyx_lang" --output "$work"
fi
orbyx_message 'Demo completata. Immagine e report nella cartella demo-output; diagnostica nella cartella logs.' \
  'Demo completed. Image and report are in demo-output; diagnostics are in logs.'
