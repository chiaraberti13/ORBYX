#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
orbyx_repo=$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)
orbyx_lang=${ORBYX_LANG:-it}
orbyx_phase=inizializzazione
orbyx_message() {
  if [[ "$orbyx_lang" == en ]]; then printf '%s\n' "$2"; else printf '%s\n' "$1"; fi
}
orbyx_logs() {
  umask 077
  local directory="$orbyx_repo/logs"
  if [[ -L "$directory" ]]; then
    orbyx_message 'La cartella logs non può essere un collegamento simbolico.' 'The logs directory must not be a symlink.' >&2; return 2
  fi
  mkdir -p "$directory"
  local stamp;stamp=$(date -u +%Y%m%dT%H%M%SZ)-$$
  orbyx_log="$directory/$1-$stamp.log"
  orbyx_errors="$directory/$1-$stamp.errors.log"
  # Exclusive files: never truncate an existing log or follow a pre-created link.
  (set -o noclobber; : > "$orbyx_log"; : > "$orbyx_errors")
  exec > >(tee -a "$orbyx_log") 2> >(tee -a "$orbyx_log" "$orbyx_errors" >&2)
  trap 'orbyx_failure "$?"' ERR
  trap 'orbyx_failure 130' INT
  trap 'orbyx_failure 143' TERM
  printf 'ORBYX | %s | %s\n' "$1" "$(date -u +%FT%TZ)"
}
orbyx_failure() {
  local status=$1
  trap - ERR INT TERM
  orbyx_message "Operazione interrotta: $orbyx_phase (codice $status). Dettagli nella cartella logs." \
    "Operation stopped: $orbyx_phase (exit $status). Details are in the logs directory." >&2
  # Tools can emit errors to stdout as well as stderr. Retain the failed context.
  tail -n 80 "$orbyx_log" >> "$orbyx_errors" 2>/dev/null || true
  exit "$status"
}
orbyx_run() {
  local temporary status
  local -a statuses
  temporary=$(mktemp "$orbyx_repo/logs/.step-XXXXXX")
  if "$@" 2>&1 | tee "$temporary"; then
    rm -f "$temporary"
  else
    statuses=("${PIPESTATUS[@]}")
    status=${statuses[0]}
    ((status!=0)) || status=${statuses[1]}
    # Synchronous copy also preserves tools that report failures only on stdout.
    cat "$temporary" >> "$orbyx_errors"
    rm -f "$temporary"
    return "$status"
  fi
}
orbyx_prefix() {
  printf '%s/orbyx/application' "${XDG_DATA_HOME:-$HOME/.local/share}"
}
orbyx_launch() {
  local mode=$1;shift
  orbyx_logs "$mode"
  orbyx_phase=avvio
  local binary;binary="$(orbyx_prefix)/bin/orbyx"
  [[ "$mode" != desktop ]] || binary+="-gui"
  if [[ ! -f "$orbyx_repo/build-desktop/.orbyx-install-success" || ! -x "$binary" ]]; then
    orbyx_message 'Installazione incompleta. Esegui prima: bash install.sh' 'Installation incomplete. Run first: bash install.sh' >&2
    return 2
  fi
  exec 9> "$orbyx_repo/.orbyx-install.lock"
  if ! flock -n -s 9; then
    orbyx_message 'Installazione in corso: attendi il completamento.' 'Installation is running: wait for completion.' >&2
    return 2
  fi
  orbyx_run "$binary" "$@"
}
