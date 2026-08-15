# Packaging

If you wish to ship a release zip like we do that users can just drag and drop to their game files, just run [publish.sh](../Development/Src/Targets/publish.sh) which should give you a ready-to-ship `OpenOL.7z` file in that's located in `$OUTLASTSRC/PublishTemp` . Users might need to start their game with `-log -nosteam -seekfreeloadingpcconsole` after installing `OpenOL.7z` .

Here's a file tree of a 7z file we ship for reference;

```sh
┌─── Binaries/
│    └─── Win64/
│         └─── OLGame.exe
├─── OutlastLauncher.exe
└─── OLGame/
     ├─── Config/
     │    ├── DefaultEngine.ini
     │    ├── DefaultGame.ini
     │    ├── DefaultMultiplayer.ini
     │    └── DefaultUI.ini
     ├─── CookedPCConsole/
     │    ├── Core.upk
     │    ├── Engine.upk
     │    ├── GameFramework.upk
     │    ├── IpDrv.upk
     │    ├── OLFrontEnd.upk
     │    ├── OnlineSubsystemPC.upk
     │    ├── OnlineSubsystemSteamworks.upk
     │    ├── OpenOL/
     │    │    ├── Multiplayer.u
     │    │    └── OLGame.u
     │    └── menuassets.upk
     └─── Localization/
          ├── INT/
          │    └── OLGame.int
          └── RUS/
               └── olgame.RUS
```
