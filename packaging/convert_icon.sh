#!/usr/bin/env bash
# Convert the X4Launcher icon PNG to Windows .ico format and set up Linux icon.
# Prerequisites: imagemagick (convert/magick)
#
# Usage: ./packaging/convert_icon.sh <input.png>
#        ./packaging/convert_icon.sh resources/icons/x4launcher.png

set -euo pipefail

ROOT_DIR=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
INPUT="${1:-${ROOT_DIR}/resources/icons/x4launcher.png}"

if [ ! -f "$INPUT" ]; then
    echo "Error: Input PNG not found: $INPUT" >&2
    echo "Usage: $0 <path-to-256x256-png>" >&2
    exit 1
fi

CONVERT="convert"
if command -v magick >/dev/null 2>&1; then
    CONVERT="magick"
fi

echo "Source: $INPUT"

# 1. Resize to 256x256 if needed and place as Linux deploy icon
echo "Creating Linux deploy icon..."
mkdir -p "${ROOT_DIR}/deploy"
$CONVERT "$INPUT" -resize 256x256 "${ROOT_DIR}/deploy/x4launcher.png"
echo "  -> deploy/x4launcher.png"

# 2. Also keep a copy in resources/icons/
$CONVERT "$INPUT" -resize 256x256 "${ROOT_DIR}/resources/icons/x4launcher.png"
echo "  -> resources/icons/x4launcher.png"

# 3. Create Windows .ico with multiple sizes (256, 128, 64, 48, 32, 16)
echo "Creating Windows .ico..."
$CONVERT "$INPUT" \
    -define icon:auto-resize=256,128,64,48,32,16 \
    "${ROOT_DIR}/resources/icons/x4launcher.ico"
echo "  -> resources/icons/x4launcher.ico"

echo ""
echo "Done! Icon files created:"
echo "  Linux:   deploy/x4launcher.png (256x256)"
echo "  Windows: resources/icons/x4launcher.ico (multi-resolution)"
echo ""
echo "The .ico will be embedded in the .exe via resources/x4launcher.rc"
