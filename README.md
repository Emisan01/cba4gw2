# ColorblindAssist

![Build](https://github.com/Emisan01/ColorblindAssist/actions/workflows/build.yml/badge.svg)

**A transparent, system-wide color aid for Windows.**

ColorblindAssist applies an adjustable color correction matrix through the
Windows Magnification API. It was born from a practical Guild Wars 2 use case,
but works across the Windows desktop wherever a system-level color aid helps.

The interface keeps the important controls visible: choose a color profile,
adjust intensity, and watch the RGB transformation respond in real time.

The tool is intended as a practical visual aid. It is not a medical device and
does not restore or diagnose color vision.

## Why it exists

Many color filters are fixed presets that change everything at once. This tool
keeps correction strength adjustable and makes the transformation visible,
so each person can find a setting that works for their own display and vision.

## Features

- System-wide color correction
- Adjustable correction intensity
- Protan, Deutan, Tritan, and Mixed modes
- Optional Anomaloscope AQ and HRR input
- Live RGB transformation graph
- English and German UI
- Saved settings and optional Windows startup
- Global `Ctrl+Alt+C` toggle
- Single-instance protection

## Download and run

Prebuilt Windows downloads are available in the [Releases](https://github.com/Emisan01/ColorblindAssist/releases) section.

For the smallest package, use `Start-ColorblindAssist.cmd` from a framework-
dependent publish. It checks for the .NET 8 Desktop Runtime and opens the
official Microsoft download page when needed.

For a self-contained package, publish with the `WinX64` profile. That version
includes the runtime and can run without a separate .NET installation.

## Requirements

- Windows 10 or Windows 11 for the modern .NET 8 build
- Windows Magnification API support

The color-effect API is available from Windows 8, but .NET 8 is not supported
on Windows 8.1. Supporting Windows 8/8.1 requires a separately tested legacy
build.

## Build

Framework-dependent development build:

```text
dotnet build
dotnet run
```

Portable Windows x64 build with the .NET runtime included:

```text
dotnet publish --profile WinX64
```

Slim Windows x64 build without an embedded runtime:

```text
dotnet publish -c Release -r win-x64 --self-contained false /p:PublishSingleFile=true
```

## Important limitations

- Exclusive fullscreen applications may bypass the Desktop Window Manager.
  Borderless or windowed mode is recommended for games.
- Windows color filters and this tool can overwrite each other.
- The correction matrices are practical approximations, not clinically
  calibrated conversions.
- AQ-to-severity mapping is heuristic and should not be treated as a medical
  measurement.

## Project structure

- `SettingsForm.cs` - Windows Forms UI and interaction logic
- `ColorMatrix.cs` - color simulation and correction matrices
- `ColorCurveView.cs` - live RGB graph
- `DiagnosticMapper.cs` - optional AQ/HRR approximation mapping
- `Magnification.cs` - Windows API wrapper and filter lifecycle
- `AppPreferences.cs` - local settings and Windows startup registration

## Contributing

Feedback from people with color vision deficiency is especially valuable.
Please open an issue with the Windows version, selected profile, and what was
better or worse. Do not include medical records or other personal information.

## License

Released under the [MIT License](LICENSE).
