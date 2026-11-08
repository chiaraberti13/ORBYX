#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-only
# Explicit AppImage build and clean-userspace validation, x86_64 only.
set -euo pipefail
mode=${1:?Expected build or test}
repo=$(cd "$(dirname "$0")/../.." && pwd)

if [ "$mode" = build ]; then
  build="$repo/build-appimage"
  tools="$build/tools"
  appdir="$build/AppDir"
  packages="$build/packages"
  test ! -e "$appdir"
  mkdir -p "$tools/bin" "$packages"
  download_checked() {
    local name=$1 digest=$2 url=$3
    curl --fail --location --retry 3 --max-time 240 "$url" -o "$tools/$name"
    printf '%s  %s\n' "$digest" "$tools/$name" | sha256sum --check --strict
    printf '%s  %s  %s\n' "$digest" "$name" "$url" >> "$packages/BUILD-TOOLS.txt"
  }
  # Official upstream continuous assets, pinned by actual downloaded byte hashes.
  # A changed upstream asset fails closed instead of executing a different tool.
  download_checked linuxdeploy-x86_64.AppImage df908b19f1ed1cdafa79508d415aa12d6c56ce4d56d4149aa8c09286718706e5 \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
  download_checked linuxdeploy-plugin-qt-x86_64.AppImage cfc1055b2b9dbc08412b579f20990b7b41a17b61beaa5847dc9477c96c9e9617 \
    https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
  download_checked appimagetool-x86_64.AppImage e8c1169c4af66c3184d7ee110713d4be04befe671fa358e2c1591ee840553b03 \
    https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
  download_checked runtime-x86_64 617aa77314d57c8b8e4fae9c3f0b35af7b30d5529cb69e98f64f92510ed98a46 \
    https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64
  # Verification precedes chmod/execution. Extraction mode does not require FUSE.
  for name in linuxdeploy linuxdeploy-plugin-qt appimagetool; do
    mkdir -p "$tools/$name"
    chmod +x "$tools/$name-x86_64.AppImage"
    (cd "$tools/$name" && "../$name-x86_64.AppImage" --appimage-extract >/dev/null)
  done
  "$tools/linuxdeploy/squashfs-root/AppRun" --version | tee -a "$packages/BUILD-TOOLS.txt"
  "$tools/appimagetool/squashfs-root/AppRun" --version | tee -a "$packages/BUILD-TOOLS.txt"
  # Plugin discovery uses this explicit trusted wrapper, not an unverified download.
  cat > "$tools/bin/linuxdeploy-plugin-qt" <<WRAPPER
#!/usr/bin/env sh
exec "$tools/linuxdeploy-plugin-qt/squashfs-root/AppRun" "\$@"
WRAPPER
  chmod +x "$tools/bin/linuxdeploy-plugin-qt"
  cmake -S "$repo" -B "$build" -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=/usr -DORBYX_INSTALL_BATCH=OFF
  cmake --build "$build" --parallel 2
  ctest --test-dir "$build" --output-on-failure
  DESTDIR="$appdir" cmake --install "$build" --prefix /usr
  export QMAKE
  QMAKE=$(command -v qmake6)
  export EXTRA_PLATFORM_PLUGINS=libqoffscreen.so
  PATH="$tools/bin:$PATH" "$tools/linuxdeploy/squashfs-root/AppRun" \
    --appdir "$appdir" --plugin qt \
    --executable "$appdir/usr/bin/orbyx-gui" --executable "$appdir/usr/bin/orbyx" \
    --desktop-file "$appdir/usr/share/applications/orbyx.desktop" \
    --icon-file "$appdir/usr/share/icons/hicolor/scalable/apps/orbyx.svg"
  # GLVND dispatch libraries are required by QtGui even for raster/offscreen use.
  # They contain no hardware driver; actual GPU/driver support is not claimed.
  for library in libOpenGL.so.0 libGLdispatch.so.0 libGLX.so.0 libGL.so.1 libEGL.so.1; do
    source="/usr/lib/x86_64-linux-gnu/$library"
    if [ -e "$source" ]; then cp -L "$source" "$appdir/usr/lib/$library"; fi
  done
  mkdir -p "$appdir/usr/share/fonts/truetype/dejavu" "$appdir/usr/share/fontconfig"
  cp /usr/share/fonts/truetype/dejavu/DejaVuSans.ttf /usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf \
    "$appdir/usr/share/fonts/truetype/dejavu/"
  cat > "$appdir/usr/share/fontconfig/fonts.conf" <<'FONTCONF'
<?xml version="1.0"?>
<!DOCTYPE fontconfig SYSTEM "urn:fontconfig:fonts.dtd">
<fontconfig>
  <dir prefix="relative">../fonts/truetype/dejavu</dir>
  <cachedir prefix="xdg">fontconfig</cachedir>
</fontconfig>
FONTCONF
  # Preserve the build's package version inventory and distribution copyright files.
  dpkg-query -W -f='${binary:Package} ${Version}\n' | sort > "$appdir/usr/share/doc/orbyx/build-package-inventory.txt"
  mkdir -p "$appdir/usr/share/doc/orbyx/distribution-copyright"
  for copyright in /usr/share/doc/*/copyright; do
    package=$(basename "$(dirname "$copyright")")
    cp -L "$copyright" "$appdir/usr/share/doc/orbyx/distribution-copyright/$package.txt"
  done
  # Remove before writing: linuxdeploy may have made AppRun a symlink to a binary.
  rm -f "$appdir/AppRun"
  cat > "$appdir/AppRun" <<'APPRUN'
#!/usr/bin/env sh
set -eu
APPDIR=$(dirname "$(readlink -f "$0")")
export APPDIR
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
for hook in "$APPDIR"/apprun-hooks/*.sh; do
  if [ -f "$hook" ]; then . "$hook"; fi
 done
export FONTCONFIG_FILE="$APPDIR/usr/share/fontconfig/fonts.conf"
export FONTCONFIG_PATH="$APPDIR/usr/share/fontconfig"
export QT_QPA_FONTDIR="$APPDIR/usr/share/fonts/truetype/dejavu"
if [ "${1:-}" = --cli ]; then
  shift
  exec "$APPDIR/usr/bin/orbyx" "$@"
fi
exec "$APPDIR/usr/bin/orbyx-gui" "$@"
APPRUN
  chmod +x "$appdir/AppRun"
  # Explicit runtime file prevents appimagetool's automatic latest-runtime fetch.
  ARCH=x86_64 "$tools/appimagetool/squashfs-root/AppRun" --no-appstream \
    --runtime-file "$tools/runtime-x86_64" "$appdir" "$packages/ORBYX-x86_64.AppImage"
  cp "$build/orbyx_tests" "$packages/orbyx-fixture-tool"
  cpack --config "$build/CPackSourceConfig.cmake" -G TGZ -B "$packages"
  (cd "$packages" && sha256sum ORBYX-x86_64.AppImage *Source.tar.gz > SHA256SUMS)
  exit 0
fi

if [ "$mode" = test ]; then
  test "$(uname -m)" = x86_64
  cat /etc/os-release
  uname -m
  # A fresh base image has no Qt/OpenCV. Python is only the verification driver;
  # ORBYX's Python batch runner is intentionally absent from this AppImage.
  apt-get update
  DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends python3
  if dpkg-query -W -f='${db:Status-Status}\n' 'libqt6*' 'libopencv*' 2>/dev/null | grep -qx installed; then
    echo 'Qt or OpenCV was unexpectedly preinstalled in clean runtime image' >&2
    exit 1
  fi
  mkdir -p /tmp/orbyx-appimage-test
  cd /tmp/orbyx-appimage-test
  /packages/ORBYX-x86_64.AppImage --appimage-extract > extract.log
  appdir="$PWD/squashfs-root"
  test ! -e "$appdir/usr/bin/orbyx-batch"
  export LD_LIBRARY_PATH="$appdir/usr/lib"
  /packages/orbyx-fixture-tool --fixture /tmp/orbyx-fixture
  ldd "$appdir/usr/bin/orbyx-gui"
  "$appdir/usr/bin/orbyx" --version
  QT_QPA_PLATFORM=offscreen "$appdir/AppRun" --smoke-test
  "$appdir/usr/bin/orbyx" generate -s L2 -d /tmp/orbyx-fixture/input \
    --config /tmp/orbyx-fixture/profile.json --output /tmp/orbyx-cli-output --cpu
  QT_QPA_PLATFORM=offscreen "$appdir/AppRun" --lang en --cpu --satellite L2 \
    --input /tmp/orbyx-fixture/input --config /tmp/orbyx-fixture/profile.json \
    --output /tmp/orbyx-gui-output --run-and-exit
  python3 - <<'PY'
from pathlib import Path
import hashlib

def inventory(root):
    return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in root.rglob('*') if p.is_file()}
a = inventory(Path('/tmp/orbyx-cli-output'))
b = inventory(Path('/tmp/orbyx-gui-output'))
assert len(a) == 8 and a == b, (a, b)
print('Real AppImage extracted without FUSE; bundled CLI/GUI products match')
PY
  exit 0
fi

echo "Unsupported AppImage CI mode: $mode" >&2
exit 2
