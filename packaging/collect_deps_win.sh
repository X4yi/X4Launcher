#!/usr/bin/env bash
# Collect Windows dependencies for X4Launcher
# This script uses windeployqt to automatically resolve and copy Qt and MinGW DLLs.
# It also verifies critical runtime components (TLS plugins, platform plugins).

set -euo pipefail

DIST_DIR=${1:-dist/windows}
PREFIX="x86_64-w64-mingw32"
WINDEPLOYQT="${PREFIX}-windeployqt"

if [ ! -d "${DIST_DIR}" ]; then
    echo "Error: Distribution directory ${DIST_DIR} not found." >&2
    exit 1
fi

echo "Collecting Windows dependencies into ${DIST_DIR}..."

EXE="${DIST_DIR}/bin/X4Launcher.exe"

if [ ! -f "$EXE" ]; then
    echo "Error: X4Launcher.exe not found in ${DIST_DIR}/bin/" >&2
    exit 1
fi

if ! command -v "$WINDEPLOYQT" >/dev/null 2>&1; then
    echo "Error: $WINDEPLOYQT not found. Please install the MinGW Qt6 toolchain (e.g., mingw-w64-qt6-base on Arch Linux)." >&2
    exit 1
fi

# Use windeployqt to resolve Qt dependencies, plugins, and compiler runtime
# The --compiler-runtime flag automatically copies libstdc++-6.dll, libgcc_s_seh-1.dll, etc.
"$WINDEPLOYQT" --release \
               --no-translations \
               --no-system-d3d-compiler \
               --no-opengl-sw \
               --compiler-runtime \
               --dir "${DIST_DIR}/bin" \
               "$EXE"

# Create a qt.conf to ensure plugins are loaded correctly relative to the exe.
cat > "${DIST_DIR}/bin/qt.conf" <<EOF
[Paths]
Plugins = .
EOF

# ── Verification ─────────────────────────────────────────────────────────────

echo ""
echo "Verifying deployment..."

ERRORS=0

# Check for platform plugin (critical: app won't start without it)
if [ ! -d "${DIST_DIR}/bin/platforms" ] || [ -z "$(ls -A "${DIST_DIR}/bin/platforms/" 2>/dev/null)" ]; then
    echo "  [FAIL] platforms/ plugin directory missing or empty!" >&2
    ERRORS=$((ERRORS + 1))
else
    echo "  [OK]   platforms/ plugin found"
fi

# Check for TLS plugin (critical: HTTPS won't work without it)
TLS_FOUND=0
for tlsdir in "${DIST_DIR}/bin/tls" "${DIST_DIR}/bin/plugins/tls"; do
    if [ -d "$tlsdir" ] && [ -n "$(ls -A "$tlsdir" 2>/dev/null)" ]; then
        TLS_FOUND=1
        echo "  [OK]   TLS plugin found in $(basename "$tlsdir")/"
        break
    fi
done
if [ $TLS_FOUND -eq 0 ]; then
    echo "  [WARN] TLS plugin NOT found! HTTPS network requests will fail silently." >&2
    echo "         Microsoft auth, version downloads, Modrinth will be broken." >&2
    ERRORS=$((ERRORS + 1))
fi

# Check for imageformats (needed for icon rendering)
if [ -d "${DIST_DIR}/bin/imageformats" ]; then
    echo "  [OK]   imageformats/ plugin found"
else
    echo "  [WARN] imageformats/ not found (icons may not render)" >&2
fi

# Check for styles (optional but nice)
if [ -d "${DIST_DIR}/bin/styles" ]; then
    echo "  [OK]   styles/ plugin found"
fi

echo ""
if [ $ERRORS -gt 0 ]; then
    echo "WARNING: $ERRORS critical issue(s) found. See above." >&2
else
    echo "Windows dependency collection complete — all checks passed."
fi
