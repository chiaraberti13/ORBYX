#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Disposable Fedora VM with its own kernel; explicit networked CI integration.
set -euo pipefail
repo=$(cd "$(dirname "$0")/../.." && pwd)
work=${1:?Expected a new absolute work directory}
[[ "$work" = /* ]]
mkdir "$work"
cleanup() {
  if [[ -f "$work/vm.pid" ]]; then
    kill "$(cat "$work/vm.pid")" 2>/dev/null || true
  fi
}
trap cleanup EXIT
mkdir "$work/packages"
fedora_image=registry.fedoraproject.org/fedora@sha256:5aae8bdc5f40bf70f8f189a70770e6f1fabcb04976e841262431c3bbfdbe3538
docker pull "$fedora_image"
docker run --rm --security-opt=no-new-privileges \
  -v "$repo:/src:ro" -v "$work/packages:/out" "$fedora_image" \
  bash /src/scripts/ci/platforms.sh build-fedora
curl --fail --location --retry 3 --max-time 900 --silent --show-error \
  https://download.fedoraproject.org/pub/fedora/linux/releases/44/Cloud/x86_64/images/Fedora-Cloud-Base-Generic-44-1.7.x86_64.qcow2 \
  --output "$work/fedora-base.qcow2"
printf '%s  %s\n' 28680fe5b371a5a82ebf43a31926e086a168e59949d03969c5093e7071f90b7f "$work/fedora-base.qcow2" | sha256sum --check
qemu-img create -f qcow2 -F qcow2 -b "$work/fedora-base.qcow2" "$work/guest.qcow2"
qemu-img resize "$work/guest.qcow2" 20G
ssh-keygen -q -t ed25519 -N '' -f "$work/ssh_key"
python3 - "$work" <<'PY'
from pathlib import Path
import sys
work=Path(sys.argv[1]);public=(work/'ssh_key.pub').read_text().strip()
(work/'meta-data').write_text('instance-id: orbyx-clean-fedora44\n')
(work/'user-data').write_text('''#cloud-config
# Keep the base image hostname; runtime QA connects by forwarded IP.
preserve_hostname: true
ssh_pwauth: false
disable_root: true
users:
  - name: orbyx
    groups: [wheel]
    sudo: ["ALL=(ALL) NOPASSWD:ALL"]
    lock_passwd: true
    ssh_authorized_keys:
      - '''+public+'\n')
PY
cloud-localds "$work/seed.iso" "$work/user-data" "$work/meta-data"
acceleration=(-accel tcg,thread=multi -cpu max)
if [[ -e /dev/kvm ]]; then
  sudo chmod a+rw /dev/kvm
  acceleration=(-accel kvm -cpu host)
fi
printf '%s\n' "${acceleration[*]}" | tee "$work/acceleration.log"
qemu-system-x86_64 "${acceleration[@]}" -m 4096 -smp 2 \
  -drive "file=$work/guest.qcow2,if=virtio,format=qcow2" \
  -drive "file=$work/seed.iso,if=virtio,format=raw,readonly=on" \
  -nic user,model=virtio-net-pci,hostfwd=tcp:127.0.0.1:2222-:22 \
  -display none -serial "file:$work/vm-serial.log" -monitor none -no-reboot \
  -daemonize -pidfile "$work/vm.pid"
ssh_options=(-i "$work/ssh_key" -o BatchMode=yes -o StrictHostKeyChecking=accept-new \
  -o "UserKnownHostsFile=$work/known_hosts" -o ConnectTimeout=5)
ready=false
for attempt in $(seq 1 120); do
  if ssh "${ssh_options[@]}" -p 2222 orbyx@127.0.0.1 true 2>/dev/null; then ready=true; break; fi
  if ! kill -0 "$(cat "$work/vm.pid")" 2>/dev/null; then
    tail -100 "$work/vm-serial.log" >&2; exit 1
  fi
  sleep 5
done
if [[ "$ready" != true ]]; then tail -100 "$work/vm-serial.log" >&2; exit 1; fi
ssh "${ssh_options[@]}" -p 2222 orbyx@127.0.0.1 'sudo cloud-init status --wait --long' | tee "$work/cloud-init.log"
ssh "${ssh_options[@]}" -p 2222 orbyx@127.0.0.1 'sudo mkdir -p /packages /checks; sudo chown orbyx:orbyx /packages /checks'
scp "${ssh_options[@]}" -P 2222 "$work/packages/"*.rpm "$work/packages/orbyx-fixture-tool" orbyx@127.0.0.1:/packages/
scp "${ssh_options[@]}" -P 2222 "$repo/scripts/ci/platforms.sh" "$repo/scripts/ci/check_image_demo.py" orbyx@127.0.0.1:/checks/
ssh "${ssh_options[@]}" -p 2222 orbyx@127.0.0.1 'bash -se' <<'GUEST' | tee "$work/guest-test.log"
set -euo pipefail
source /etc/os-release
test "$ID" = fedora && test "$VERSION_ID" = 44
uname -a
systemd-detect-virt --vm
if systemd-detect-virt --container; then echo 'Container does not satisfy full VM test' >&2;exit 1;fi
case "$(uname -r)" in *fc44*) ;; *) echo 'Expected a genuine Fedora44 kernel' >&2;exit 1;; esac
sudo env EXPECTED_ARCH=x86_64 bash /checks/platforms.sh test-fedora
echo 'Fresh Fedora44 VM: native kernel, RPM install, CLI/GUI/colour equivalence and uninstall passed'
GUEST
scp "${ssh_options[@]}" -P 2222 orbyx@127.0.0.1:/tmp/runtime-packages.txt "$work/runtime-packages.txt"
