#!/usr/bin/env bash
# Build script for OL1 project components via Wine + MSVC 2012.

set -e

# ---- Wine environment -----------------------------------------------------
export WINEPREFIX="$HOME/.local/share/wineprefixes/outlast/OL1Compile"
export WINEARCH=win64
export WINEDEBUG=-all

export VS110COMNTOOLS="$VS_ROOT_W\\Common7\\Tools\\"
export WindowsSdkDir="$SDK_ROOT_W\\"
export LIB="$VS_ROOT_W\\VC\\lib\\amd64;$SDK_ROOT_W\\Lib\\win8\\um\\x64;$DX_ROOT_W\\Lib\\x64"
export FrameworkDir="C:\\windows\\Microsoft.NET\\Framework64\\"
export FrameworkVersion="v4.0.30319"
export PROCESSOR_ARCHITECTURE="AMD64"
export DXSDK_DIR="$DX_ROOT_W\\"
export WINEPATH="$VS_ROOT/Common7/IDE;$VS_ROOT/VC/bin/amd64"
export INCLUDE="$VS_ROOT_W\\VC\\include;$VS_ROOT_W\\VC\\atlmfc\\include;$SDK_ROOT_W\\Include\\um;$SDK_ROOT_W\\Include\\shared;$SDK_ROOT_W\\Include\\winrt;$DX_ROOT_W\\Include"


# ---- Paths ----------------------------------------------------------------

WINEPREFIX_C="$WINEPREFIX/drive_c"
VS_ROOT="$WINEPREFIX_C/Program Files (x86)/Microsoft Visual Studio 11.0"
SDK_ROOT="$WINEPREFIX_C/Program Files (x86)/Windows Kits/8.0"
DX_ROOT="$WINEPREFIX_C/Program Files (x86)/Microsoft DirectX SDK (June 2010)"

VS_ROOT_W="C:\\Program Files (x86)\\Microsoft Visual Studio 11.0"
SDK_ROOT_W="C:\\Program Files (x86)\\Windows Kits\\8.0"
DX_ROOT_W="C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SRC_DIR="$SCRIPT_DIR/.."                                        # Development/Src
DEV_DIR="$SRC_DIR/.."                                           # Development
TOOLS_DIR="$DEV_DIR/Tools"

UBT_EXE="$(realpath "$DEV_DIR/Intermediate/UnrealBuildTool/Release/UnrealBuildTool.exe")"
UBT_CSPROJ="$(realpath "$SRC_DIR/UnrealBuildTool/UnrealBuildTool.csproj")"
LAUNCHER_VCXPROJ="$(realpath "$TOOLS_DIR/OutlastLauncher/OutlastLauncher.vcxproj")"

MSBUILD="C:\\windows\\Microsoft.NET\\Framework64\\v4.0.30319\\MSBuild.exe"

# ---- Helper: run MSBuild --------------------------------------------------
run_msbuild() {
    local PROJ="$1"
    local CONFIG="$2"
    local EXTRA="${3:-}"
    # Convert Linux path to Wine Z: path
    local PROJ_W="Z:$(echo "$PROJ" | sed 's|/|\\|g')"
    wine "$MSBUILD" "$PROJ_W" \
        /p:Configuration="$CONFIG" \
        /p:Platform="$4" \
        /verbosity:minimal \
        /nologo \
        $EXTRA
}

# ---- Helper: run UBT ------------------------------------------------------
run_ubt() {
    local CONFIG="$1"
    local REBUILD="${2:-}"
    cd "$SRC_DIR"
    if [ -n "$REBUILD" ]; then
        # Delete all obj files to force full rebuild
        echo "Cleaning obj files..."
        find "$(realpath "$DEV_DIR/Intermediate/OLGame")" -name "*.obj" -delete 2>/dev/null || true
    fi
    wine "$UBT_EXE" OLGame Win64 "$CONFIG" -DEPLOY \
        -output "../../Binaries/Win64/OLGame.exe" -nopdb -define WITH_STEAMWORKS=1
}

# ---- Menu -----------------------------------------------------------------
echo ""
echo "  OL1 Build Menu"
echo "  ─────────────────────────────────"
echo "  1) Build OLGame.exe        (UBT, incremental)"
echo "  2) Rebuild OLGame.exe      (UBT, clean obj)"
echo "  3) Build OutlastLauncher   (MSVC, Release|Win32)"
echo "  4) Build UnrealBuildTool   (MSBuild, Release)"
echo ""
printf "  Choice [1-4]: "
read -r CHOICE

case "$CHOICE" in
    1)
        echo "[build_wine] Build OLGame.exe (incremental)"
        run_ubt Release
        ;;
    2)
        echo "[build_wine] Rebuild OLGame.exe (clean)"
        run_ubt Release "rebuild"
        ;;
    3)
        echo "[build_wine] Build OutlastLauncher"
        run_msbuild "$LAUNCHER_VCXPROJ" "Release" "" "Win32"
        ;;
    4)
        echo "[build_wine] Build UnrealBuildTool"
        UBT_OUT_W="Z:$(realpath "$DEV_DIR/Intermediate/UnrealBuildTool/Release/" | sed 's|/|\\|g')\\"
        run_msbuild "$UBT_CSPROJ" "Release" "/p:OutputPath=\"$UBT_OUT_W\"" "AnyCPU"
        ;;
    *)
        echo "Invalid choice."
        exit 1
        ;;
esac

echo "[build_wine] Done."
