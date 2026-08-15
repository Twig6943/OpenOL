# Building the mod

# Requirements
- Outlast source
- [Visual studio 2012](https://archive.org/details/en_visual_studio_professional_2012_x86_dvd)
- [DirectX SDK](https://download.microsoft.com/download/A/E/7/AE743F1F-632B-4809-87A9-AA1BB3458E31/DXSDK_Jun10.exe) you can find [an extracted copy here if you need it for some reason](https://github.com/testing-laboratory/DirectX-SDK-June2010)

**We won't be providing any links to outlast source here.** Once you've got it downloaded, you'll need to extract the following folders;

```
┌─── Binaries/        5 GB
├─── Development/
│    ├── External/    30 GB
│    ├── Src/         8.6 GB
│    └── Tools/       350 MB
├─── Engine/          300 MB
└─── OLGame/          22.6 GB
```

Copy `Development` , `OLGame` in the root of this repository to `$OUTLASTSRC/Development` and press replace when asked.

### 1.Build UnrealBuildTool

Rebuild this before building UnrealScript as it's needed sometimes

### 2. Build openol-relay

### 3. Build binaries (OLGame, OutlastLauncher)

### 4.Build UnrealScript & Cook Packages

If you get weird errors/crashes when building UnrealScript make sure to build the game's .exe/C++ part cleanly first.

# Launch options

make

-make

-nosteam

-log

-forcedebuginfo

-silent

-seekfreeloading

-seekfreeloadingpcconsole

-VERBOSE

# Containerfile (Experimental)

Place `Outlast1.7z` to the root of this repository, then run this;

```sh
podman build --device /dev/dri -t openol -f CI/Containerfile .
```

Keep in mind this is might not work as I don't have a vps with enough storage to test it.

# Disclaimer about the CI

Define the link to `$OUTLASTSRC` as a secret. It won't work on free runners simply because of the fact that game's source code is over 20+ gigabytes. You must use a premium runner or a self-hosted one.

See [LINUX.md](./LINUX.md) & [the workflows](../.github/workflows) for more details.
