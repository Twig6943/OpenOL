# How to setup development environment on linux

# Disclaimer: Make sure the `$WINEPREFIX` you're using is as same as the one in [build_wine.sh](../Src/Targets/build_wine.sh)

### 1.Install visual studio 2012

Wine version confirmed to work: 11.14

Install these winetricks packages

```sh
winetricks -q dotnet20 dotnet40 gdiplus corefonts riched20 atmlib msxml3 msls31
```

Run ` sudo pkill -9 -f "\\.exe" ` if visual studio asks for some sort of restart.

Pick only;
- Microsoft Foundation Classes for C++

### 2. Install DirectX SDK

This one is straight forward just run the installer in the same prefix.

### 3. Building
Cd into `$OUTLASTSRC/Development/Src/Targets` and run `build_wine.sh`

// Troubleshooting (WIP ERROR FIX)

```log
Log: PsyX GPU Support: Disabled
Log: [FSocketWin::Bind] Binding to 0.0.0.0:9989
Warning, 0 Shader compiler errors compiling global for platform pc-d3d-sm3:
Critical: appError called: Failed to compile global shader TFilterPixelShader<16>
Critical: Windows GetLastError: Success. (0)
Log: === Critical error: ===
Failed to compile global shader TFilterPixelShader<16>

Address 0xff32d977 (filename not found) [in C:\windows\system32\kernelbase.dll]
```

winetricks -q d3dcompiler_**
winetricks -q dotnet45 mdx d3d9x d3dcompiler_43 win10
