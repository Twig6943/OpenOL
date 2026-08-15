# Packaging

If you wish to ship a release zip like we do that users can just drag and drop to their game files, you'll need to include the configs from [docs/OLGame/Config](./OLGame/Config) . These aren't in the main OLGame folder they're only needed by the game client user downloads from their platform (steam/gog) . After you're done running tests on a regular game client just run [publish.sh](../publish.sh) which should give you a ready-to-ship `OpenOL.7z` file

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
