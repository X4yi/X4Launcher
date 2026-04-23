#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR=$(cd "$(dirname "$0")" && pwd)
BUILD_DIR="${ROOT_DIR}/build"
STAGING_DIR="${BUILD_DIR}/package"
DIST_DIR="${ROOT_DIR}/dist"
OUTPUT_LINUX="X4Launcher-linux.tar.gz"
OUTPUT_WINDOWS="X4Launcher-windows.zip"
MODE="linux"

usage() {
  cat <<EOF
Usage: ./distribute.sh [--linux|--windows|--zip] [--build-dir DIR] [--staging-dir DIR] [--dist-dir DIR] [--output FILE]

Options:
  --linux            Build and package for Linux (default).
  --windows          Build and package for Windows ZIP distribution.
  --zip              Build and create a generic ZIP archive.
  --build-dir DIR    Build directory (default: build).
  --staging-dir DIR  Staging directory (default: build/package).
  --dist-dir DIR     Distribution output folder (default: dist).
  --output FILE      Output archive name.
  -h, --help         Show this help message.
EOF
  exit 1
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --linux)
      MODE="linux"
      shift
      ;;
    --windows)
      MODE="windows"
      shift
      ;;
    --zip)
      MODE="zip"
      shift
      ;;
    --build-dir)
      BUILD_DIR="${2:?Missing build dir}"
      shift 2
      ;;
    --staging-dir)
      STAGING_DIR="${2:?Missing staging dir}"
      shift 2
      ;;
    --dist-dir)
      DIST_DIR="${2:?Missing dist dir}"
      shift 2
      ;;
    --output)
      OUTPUT="${2:?Missing output file}"
      shift 2
      ;;
    -h|--help)
      usage
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      ;;
  esac
 done

if [[ "${MODE}" == "windows" ]]; then
  BUILD_DIR="${ROOT_DIR}/build_windows"
fi
STAGING_DIR="${BUILD_DIR}/package"

if [[ -n "${OUTPUT:-}" ]]; then
  if [[ "${MODE}" == "linux" ]]; then
    OUTPUT_LINUX="${OUTPUT}"
  elif [[ "${MODE}" == "windows" || "${MODE}" == "zip" ]]; then
    OUTPUT_WINDOWS="${OUTPUT}"
  fi
fi

mkdir -p "${ROOT_DIR}"
mkdir -p "${DIST_DIR}"

echo "Building project in ${BUILD_DIR}..."
TOOLCHAIN_FILE=""
if [[ "${MODE}" == "windows" ]]; then
  TOOLCHAIN_FILE="${ROOT_DIR}/cmake/mingw-w64.cmake"
fi
bash "${ROOT_DIR}/packaging/build_release.sh" "${BUILD_DIR}" "${STAGING_DIR}" "${TOOLCHAIN_FILE}"

echo "Collecting dependencies into ${DIST_DIR}..."
if [[ "${MODE}" == "windows" ]]; then
  # Copy the executable first
  mkdir -p "${DIST_DIR}/bin"
  cp -a "${STAGING_DIR}/usr/bin/"* "${DIST_DIR}/bin/" || true
  bash "${ROOT_DIR}/packaging/collect_deps_win.sh" "${DIST_DIR}"
else
  bash "${ROOT_DIR}/packaging/collect_deps.sh" "${STAGING_DIR}" "${DIST_DIR}"
fi

echo "Fixing runtime library paths..."
bash "${ROOT_DIR}/packaging/fix_rpath.sh" "${DIST_DIR}/bin/X4Launcher" "${DIST_DIR}/lib"

case "${MODE}" in
  linux)
    echo "Packaging Linux tarball..."
    bash "${ROOT_DIR}/packaging/package_linux.sh" "${DIST_DIR}" "${OUTPUT_LINUX}"
    ;;
  windows)
    echo "Packaging Windows ZIP..."
    bash "${ROOT_DIR}/packaging/package_windows.sh" "${DIST_DIR}" "${OUTPUT_WINDOWS}"
    ;;
  zip)
    echo "Packaging generic ZIP..."
    bash "${ROOT_DIR}/packaging/package_zip.sh" "${DIST_DIR}" "${OUTPUT_WINDOWS}"
    ;;
  *)
    echo "Unsupported mode: ${MODE}" >&2
    exit 1
    ;;
 esac

echo "Distribution complete. Output is in ${ROOT_DIR}/${OUTPUT_LINUX:-${OUTPUT_WINDOWS}}"
