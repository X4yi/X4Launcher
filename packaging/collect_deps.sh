#!/usr/bin/env bash
set -euo pipefail

STAGING_DIR=${1:-build/package}
OUT_DIR=${2:-dist}

if [ ! -d "${STAGING_DIR}/usr" ]; then
  echo "Staging tree not found in ${STAGING_DIR}. Run build_release.sh first." >&2
  exit 1
fi

mkdir -p "${OUT_DIR}/bin" "${OUT_DIR}/lib" "${OUT_DIR}/plugins" "${OUT_DIR}/data" "${OUT_DIR}/share"

if [ -d "${STAGING_DIR}/usr/bin" ]; then
  cp -a "${STAGING_DIR}/usr/bin/"* "${OUT_DIR}/bin/" || true
fi

if [ -d "${STAGING_DIR}/usr/share" ]; then
  cp -a "${STAGING_DIR}/usr/share/"* "${OUT_DIR}/share/" || true
fi

EXCLUDE_BASENAMES=(
  "ld-linux" "libc.so" "libm.so" "libpthread.so" "librt.so" "libdl.so"
  "libgcc_s.so" "libstdc++.so"
  "libX11.so" "libxcb.so" "libwayland" "libEGL" "libGL" "libGLX" "libGLdispatch"
  "libdrm" "libfontconfig" "libfreetype" "libexpat"
  "libnss" "libnspr" "libsoftokn" "libfreebl"
  "libdbus" "libsystemd" "libresolv"
  "libavcodec" "libavformat" "libavutil" "libswresample" "libswscale"
  "libx264" "libx265" "libaom" "libvpx" "libopus" "libSvtAv1"
  "librav1e" "libdav1d" "libopenh264" "libopenmpt"
  "libvorbis" "libFLAC" "libsndfile" "libgsm" "libspeex"
  "libbluray" "libdvdread" "libdvdnav" "libsoxr"
  "libopencore" "libmp3lame" "libtheora" "libvidstab"
  "libva.so" "libva-drm" "libva-x11" "libvdpau"
  "libpango" "libcairo" "libpixman" "libgio-" "libglib-" "libgobject-"
  "libgmodule" "libgthread" "libglycin" "librsvg"
  "libidn" "libunistring" "libtasn1" "libnettle" "libhogweed"
  "libgnutls" "libp11-kit" "libgcrypt" "libgpg-error"
  "libsnappy" "libre2.so" "libabsl_" "libprotobuf"
  "libjxl" "libhwy"
  "libtss2"
)

declare -A COPIED

copy_and_recurse() {
  local path="$1"
  [ -z "$path" ] && return
  local base=$(basename "$path")
  for ex in "${EXCLUDE_BASENAMES[@]}"; do
    if [[ "$base" == $ex* ]]; then
      return
    fi
  done
  if [ -n "${COPIED[$path]:-}" ]; then
    return
  fi
  COPIED[$path]=1

  mkdir -p "${OUT_DIR}/lib"
  cp -a "$path" "${OUT_DIR}/lib/" || true

  if [ -L "$path" ]; then
    local realpath
    realpath=$(readlink -f "$path")
    if [ -n "$realpath" ] && [ "$realpath" != "$path" ]; then
      copy_and_recurse "$realpath"
    fi
  fi

  while read -r line; do
    dep=$(echo "$line" | awk '{for(i=1;i<=NF;i++){ if ($i ~ /^\//) {print $i; break}}}') || true
    [ -z "$dep" ] && continue
    copy_and_recurse "$dep"
  done < <(ldd "$path" 2>/dev/null || true)
}

for exe in "${OUT_DIR}/bin/"*; do
  [ -x "$exe" ] || continue
  while read -r line; do
    path=$(echo "$line" | awk '{for(i=1;i<=NF;i++){ if ($i ~ /^\//) {print $i; break}}}')
    [ -z "$path" ] && continue
    copy_and_recurse "$path"
  done < <(ldd "$exe" 2>/dev/null || true)
done

QT_PLUGIN_DIR=""
if command -v qtpaths-qt6 >/dev/null 2>&1; then
  QT_PLUGIN_DIR=$(qtpaths-qt6 --plugin-dir 2>/dev/null || true)
elif command -v qmake6 >/dev/null 2>&1; then
  QT_PLUGIN_DIR=$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || true)
elif command -v qmake-qt6 >/dev/null 2>&1; then
  QT_PLUGIN_DIR=$(qmake-qt6 -query QT_INSTALL_PLUGINS 2>/dev/null || true)
elif command -v qtpaths >/dev/null 2>&1; then
  QT_PLUGIN_DIR=$(qtpaths --plugin-dir 2>/dev/null || true)
fi

if [ -z "$QT_PLUGIN_DIR" ]; then
  echo "Warning: no Qt plugin tool found; plugin copy skipped." >&2
fi

if [ -n "$QT_PLUGIN_DIR" ] && [ -d "$QT_PLUGIN_DIR" ]; then
  for p in platforms imageformats iconengines platforminputcontexts tls; do
    if [ -d "$QT_PLUGIN_DIR/$p" ]; then
      mkdir -p "${OUT_DIR}/plugins/$p"
      cp -a "$QT_PLUGIN_DIR/$p/"* "${OUT_DIR}/plugins/$p/" || true
    fi
  done

  rm -f "${OUT_DIR}/plugins/platforms/libqeglfs.so"
  rm -f "${OUT_DIR}/plugins/platforms/libqlinuxfb.so"
  rm -f "${OUT_DIR}/plugins/platforms/libqminimalegl.so"
  rm -f "${OUT_DIR}/plugins/platforms/libqvkkhrdisplay.so"
  rm -f "${OUT_DIR}/plugins/platforms/libqvnc.so"

  rm -f "${OUT_DIR}/plugins/imageformats/libqavif"*.so
  rm -f "${OUT_DIR}/plugins/imageformats/libqicns.so"
  rm -f "${OUT_DIR}/plugins/imageformats/libqjp2.so"
  rm -f "${OUT_DIR}/plugins/imageformats/libqmng.so"
  rm -f "${OUT_DIR}/plugins/imageformats/libqpdf.so"
  rm -f "${OUT_DIR}/plugins/imageformats/libqtga.so"
  rm -f "${OUT_DIR}/plugins/imageformats/libqtiff.so"
  rm -f "${OUT_DIR}/plugins/imageformats/libqwbmp.so"

  rm -f "${OUT_DIR}/plugins/platforminputcontexts/libqtvirtualkeyboardplugin.so"

  # Also collect dependencies for Qt plugins, not only the executable.
  if [ -d "${OUT_DIR}/plugins" ]; then
    find "${OUT_DIR}/plugins" \( -type f -o -type l \) \( -name '*.so' -o -name '*.so.*' \) | while read -r plugin; do
      if file "$plugin" | grep -q 'ELF'; then
        while read -r line; do
          path=$(echo "$line" | awk '{for(i=1;i<=NF;i++){ if ($i ~ /^\//) {print $i; break}}}')
          [ -z "$path" ] && continue
          copy_and_recurse "$path"
        done < <(ldd "$plugin" 2>/dev/null || true)
      fi
    done
  fi
else
  echo "Warning: Qt plugin directory not found; plugin copy skipped." >&2
fi

cat > "${OUT_DIR}/bin/qt.conf" <<EOF
[Paths]
Plugins=../plugins
Libraries=../lib
EOF

echo "Dependencies collected into ${OUT_DIR}"
