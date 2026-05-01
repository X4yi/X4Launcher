#!/usr/bin/env bash
set -euo pipefail

# Creates the final zip archive from the collected distribution tree
# Usage: packaging/package_zip.sh [dist-dir] [output-zip]

DIST_DIR=${1:-dist}
OUT_ZIP=${2:-X4Launcher-linux.zip}

if [ ! -d "${DIST_DIR}" ]; then
  echo "Distribution directory ${DIST_DIR} not found. Run collect_deps.sh first." >&2
  exit 1
fi

rm -f "${OUT_ZIP}"
pushd "${DIST_DIR}" >/dev/null
  # Ensure top-level structure: bin/ lib/ plugins/ data/
  zip -r "../${OUT_ZIP}" . -x "*.DS_Store"
popd >/dev/null

echo "Created ${OUT_ZIP}"
