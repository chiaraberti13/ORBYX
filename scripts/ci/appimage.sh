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
  # linuxdeploy's blacklist assumes a desktop host and omits libraries such as
  # FreeType/fontconfig. A pristine container does not satisfy that assumption.
  # Close the ELF dependency graph for both executables, every deployed plugin,
  # every deployed library, and the external fixture driver used by runtime QA.
  # Only glibc/its loader remain a documented requirement of the Ubuntu baseline.
  python3 - "$appdir" "$build/orbyx_tests" "$packages/BUNDLED-LIBRARIES.json" <<'PY_CLOSURE'
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys

appdir = Path(sys.argv[1]).resolve()
fixture = Path(sys.argv[2]).resolve()
report = Path(sys.argv[3])
library_dir = appdir / "usr/lib"
library_dir.mkdir(parents=True, exist_ok=True)
glibc = {
    "libc.so.6", "libm.so.6", "libdl.so.2", "libpthread.so.0", "librt.so.1",
    "libresolv.so.2", "libanl.so.1", "libutil.so.1", "libBrokenLocale.so.1",
    "libnss_compat.so.2", "libnss_dns.so.2", "libnss_files.so.2",
    "libnss_hesiod.so.2", "libthread_db.so.1", "ld-linux-x86-64.so.2",
}
environment = dict(os.environ, LD_LIBRARY_PATH=str(library_dir), LC_ALL="C")
copied = {}
required_system = set()


def elf(path):
    try:
        with path.open("rb") as stream:
            return stream.read(4) == b"\x7fELF"
    except (OSError, ValueError):
        return False


def digest(path):
    checksum = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            checksum.update(block)
    return checksum.hexdigest()


for iteration in range(16):
    changed = False
    roots = sorted({p.resolve() for p in appdir.rglob("*") if p.is_file() and elf(p)})
    roots.append(fixture)
    if len(roots) > 10000:
        raise RuntimeError("Unexpectedly large ELF dependency graph")
    for binary in roots:
        result = subprocess.run(["ldd", str(binary)], env=environment,
                                text=True, capture_output=True, timeout=30)
        output = result.stdout + result.stderr
        if "not found" in output:
            raise RuntimeError(f"Unresolved dependency in {binary}:\n{output}")
        if result.returncode and not any(phrase in output for phrase in
                                        ("not a dynamic executable", "statically linked")):
            raise RuntimeError(f"ldd failed for {binary}:\n{output}")
        for line in output.splitlines():
            match = re.match(r"^\s*(\S+) => (/\S+)\s+\(", line)
            if match:
                soname, source_text = match.groups()
            else:
                match = re.match(r"^\s*(/\S+)\s+\(", line)
                if not match:
                    continue  # virtual linux-vdso and static-binary diagnostics
                source_text = match.group(1)
                soname = Path(source_text).name
            if soname in glibc:
                required_system.add(soname)
                continue
            source = Path(source_text).resolve(strict=True)
            if not source.is_file():
                raise RuntimeError(f"Resolved dependency is not a regular file: {source}")
            target = library_dir / soname
            if target.is_symlink() and not target.resolve().is_relative_to(appdir):
                target.unlink()  # do not retain absolute links into the build host
            if target.exists():
                if target.resolve() != source and digest(target) != digest(source):
                    raise RuntimeError(f"Conflicting library with the same SONAME: {soname}")
                continue
            if target.is_symlink():
                target.unlink()  # dangling link is not a bundled dependency
            shutil.copy2(source, target)
            copied[soname] = {"source": str(source), "sha256": digest(target)}
            changed = True
    if not changed:
        break
else:
    raise RuntimeError("ELF dependency closure did not converge")

inventory = {p.name: digest(p) for p in sorted(library_dir.iterdir())
             if p.is_file() and elf(p)}
report.write_text(json.dumps({"copied_after_linuxdeploy": copied,
                              "bundled_elf_sha256": inventory,
                              "required_system_glibc": sorted(required_system)},
                             indent=2, sort_keys=True) + "\n")
print(f"ELF closure verified: {len(inventory)} bundled libraries; "
      f"{len(copied)} dependencies copied after linuxdeploy")
PY_CLOSURE
  cp "$packages/BUNDLED-LIBRARIES.json" "$appdir/usr/share/doc/orbyx/BUNDLED-LIBRARIES.json"
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
