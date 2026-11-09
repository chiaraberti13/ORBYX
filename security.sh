#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
set -Eeuo pipefail
source "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/scripts/lib/session.sh"
orbyx_logs security
prefix=$(orbyx_prefix)
work="$orbyx_repo/logs/security-$(date -u +%Y%m%dT%H%M%SZ)-$$"
mkdir "$work"
orbyx_phase=inventario-runtime
orbyx_run python3 "$orbyx_repo/scripts/orbyx_sbom.py" "$prefix/bin/orbyx" "$prefix/bin/orbyx-gui" --output "$work/inventory.json"
orbyx_phase=avvisi-sicurezza
orbyx_message 'Consulto OSV con nomi/versioni dei pacchetti sorgente Ubuntu. File e percorsi personali restano locali.' 'Querying OSV with Ubuntu source-package names/versions. Files and personal paths remain local.'
if orbyx_run python3 "$orbyx_repo/scripts/orbyx_advisories.py" --inventory "$work/inventory.json" --online --output "$work/advisories.json"; then
  status=0
else
  status=$?
fi
orbyx_message 'Report e snapshot riproducibile salvati nella cartella logs. Un esito senza segnalazioni non garantisce assenza di vulnerabilità.' 'Report and replayable snapshot saved under logs. No matches is not a guarantee of no vulnerabilities.'
exit "$status"
