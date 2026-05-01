#!/usr/bin/env bash
set -euo pipefail

# Strip unneeded symbols from packaged ELF binaries and shared libraries.
# Usage: packaging/strip_binaries.sh [dist-dir]

DIST_DIR=${1:-dist}

if [ ! -d "${DIST_DIR}" ]; then
  echo "Distribution directory ${DIST_DIR} not found. Run collect_deps.sh first." >&2
  exit 1
fi

if ! command -v strip >/dev/null 2>&1; then
  echo "strip not available; install binutils or equivalent." >&2
  exit 1
fi

find "${DIST_DIR}" -type f \( -name 'X4Launcher' -o -name 'X4Launcher.exe' -o -name '*.so' -o -name '*.so.*' -o -name '*.dll' \) | while read -r file; do
  if file "${file}" | grep -q 'ELF'; then
    strip --strip-unneeded "${file}" || true
  elif file "${file}" | grep -q 'PE32+ executable'; then
    if command -v x86_64-w64-mingw32-strip >/dev/null 2>&1; then
      x86_64-w64-mingw32-strip --strip-unneeded "${file}" || true
    fi
  fi
done

echo "Stripped binaries in ${DIST_DIR}"
