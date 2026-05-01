#!/usr/bin/env bash
set -euo pipefail

# Test launching the shipped binary in a minimal environment using env -i
# Usage: packaging/test_env.sh [dist-dir] [binary] [--platform PLATFORM]
# Example: packaging/test_env.sh --platform offscreen dist_tmp

DIST_DIR=dist
BIN=""
PLATFORM=""
ARGS=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --platform)
      PLATFORM="${2:-}"
      shift 2
      ;;
    --platform=*)
      PLATFORM="${1#*=}"
      shift
      ;;
    *)
      ARGS+=("$1")
      shift
      ;;
  esac
 done

if [ ${#ARGS[@]} -gt 0 ]; then
  DIST_DIR="${ARGS[0]}"
fi
if [ ${#ARGS[@]} -gt 1 ]; then
  BIN="${ARGS[1]}"
else
  BIN="${DIST_DIR}/bin/X4Launcher"
fi

if [ ! -x "${BIN}" ]; then
  echo "Executable not found or not executable: ${BIN}" >&2
  exit 1
fi

APPDIR=$(cd "$(dirname "$BIN")/.." && pwd)

echo "Running ${BIN} in a minimal env to detect missing libs/plugins..."

if [ -z "${PLATFORM:-}" ]; then
  if [ -z "${DISPLAY:-}" ] && [ -z "${WAYLAND_DISPLAY:-}" ]; then
    echo "No display detected; forcing offscreen platform for headless package validation."
    PLATFORM=offscreen
  else
    PLATFORM="${QT_QPA_PLATFORM:-}"
  fi
fi
TEST_DISPLAY="${DISPLAY:-}"
TEST_WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-}"
if [ "${PLATFORM}" = "offscreen" ]; then
  TEST_DISPLAY=""
  TEST_WAYLAND_DISPLAY=""
fi
TEST_XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/tmp}"
TEST_XAUTHORITY="${XAUTHORITY:-}"

echo "Using QT_QPA_PLATFORM=${PLATFORM:-default} for test_env.sh"
env -i \
  LANG=C.UTF-8 \
  LC_ALL=C.UTF-8 \
  PATH=/usr/bin:/bin \
  LD_LIBRARY_PATH="${APPDIR}/lib" \
  QT_PLUGIN_PATH="${APPDIR}/plugins" \
  QT_QPA_PLATFORM="${PLATFORM}" \
  QT_QPA_PLATFORM_PLUGIN_PATH="${APPDIR}/plugins/platforms" \
  DISPLAY="${TEST_DISPLAY}" \
  WAYLAND_DISPLAY="${TEST_WAYLAND_DISPLAY}" \
  XDG_RUNTIME_DIR="${TEST_XDG_RUNTIME_DIR}" \
  XAUTHORITY="${TEST_XAUTHORITY}" \
  HOME=${HOME:-/tmp} \
  "${BIN}" --no-sandbox || true

echo "If the app fails, inspect output and run ldd on the binary and plugins." 
