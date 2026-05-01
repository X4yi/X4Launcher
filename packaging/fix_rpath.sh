#!/usr/bin/env bash
set -euo pipefail

# Set relative rpath for the main executable and optional shared libraries.
# Usage: packaging/fix_rpath.sh [binary] [lib-dir]

BINARY=${1:-dist/bin/X4Launcher}
LIB_DIR=${2:-dist/lib}

if [ ! -x "${BINARY}" ] && [[ ! "${BINARY}" == *.exe ]]; then
  echo "Executable not found: ${BINARY}" >&2
  exit 1
fi

if file "${BINARY}" | grep -q 'PE32+'; then
  echo "Windows binary detected; skipping RPATH fix."
  exit 0
fi

if [ ! -d "${LIB_DIR}" ]; then
  echo "Library directory not found: ${LIB_DIR}" >&2
  exit 1
fi

if ! command -v patchelf >/dev/null 2>&1; then
  echo "patchelf not available; install it before running this script." >&2
  echo "On Arch: sudo pacman -S patchelf" >&2
  echo "On Debian/Ubuntu: sudo apt install patchelf" >&2
  exit 1
fi

RPATH='\$ORIGIN/../lib'
patchelf --set-rpath "${RPATH}" "${BINARY}"

echo "Set RPATH=${RPATH} on ${BINARY}"

# Optionally patch Qt plugin binaries if they are ELF and need RUNPATH.
find "${LIB_DIR}" -type f \( -name '*.so' -o -name '*.so.*' \) | while read -r lib; do
  if file "$lib" | grep -q 'ELF'; then
    libdir=$(dirname "$lib")
    relative_rpath=$(realpath --relative-to="$libdir" "${LIB_DIR}")
    if [ "$relative_rpath" = "." ]; then
      relative_rpath=""
    fi
    patchelf --set-rpath "\$ORIGIN${relative_rpath:+/$relative_rpath}" "$lib" || true
  fi
done

echo "Patched RPATH for ELF shared objects in ${LIB_DIR}"
PLUGIN_DIR="$(dirname "${LIB_DIR}")/plugins"
if [ -d "${PLUGIN_DIR}" ]; then
  find "${PLUGIN_DIR}" -type f \( -name '*.so' -o -name '*.so.*' \) | while read -r lib; do
    if file "$lib" | grep -q 'ELF'; then
      libdir=$(dirname "$lib")
      relative_rpath=$(realpath --relative-to="$libdir" "${LIB_DIR}")
      if [ "$relative_rpath" = "." ]; then
        relative_rpath=""
      fi
      patchelf --set-rpath "\$ORIGIN${relative_rpath:+/$relative_rpath}" "$lib" || true
    fi
  done
  echo "Patched RPATH for ELF shared objects in ${PLUGIN_DIR}"
fi