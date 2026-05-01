#!/usr/bin/env bash
set -euo pipefail

# Create a ZIP archive from a prepared distribution tree.
# Usage: packaging/package_windows.sh [dist-dir] [output-zip]

DIST_DIR=${1:-dist}
OUT_ZIP=${2:-X4Launcher-windows.zip}

if [ ! -d "${DIST_DIR}" ]; then
  echo "Distribution directory ${DIST_DIR} not found. Run collect_deps.sh first." >&2
  exit 1
fi

if ! command -v zip >/dev/null 2>&1; then
  echo "zip not available; install zip before running this script." >&2
  exit 1
fi

rm -f "${OUT_ZIP}"
pushd "${DIST_DIR}" >/dev/null
  if [[ ! "$OUT_ZIP" = /* ]]; then
    OUT_ZIP="../$OUT_ZIP"
  fi
  zip -r "${OUT_ZIP}" . -x "*.DS_Store"
popd >/dev/null

echo "Created ${OUT_ZIP}"
