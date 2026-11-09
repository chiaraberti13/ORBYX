#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Explicit networked setup. Build and application run as the invoking user.
set -Eeuo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/scripts/lib/session.sh"
install_dependencies=true
while [[ $# -gt 0 ]]; do
  case "$1" in
    --no-deps) install_dependencies=false;shift ;;
    --lang) orbyx_lang=${2:?Expected it or en};shift 2 ;;
    -h|--help) printf '%s\n' 'bash install.sh [--lang it|en] [--no-deps]';exit 0 ;;
    *) printf 'Opzione non valida / Invalid option: %s\n' "$1" >&2;exit 2 ;;
  esac
done
[[ "$orbyx_lang" == it || "$orbyx_lang" == en ]]
orbyx_logs install
orbyx_phase=prerequisiti
if [[ $EUID -eq 0 ]]; then
  orbyx_message 'Esegui questo script con il tuo utente, senza sudo. I privilegi saranno richiesti solo per APT.' \
    'Run this script as your user, without sudo. Privileges are requested only for APT.' >&2
  exit 2
fi
command -v flock >/dev/null
exec 9> "$orbyx_repo/.orbyx-install.lock"
if ! flock -n 9; then
  orbyx_message 'Chiudi ORBYX o attendi che termini l’altra installazione.' 'Close ORBYX or wait for the other installation to finish.' >&2
  exit 2
fi
prefix=$(orbyx_prefix)
managed_root=$(dirname "$prefix")
if [[ -L "$managed_root" || -L "$managed_root/updates" || -L "$managed_root/updates/lock" ]]; then
  orbyx_message 'La cartella di installazione o il lock non possono essere collegamenti simbolici.' 'Installation directories and lock must not be symlinks.' >&2
  exit 2
fi
mkdir -p "$managed_root/updates"
exec 8>> "$managed_root/updates/lock"
if ! flock -n 8; then
  orbyx_message 'Un’altra installazione o un aggiornamento verificato è in corso.' 'Another installation or verified update is in progress.' >&2
  exit 2
fi
if [[ -L "$prefix" ]]; then
  orbyx_message 'Installazione gestita da aggiornamenti firmati: usa Aggiornamenti nell’app oppure bash update.sh. La release verificata non viene modificata da install.sh.' \
    'Signed managed installation: use Updates in the app or bash update.sh. install.sh does not modify a verified release.' >&2
  exit 2
fi
orbyx_gpu_diagnostic() {
  # Runs the opencl-gpu verification on its own and reports the outcome without
  # ever aborting the installation. Verified → GPU acceleration is available;
  # skipped → no suitable GPU, ORBYX uses the CPU; otherwise → the GPU diverges
  # from the CPU and ORBYX falls back to the CPU automatically (results intact).
  local build=$1 status=0 report
  report=$(mktemp "$orbyx_repo/logs/.gpu-XXXXXX")
  ctest --test-dir "$build" -V --tests-regex '^opencl-gpu$' > "$report" 2>&1 || status=$?
  cat "$report"
  if grep -q '"status" : "gpu_kernel_verified"' "$report"; then
    orbyx_message 'Diagnostica GPU: OpenCL verificata, accelerazione disponibile.' \
      'GPU diagnostic: OpenCL verified, acceleration available.'
  elif grep -q '"status" : "skipped"' "$report"; then
    orbyx_message 'Diagnostica GPU: nessuna GPU OpenCL idonea; ORBYX elabora su CPU.' \
      'GPU diagnostic: no suitable OpenCL GPU; ORBYX runs on the CPU.'
  else
    orbyx_message 'Diagnostica GPU: la GPU OpenCL diverge dalla CPU; ORBYX usa automaticamente la CPU, i risultati non cambiano.' \
      'GPU diagnostic: the OpenCL GPU diverges from the CPU; ORBYX falls back to the CPU automatically, results are unaffected.'
  fi
  rm -f "$report"
  return 0
}
apt_wait=${ORBYX_APT_LOCK_TIMEOUT:-900}
[[ "$apt_wait" =~ ^[0-9]+$ && ${#apt_wait} -le 4 ]] && ((apt_wait>=1 && apt_wait<=3600))
apt_run() {
  local started=$SECONDS status output
  output=$(mktemp "$orbyx_repo/logs/.apt-XXXXXX")
  while true; do
    if sudo env LC_ALL=C apt-get -o DPkg::Lock::Timeout=30 -o Acquire::Retries=3 "$@" > "$output" 2>&1; then
      cat "$output";rm -f "$output";return 0
    else
      status=$?
    fi
    cat "$output"
    if grep -Eq 'Could not get lock|Unable to acquire.*lock|Unable to lock' "$output" && ((SECONDS-started<apt_wait)); then
      orbyx_message 'Gestore pacchetti occupato. Attendo gli aggiornamenti automatici senza interromperli…' \
        'Package manager busy. Waiting for automatic updates without interrupting them…'
      sleep 5
    else
      cat "$output" >&2;rm -f "$output";return "$status"
    fi
  done
}
if "$install_dependencies"; then
  command -v apt-get >/dev/null
  command -v sudo >/dev/null
  orbyx_phase=dipendenze
  orbyx_message 'Installazione delle dipendenze. APT può attendere gli aggiornamenti automatici.' \
    'Installing dependencies. APT may wait for automatic updates.'
  sudo -v
  apt_run update
  apt_run install -y build-essential cmake ninja-build libopencv-dev libjsoncpp-dev \
    qt6-base-dev qt6-qpa-plugins libgl-dev gnupg binutils python3 gh fonts-dejavu-core
fi
for tool in cmake ctest ninja c++ python3; do
  if ! command -v "$tool" >/dev/null; then
    orbyx_message "Dipendenza mancante: $tool. Ripeti l'installazione senza --no-deps." \
      "Missing dependency: $tool. Retry without --no-deps." >&2
    exit 2
  fi
done
build="$orbyx_repo/build-desktop"
orbyx_phase=configurazione
orbyx_run cmake -S "$orbyx_repo" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DORBYX_BUILD_GUI=ON -DBUILD_TESTING=ON -DCMAKE_INSTALL_PREFIX="$prefix"
orbyx_phase=compilazione
orbyx_run cmake --build "$build" --parallel 2
orbyx_phase=test
# Functional gate: every ORBYX test except the optional GPU hardware probe.
orbyx_run ctest --test-dir "$build" --output-on-failure --label-exclude gpu
# GPU OpenCL equivalence is a hardware diagnostic, never an install blocker. The
# engine verifies the GPU at runtime and falls back to the CPU when the device's
# affine diverges, so an old, divergent or absent GPU never stops a working CPU
# installation. The test still runs and its result is reported to the user.
orbyx_phase=diagnostica-gpu
orbyx_gpu_diagnostic "$build"
orbyx_phase=installazione
rm -f "$build/.orbyx-install-success"
orbyx_run cmake --install "$build"
orbyx_phase=verifica
orbyx_run "$prefix/bin/orbyx" --version
QT_QPA_PLATFORM=offscreen orbyx_run "$prefix/bin/orbyx-gui" --smoke-test
printf '%s\n' "$(date -u +%FT%TZ)" > "$build/.orbyx-install-success"
orbyx_message 'Installazione completata. Avvio desktop: bash start.sh — CLI: bash orbyx.sh --help' \
  'Installation completed. Desktop: bash start.sh — CLI: bash orbyx.sh --help'
orbyx_message 'Log di installazione ed errori disponibili nella cartella logs.' 'Installation logs and errors are available in the logs directory.'
