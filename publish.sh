#!/bin/bash
set -e

# Place this to the root of your game installation.

ROOT="$(cd "$(dirname "$0")" && pwd)"
DEST="$ROOT/PublishTemp"
ARCHIVE="$DEST/OpenOL.7z"

FILES=(
    "OutlastLauncher.exe"
    "Binaries/Win64/OLGame.exe"
    "OLGame/CookedPCConsole/OpenOL/Multiplayer.u"
    "OLGame/CookedPCConsole/OpenOL/OLGame.u"
    "OLGame/CookedPCConsole/Engine.upk"
    "OLGame/CookedPCConsole/Core.upk"
    "OLGame/CookedPCConsole/GameFramework.upk"
    "OLGame/CookedPCConsole/IpDrv.upk"
    "OLGame/CookedPCConsole/OLFrontEnd.upk"
    "OLGame/CookedPCConsole/menuassets.upk"
    "OLGame/CookedPCConsole/OnlineSubsystemPC.upk"
    "OLGame/CookedPCConsole/OnlineSubsystemSteamworks.upk"
    "OLGame/Localization/RUS/olgame.RUS"
    "OLGame/Localization/INT/OLGame.int"
    "OLGame/Config/DefaultUI.ini"
    "OLGame/Config/DefaultEngine.ini"
    "OLGame/Config/DefaultMultiplayer.ini"
    "OLGame/Config/DefaultGame.ini"
)

echo "Copying files to $DEST..."
for f in "${FILES[@]}"; do
    src="$ROOT/$f"
    dst="$DEST/$f"
    mkdir -p "$(dirname "$dst")"
    cp "$src" "$dst"
    echo "  $f"
done

echo "Creating archive $ARCHIVE..."
rm -f "$ARCHIVE"
cd "$DEST"
7z a -mx=9 -mmt=on "$ARCHIVE" "${FILES[@]}"

echo "Done: $ARCHIVE"
