#!/usr/bin/env bash
# Build script for OL1 project components via Wine + MSVC 2012.

set -e

# ---- Configure before use ------------------------------------------------
WINEPREFIX="$HOME/.local/share/prefixes/OL1Compile"
COOK_ARGS="-platform=PCConsole -multilanguagecook=INT"
# --------------------------------------------------------------------------

if [ -z "$WINEPREFIX" ]; then
    echo "ERROR: WINEPREFIX is not set. Edit this file and set WINEPREFIX."
    exit 1
fi

# ---- Paths ----------------------------------------------------------------
# On this machine the full MSVC 2012 install, the Windows 8.0 SDK and the
# C++ build targets all live under "Program Files (x86)".  The non-x86
# "Microsoft Visual Studio 11.0" dir is only a partial install (Common7).
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
OL1_DIR="$(realpath "$DEV_DIR/..")"                             # OL1 root
OLGAME_EXE="$OL1_DIR/Binaries/Win64/OLGame.exe"

UBT_EXE="$(realpath "$DEV_DIR/Intermediate/UnrealBuildTool/Release/UnrealBuildTool.exe")"
UBT_CSPROJ="$(realpath "$SRC_DIR/UnrealBuildTool/UnrealBuildTool.csproj")"
LAUNCHER_VCXPROJ="$(realpath "$TOOLS_DIR/OutlastLauncher/OutlastLauncher.vcxproj")"
RELAY_VCXPROJ="$(realpath "$DEV_DIR/External/openol_relay/OpenOL_Relay.vcxproj")"

MSBUILD="C:\\windows\\Microsoft.NET\\Framework64\\v4.0.30319\\MSBuild.exe"

# ---- Wine environment -----------------------------------------------------

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

# ---- Helper: run MSBuild --------------------------------------------------
run_msbuild() {
    local PROJ="$1"
    local CONFIG="$2"
    local PLATFORM="$3"
    shift 3
    # Convert Linux path to Wine Z: path
    local PROJ_W="Z:$(echo "$PROJ" | sed 's|/|\\|g')"
    wine "$MSBUILD" "$PROJ_W" \
        /p:Configuration="$CONFIG" \
        /p:Platform="$PLATFORM" \
        /verbosity:minimal \
        /nologo \
        "$@"
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
# Optional non-interactive mode: pass the option as an argument, e.g.:
#   ./build_wine.sh 1   # Build OLGame.exe (UBT, incremental)
CHOICE="${1:-}"
if [ -z "$CHOICE" ]; then
    echo ""
    echo "  OL1 Build Menu"
    echo "  ─────────────────────────────────"
    echo "  1) Build UnrealBuildTool   (MSBuild, Release)"
    echo "  2) Build openol_relay      (MSVC, Release|x64)"
    echo "  3) Build OLGame.exe        (UBT, incremental)"
    echo "  4) Rebuild OLGame.exe      (UBT, clean obj)"
    echo "  5) Build OutlastLauncher   (MSVC, Release|Win32)"
    echo "  6) Build UnrealScript      (make -auto)"
    echo "  7) Rebuild UnrealScript    (make -auto -full)"
    echo "  8) Cook Packages           (CookPackages \$COOK_ARGS)"
    echo ""
    printf "  Choice [1-8]: "
    read -r CHOICE
fi

open_link() {
    echo ""
    echo "  Output: $1"
}

case "$CHOICE" in
    1)
        echo "[build_wine] Build UnrealBuildTool"
        UBT_OUT_W="Z:$(realpath "$DEV_DIR/Intermediate/UnrealBuildTool/Release/" | sed 's|/|\\|g')\\"
        run_msbuild "$UBT_CSPROJ" "Release" "AnyCPU" "/p:OutputPath=$UBT_OUT_W"
        open_link "$(realpath "$DEV_DIR/Intermediate/UnrealBuildTool/Release/UnrealBuildTool.exe")"
        ;;
    2)
        echo "[build_wine] Build openol_relay"
        wine "$MSBUILD" \
            "Z:$(echo "$RELAY_VCXPROJ" | sed 's|/|\\|g')" \
            /p:Configuration=Release \
            /p:Platform=x64 \
            "/p:VCTargetsPath=C:\\Program Files (x86)\\MSBuild\\Microsoft.Cpp\\v4.0\\V110\\" \
            /p:TrackFileAccess=false \
            "/p:WindowsSdkDir=$SDK_ROOT_W\\" \
            "/p:VCInstallDir=$VS_ROOT_W\\VC\\" \
            /verbosity:minimal /nologo
        open_link "$(realpath "$DEV_DIR/External/openol_relay/lib/openol_relay.lib")"
        ;;
    3)
        echo "[build_wine] Build OLGame.exe (incremental)"
        run_ubt Release
        open_link "$(realpath "$DEV_DIR/../Binaries/Win64/OLGame.exe")"
        ;;
    4)
        echo "[build_wine] Rebuild OLGame.exe (clean)"
        run_ubt Release "rebuild"
        open_link "$(realpath "$DEV_DIR/../Binaries/Win64/OLGame.exe")"
        ;;
    5)
        echo "[build_wine] Build OutlastLauncher"
        wine "$MSBUILD" \
            "Z:$(echo "$LAUNCHER_VCXPROJ" | sed 's|/|\\|g')" \
            /p:Configuration=Release \
            /p:Platform=Win32 \
            "/p:VCTargetsPath=C:\\Program Files (x86)\\MSBuild\\Microsoft.Cpp\\v4.0\\V110\\" \
            /p:TrackFileAccess=false \
            "/p:WindowsSdkDir=$SDK_ROOT_W\\" \
            "/p:VCInstallDir=$VS_ROOT_W\\VC\\" \
            /verbosity:minimal /nologo
        open_link "$(realpath "$TOOLS_DIR/OutlastLauncher/Release/OutlastLauncher.exe")"
        ;;
    6|7|8)
        if [ ! -f "$OLGAME_EXE" ]; then
            echo "WARNING: OLGame.exe not found: $OLGAME_EXE"
            echo "         Build it first with option 3 or 4."
            exit 1
        fi
        case "$CHOICE" in
            6)
                echo "[build_wine] Build UnrealScript"
                WINEPATH="" wine "$OLGAME_EXE" make -auto
                ;;
            7)
                echo "[build_wine] Rebuild UnrealScript"
                WINEPATH="" wine "$OLGAME_EXE" make -auto -full
                ;;
            8)
                echo "[build_wine] Cook Packages: $COOK_ARGS"
                # shellcheck disable=SC2086
                WINEPATH="" wine "$OLGAME_EXE" CookPackages $COOK_ARGS
                ;;
        esac
        ;;
    *)
        echo "Invalid choice."
        exit 1
        ;;
esac

echo "[build_wine] Done."
