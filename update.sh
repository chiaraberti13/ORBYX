#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
set -Eeuo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/scripts/lib/session.sh"
orbyx_logs update
orbyx_phase=aggiornamento-verificato
if [[ $# -eq 0 ]]; then
  prefix=$(orbyx_prefix)
  orbyx_run "$prefix/bin/orbyx-gui" --updates --lang "$orbyx_lang"
else
  orbyx_run python3 "$orbyx_repo/scripts/orbyx_update.py" "$@"
fi
