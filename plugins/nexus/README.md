# ColorblindAssist Nexus Plugin

This directory contains the standalone `cba.dll` plugin for the GW2 Nexus
loader. It is independent from the WinForms executable and intentionally has
no Windows autostart registration: Nexus loads the DLL with Guild Wars 2.

The plugin provides:

- Protan, Deutan, Tritan, and mixed correction profiles
- Adjustable correction strength
- AQ/HRR diagnosis hint input for a more precise starting point
- German/English option labels
- A colored beam preview and static commander-symbol contrast view
- A global `Ctrl+Alt+C` toggle through Nexus keybinds
- Exclusive-fullscreen detection with a visible warning

## Build

From this directory with Visual Studio 2022 and CMake:

```text
cmake -S . -B build -A x64
cmake --build build --config Release
```

The resulting `cba.dll` is written to `build/bin/Release/cba.dll` and can be
copied to the Nexus addons directory. The vendored Nexus and ImGui headers are
kept in `thirdparty/` so the plugin build does not depend on the standalone
application or on an autostart helper.

## Install (release package)

1. Download `ColorblindAssist-nexus-plugin-win-x64.zip` from
   [Releases](https://github.com/Emisan01/ColorblindAssist/releases).
2. Extract `cba.dll`.
3. Copy `cba.dll` into your Nexus addons folder (the same directory where other
   GW2 Nexus addons live).
4. Start Guild Wars 2 through Nexus and open Nexus options to configure the
   addon. Use windowed or borderless display mode in GW2 graphics settings.

The plugin uses the Windows Magnification API and should be used in windowed
or borderless GW2 mode. Exclusive fullscreen bypasses the desktop compositor,
so the effect is disabled there and the options panel explains why.

AQ/HRR values are heuristics for choosing a starting point, not a medical
diagnosis or clinically validated conversion.
