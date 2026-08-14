# Building the mod

# Requirements
- Outlast source
- [Visual studio 2012](https://archive.org/details/en_visual_studio_professional_2012_x86_dvd)
- [DirectX SDK](https://download.microsoft.com/download/A/E/7/AE743F1F-632B-4809-87A9-AA1BB3458E31/DXSDK_Jun10.exe) you can find [an extracted copy here if you need it for some reason](https://github.com/testing-laboratory/DirectX-SDK-June2010)

**We won't be providing any links to outlast source here.** Once you've got it, downloaded you'll need to extract the following folders;

```
┌─── Binaries/        5 GB
├─── Development/
│    ├── External/    30 GB
│    ├── Src/         8.6 GB
│    └── Tools/       350 MB
├─── Engine/          300 MB
└─── OLGame/
     ├── Config/      1 MB
     └── Content/     23 GB
```

Copy `Src` & `Tools` in the root of this repository to `$OUTLASTSRC/Development` and press replace when asked.

### Compiling UnrealBuildTool

### Compiling UnrealScript

### Compiling binaries

# Launch options

make
-make
-log
-forcedebuginfo
-silent
-seekfreeloading
-seekfreeloadingpcconsole

# Disclaimer about the CI

It won't work on github free runner simply because of the fact that game's source code is over 14 gigabytes which is what free runners offers. You must use a premium runner or a self-hosted one.

See [LINUX.md](./LINUX.md) & [the workflows](../.github/workflows) for more details.
