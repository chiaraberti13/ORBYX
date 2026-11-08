#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Run explicitly in disposable, native-architecture CI containers.
set -euo pipefail
mode=${1:?Expected build-fedora, test-fedora or test-ubuntu}

check_runtime() {
  local fixture_tool=${1:?Fixture executable required}
  test -x "$fixture_tool"
  test "$(uname -m)" = "${EXPECTED_ARCH:?Expected architecture required}"
  cat /etc/os-release
  uname -m
  orbyx --version
  orbyx-batch --lang it --help
  QT_QPA_PLATFORM=offscreen orbyx-gui --smoke-test
  "$fixture_tool" --fixture /tmp/orbyx-fixture
  orbyx generate -s L2 -d /tmp/orbyx-fixture/input \
    --config /tmp/orbyx-fixture/profile.json --output /tmp/orbyx-cli-output --cpu
  QT_QPA_PLATFORM=offscreen orbyx-gui --lang en --cpu --satellite L2 \
    --input /tmp/orbyx-fixture/input --config /tmp/orbyx-fixture/profile.json \
    --output /tmp/orbyx-gui-output --run-and-exit
  python3 - <<'PY'
from pathlib import Path
import hashlib

def snapshot(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}
a = snapshot(Path('/tmp/orbyx-cli-output'))
b = snapshot(Path('/tmp/orbyx-gui-output'))
assert len(a) == 8 and a == b, (a, b)
print('Installed CLI and GUI produced the same eight product files')
PY
}

case "$mode" in
  build-fedora)
    test "$(uname -m)" = x86_64
    dnf install -y gcc-c++ cmake ninja-build opencv-devel jsoncpp-devel \
      qt6-qtbase-devel mesa-libGL-devel rpm-build python3 gnupg2 binutils
    cat /etc/os-release | tee /out/build-environment.txt
    uname -m | tee -a /out/build-environment.txt
    rpm -qa --qf '%{NAME} %{VERSION}-%{RELEASE}.%{ARCH}\n' | sort > /out/build-packages.txt
    cmake -S /src -B /tmp/orbyx-build -G Ninja -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr
    cmake --build /tmp/orbyx-build --parallel 2
    ctest --test-dir /tmp/orbyx-build --output-on-failure
    cpack --config /tmp/orbyx-build/CPackConfig.cmake -G RPM -B /tmp/orbyx-build/packages
    rpm -qpi /tmp/orbyx-build/packages/*.rpm
    rpm -qp --requires /tmp/orbyx-build/packages/*.rpm | sort > /out/rpm-requirements.txt
    cp /tmp/orbyx-build/packages/*.rpm /out/
    cp /tmp/orbyx-build/orbyx_tests /out/orbyx-fixture-tool
    ;;
  test-fedora)
    # This is a fresh image, not the build container. DNF resolves the RPM's own
    # declared runtime requirements; no compiler or development headers are added.
    dnf install -y /packages/*.rpm
    for package in gcc-c++ cmake opencv-devel jsoncpp-devel qt6-qtbase-devel; do
      if rpm -q "$package" >/dev/null 2>&1; then
        echo "Unexpected build dependency: $package" >&2
        exit 1
      fi
    done
    if rpm -qa --qf '%{NAME}\n' | grep -E -- '-devel$'; then
      echo 'Unexpected development package in clean runtime image' >&2
      exit 1
    fi
    check_runtime /packages/orbyx-fixture-tool
    rpm -qa --qf '%{NAME} %{VERSION}-%{RELEASE}.%{ARCH}\n' | sort > /tmp/runtime-packages.txt
    cat /tmp/runtime-packages.txt
    dnf remove -y orbyx
    hash -r
    if command -v orbyx >/dev/null 2>&1; then exit 1; fi
    test -f /tmp/orbyx-fixture/input/MSUGS_VIS1/product.cbor
    ;;
  test-ubuntu)
    export DEBIAN_FRONTEND=noninteractive
    apt-get update
    apt-get install -y /packages/*.deb
    for package in g++ cmake libopencv-dev libjsoncpp-dev qt6-base-dev; do
      if dpkg-query -W -f='${db:Status-Status}' "$package" 2>/dev/null | grep -qx installed; then
        echo "Unexpected build dependency: $package" >&2
        exit 1
      fi
    done
    check_runtime /packages/orbyx-fixture-tool
    dpkg-query -W -f='${binary:Package} ${Version}\n' | sort
    apt-get remove -y orbyx
    hash -r
    if command -v orbyx >/dev/null 2>&1; then exit 1; fi
    test -f /tmp/orbyx-fixture/input/MSUGS_VIS1/product.cbor
    ;;
  *) echo "Unsupported CI mode: $mode" >&2; exit 2 ;;
esac
