#!/bin/bash
# publish.sh — collect mod files from OL1 editor layout into PublishTemp/
# mirroring the destination layout used by the retail game.
#
# Sources:
#   OLGame/Script/*.u        → OLGame/CookedPCConsole/OpenOL/*.u   (Multiplayer, OLGame)
#                            → OLGame/CookedPCConsole/*.upk         (engine packages as .u→.upk)
#   OLGame/Content/04_UI/    → OLGame/CookedPCConsole/              (menuassets, OLFrontEnd)
#   Binaries, Config, Localization — same relative paths

set -e

OL1="$(cd "$(dirname "$0")/../../.." && pwd)"
DEST="$OL1/PublishTemp"
ARCHIVE="$DEST/OpenOL.7z"

# Each entry: "OL1_source_path|retail_dest_path"
declare -a MAPPINGS=(
    # Executables
    "Development/Tools/OutlastLauncher/Release/OutlastLauncher.exe|OutlastLauncher.exe"
    "Binaries/Win64/OLGame.exe|Binaries/Win64/OLGame.exe"

    # UnrealScript packages — mod-specific go to OpenOL/, engine packages as .upk
    "OLGame/Script/Multiplayer.u|OLGame/CookedPCConsole/OpenOL/Multiplayer.u"
    "OLGame/Script/OLGame.u|OLGame/CookedPCConsole/OpenOL/OLGame.u"
    "OLGame/Script/Core.u|OLGame/CookedPCConsole/OpenOL/Core.u"
    "OLGame/Script/Engine.u|OLGame/CookedPCConsole/OpenOL/Engine.u"
    "OLGame/Script/GameFramework.u|OLGame/CookedPCConsole/OpenOL/GameFramework.u"
    "OLGame/Script/IpDrv.u|OLGame/CookedPCConsole/OpenOL/IpDrv.u"
    "OLGame/Script/WinDrv.u|OLGame/CookedPCConsole/OpenOL/WinDrv.u"
    "OLGame/Script/AkAudio.u|OLGame/CookedPCConsole/OpenOL/AkAudio.u"
    "OLGame/Script/GFxUI.u|OLGame/CookedPCConsole/OpenOL/GFxUI.u"
    "OLGame/Script/OnlineSubsystemPC.u|OLGame/CookedPCConsole/OpenOL/OnlineSubsystemPC.u"
    "OLGame/Script/OnlineSubsystemSteamworks.u|OLGame/CookedPCConsole/OpenOL/OnlineSubsystemSteamworks.u"

    # UI content packages
    "OLGame/Content/04_UI/menuassets.upk|OLGame/CookedPCConsole/menuassets.upk"
    "OLGame/Content/04_UI/OLFrontEnd.upk|OLGame/CookedPCConsole/OLFrontEnd.upk"

    # Localization
    "OLGame/Localization/INT/OLGame.int|OLGame/Localization/INT/OLGame.int"
    "OLGame/Localization/RUS/olgame.RUS|OLGame/Localization/RUS/olgame.RUS"

    # Config
    "OLGame/Config/DefaultUI.ini|OLGame/Config/DefaultUI.ini"
    "OLGame/Config/DefaultEngine.ini|OLGame/Config/DefaultEngine.ini"
    "OLGame/Config/DefaultMultiplayer.ini|OLGame/Config/DefaultMultiplayer.ini"
    "OLGame/Config/DefaultGame.ini|OLGame/Config/DefaultGame.ini"
)

echo "Publishing to $DEST ..."
rm -rf "$DEST"

DEST_FILES=()
for entry in "${MAPPINGS[@]}"; do
    src_rel="${entry%%|*}"
    dst_rel="${entry##*|}"
    src="$OL1/$src_rel"
    dst="$DEST/$dst_rel"

    if [ ! -f "$src" ]; then
        echo "  MISSING: $src_rel"
        continue
    fi

    mkdir -p "$(dirname "$dst")"
    cp "$src" "$dst"
    echo "  $src_rel  →  $dst_rel"
    DEST_FILES+=("$dst_rel")
done

echo ""
echo "Creating archive $ARCHIVE ..."
rm -f "$ARCHIVE"
cd "$DEST"
7z a -mx=9 -mmt=on "$ARCHIVE" "${DEST_FILES[@]}"

echo ""
echo "Done: $ARCHIVE"
