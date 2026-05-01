#!/usr/bin/env bash
set -euo pipefail

# Verify the packaged binary and Qt plugins for missing shared libraries.
# Usage: packaging/check_ldd.sh [dist-dir]

DIST_DIR=${1:-dist}
BINARY=${2:-${DIST_DIR}/bin/X4Launcher}

if [ ! -x "${BINARY}" ]; then
  echo "Executable not found: ${BINARY}" >&2
  exit 1
fi

echo "Checking main binary: ${BINARY}"
ldd "${BINARY}"

echo
if [ -d "${DIST_DIR}/plugins" ]; then
  find "${DIST_DIR}/plugins" \( -type f -o -type l \) | while read -r plugin; do
    printf '\nChecking plugin: %s\n' "$plugin"
    ldd "$plugin"
  done
fi

echo "ldd verification complete. No 'not found' lines should appear."