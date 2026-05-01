# X4Launcher Windows Native Build Script
# =========================================
# Run from "Developer PowerShell for VS 2022" or any terminal with MSVC in PATH.
#
# Usage:
#   .\packaging\build_windows_native.ps1
#   .\packaging\build_windows_native.ps1 -QtRoot "D:\Qt\6.7.2\msvc2022_64"
#   .\packaging\build_windows_native.ps1 -BuildType Debug

param(
    [string]$QtRoot = "",
    [string]$BuildType = "Release",
    [switch]$SkipDeploy = $false,
    [switch]$Help = $false
)

$ErrorActionPreference = "Stop"

if ($Help) {
    Write-Host @"
X4Launcher Windows Build Script

Options:
  -QtRoot <path>    Path to Qt installation (e.g. C:\Qt\6.7.2\msvc2022_64)
                    Auto-detected from environment if not specified.
  -BuildType <type> Release (default) or Debug
  -SkipDeploy       Skip windeployqt step
  -Help             Show this help

Prerequisites:
  - Visual Studio 2022 with "Desktop development with C++" workload
  - Qt 6.5+ for MSVC 2022 (64-bit)
  - CMake 3.20+ and Ninja (bundled with VS)
"@
    exit 0
}

# ── Resolve paths ────────────────────────────────────────────────────────────
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$Root = Split-Path -Parent $ScriptDir
$BuildDir = Join-Path $Root "build_windows"
$DistDir = Join-Path $Root "dist" "windows"

# ── Find Qt ──────────────────────────────────────────────────────────────────
if (-not $QtRoot) {
    # Try environment variables
    foreach ($envVar in @("X4_QT_ROOT", "Qt6_ROOT", "QT_ROOT_DIR")) {
        $val = [Environment]::GetEnvironmentVariable($envVar)
        if ($val -and (Test-Path $val)) {
            $QtRoot = $val
            break
        }
    }
}

if (-not $QtRoot) {
    # Try common paths
    $commonPaths = @(
        "C:\Qt\6.7.2\msvc2022_64",
        "C:\Qt\6.6.3\msvc2022_64",
        "C:\Qt\6.5.3\msvc2022_64",
        "$env:USERPROFILE\Qt\6.7.2\msvc2022_64"
    )
    foreach ($p in $commonPaths) {
        if (Test-Path $p) {
            $QtRoot = $p
            break
        }
    }
}

if (-not $QtRoot -or -not (Test-Path $QtRoot)) {
    Write-Host "ERROR: Qt installation not found." -ForegroundColor Red
    Write-Host "Specify with: -QtRoot 'C:\Qt\6.x.x\msvc2022_64'" -ForegroundColor Yellow
    exit 1
}

$WindeployQt = Join-Path $QtRoot "bin" "windeployqt.exe"
if (-not (Test-Path $WindeployQt)) {
    # Try windeployqt6
    $WindeployQt = Join-Path $QtRoot "bin" "windeployqt6.exe"
}

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "  X4Launcher Windows Build" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan
Write-Host "Qt Root:    $QtRoot" -ForegroundColor DarkGray
Write-Host "Build Type: $BuildType" -ForegroundColor DarkGray
Write-Host "Build Dir:  $BuildDir" -ForegroundColor DarkGray
Write-Host "Dist Dir:   $DistDir" -ForegroundColor DarkGray
Write-Host ""

# ── Check tools ──────────────────────────────────────────────────────────────
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "ERROR: cmake not found in PATH." -ForegroundColor Red
    Write-Host "Open 'Developer PowerShell for VS 2022' or add cmake to PATH." -ForegroundColor Yellow
    exit 1
}

# Prefer Ninja, fall back to "NMake Makefiles"
$generator = "Ninja"
$ninja = Get-Command ninja -ErrorAction SilentlyContinue
if (-not $ninja) {
    Write-Host "WARN: Ninja not found, using NMake Makefiles." -ForegroundColor Yellow
    $generator = "NMake Makefiles"
}

# ── Configure ────────────────────────────────────────────────────────────────
Write-Host "[1/4] Configuring..." -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

cmake -B $BuildDir -G $generator `
    -DCMAKE_BUILD_TYPE=$BuildType `
    -DX4_QT_ROOT="$QtRoot" `
    -DCMAKE_PREFIX_PATH="$QtRoot" `
    $Root

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: CMake configure failed." -ForegroundColor Red
    exit 1
}

# ── Build ────────────────────────────────────────────────────────────────────
Write-Host "[2/4] Building..." -ForegroundColor Green
cmake --build $BuildDir --config $BuildType

if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed." -ForegroundColor Red
    exit 1
}

# ── Deploy ───────────────────────────────────────────────────────────────────
Write-Host "[3/4] Deploying..." -ForegroundColor Green
New-Item -ItemType Directory -Force -Path $DistDir | Out-Null

# Find the built executable
$ExePath = Join-Path $BuildDir "X4Launcher.exe"
if (-not (Test-Path $ExePath)) {
    # Try in the Release subdir (multi-config generators)
    $ExePath = Join-Path $BuildDir $BuildType "X4Launcher.exe"
}
if (-not (Test-Path $ExePath)) {
    Write-Host "ERROR: X4Launcher.exe not found in build directory." -ForegroundColor Red
    exit 1
}

Copy-Item $ExePath $DistDir -Force

if (-not $SkipDeploy) {
    if (Test-Path $WindeployQt) {
        & $WindeployQt `
            --release `
            --no-translations `
            --no-system-d3d-compiler `
            --no-opengl-sw `
            --compiler-runtime `
            (Join-Path $DistDir "X4Launcher.exe")

        if ($LASTEXITCODE -ne 0) {
            Write-Host "WARN: windeployqt returned non-zero exit code." -ForegroundColor Yellow
        }

        # Verify TLS plugin was deployed (critical for HTTPS)
        $tlsDir = Join-Path $DistDir "tls"
        if (-not (Test-Path $tlsDir) -or (Get-ChildItem $tlsDir -ErrorAction SilentlyContinue).Count -eq 0) {
            Write-Host "WARNING: TLS plugin not found! HTTPS will not work." -ForegroundColor Red
            Write-Host "Manually copy qschannelbackend.dll to $tlsDir" -ForegroundColor Yellow
        } else {
            Write-Host "  TLS plugin OK" -ForegroundColor DarkGreen
        }
    } else {
        Write-Host "WARN: windeployqt not found at $WindeployQt" -ForegroundColor Yellow
        Write-Host "You'll need to manually copy Qt DLLs." -ForegroundColor Yellow
    }
}

# Copy metadata files
Copy-Item (Join-Path $Root "LICENSE.txt") $DistDir -Force -ErrorAction SilentlyContinue
Copy-Item (Join-Path $Root "README.md") $DistDir -Force -ErrorAction SilentlyContinue

# ── Package ──────────────────────────────────────────────────────────────────
Write-Host "[4/4] Packaging ZIP..." -ForegroundColor Green
$ZipPath = Join-Path $Root "X4Launcher-windows.zip"
if (Test-Path $ZipPath) { Remove-Item $ZipPath }
Compress-Archive -Path "$DistDir\*" -DestinationPath $ZipPath -Force

Write-Host ""
Write-Host "============================================" -ForegroundColor Green
Write-Host "  BUILD COMPLETE" -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host "Executable: $DistDir\X4Launcher.exe"
Write-Host "Package:    $ZipPath"
Write-Host ""
Write-Host "Test on a clean Windows machine to verify all DLLs are present." -ForegroundColor DarkGray
