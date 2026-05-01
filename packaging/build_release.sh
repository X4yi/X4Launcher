#!/usr/bin/env bash
set -euo pipefail

# Builds the project in Release and installs into a staging directory
# Usage: packaging/build_release.sh [build-dir] [staging-dir] [toolchain-file]

BUILD_DIR=${1:-build}
STAGING_DIR=${2:-${BUILD_DIR}/package}
TOOLCHAIN_FILE=${3:-}
NUMJOBS=${4:-$(nproc || echo 1)}

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)

if [[ "${BUILD_DIR}" != /* ]]; then
  BUILD_DIR="${ROOT_DIR}/${BUILD_DIR}"
fi

if [[ "${STAGING_DIR}" != /* ]]; then
  STAGING_DIR="${ROOT_DIR}/${STAGING_DIR}"
fi

mkdir -p "${BUILD_DIR}"
mkdir -p "${STAGING_DIR}"
cd "${BUILD_DIR}"

CMAKE_ARGS=(-DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr)
if [ -n "${TOOLCHAIN_FILE}" ]; then
  CMAKE_ARGS+=(-DCMAKE_TOOLCHAIN_FILE="${TOOLCHAIN_FILE}")
fi

cmake "${CMAKE_ARGS[@]}" "${ROOT_DIR}"
cmake --build . --config Release -j"${NUMJOBS}"

rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}"

# Install into staging using DESTDIR so files go under ${STAGING_DIR}/usr/...
DESTDIR="${STAGING_DIR}" cmake --install . --prefix /usr

echo "Build and install complete. Staging at: ${STAGING_DIR}"
