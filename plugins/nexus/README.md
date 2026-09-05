# cba4gw2 Nexus Plugin

This directory contains the source code for `cba.dll`, a standalone plugin for the GW2 Nexus addon loader.

The plugin provides:

- Protan, Deutan, Tritan, and mixed correction profiles
- Adjustable correction strength
- Diagnosis hint input for choosing a precise starting point
- German/English option labels
- Real-time RGB transfer curves and dynamic filtered spectrum beam
- Detachable floating HUD (`ALT+C`) with glassmorphic transparency
- Window-mode detection with automatic neutral restoration when tabbing out

## Build

From this directory with Visual Studio 2022 and CMake:

```text
cmake -S . -B build -A x64
cmake --build build --config Release
```

The resulting `cba.dll` is written to `build/bin/Release/cba.dll` and can be
copied to the Nexus addons directory. The vendored Nexus and ImGui headers are
kept in `thirdparty/` so the plugin build has zero external dependencies.

## Install (release package)

1. Download `cba.dll` from [Releases](https://github.com/Emisan01/cba4gw2/releases).
2. Copy `cba.dll` into your Nexus addons folder: `<Guild Wars 2>/addons/`.
3. Start Guild Wars 2 through Nexus and open Nexus options → `cba4gw2` to configure the addon.
4. Use Windowed or Windowed Fullscreen (Borderless) display mode in GW2 graphics settings.

The plugin uses the Windows Magnification API and should be used in windowed
or borderless GW2 mode. Exclusive fullscreen bypasses the desktop compositor,
so the effect is disabled there and the options panel explains why.

