#!/usr/bin/env bash
set -euo pipefail

# Create a tar.gz archive from a prepared distribution tree.
# Usage: packaging/package_linux.sh [dist-dir] [output-tar.gz]

DIST_DIR=${1:-dist}
OUT_TAR=${2:-X4Launcher-linux.tar.gz}

if [ ! -d "${DIST_DIR}" ]; then
  echo "Distribution directory ${DIST_DIR} not found. Run collect_deps.sh first." >&2
  exit 1
fi

rm -f "${OUT_TAR}"

tar -C "$(dirname "${DIST_DIR}")" -czf "${OUT_TAR}" "$(basename "${DIST_DIR}")"

echo "Created ${OUT_TAR}"
