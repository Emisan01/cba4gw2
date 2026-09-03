# ColorblindAssist

ColorblindAssist is a small Windows desktop accessibility helper that applies
a system-wide color correction matrix through the Windows Magnification API.
It provides adjustable Protan, Deutan, Tritan, and Mixed profiles, a live RGB
curve view, saved settings, and optional diagnostic-value input.

The tool is intended as a practical visual aid. It is not a medical device and
does not restore or diagnose color vision.

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

For the slim build, use `Start-ColorblindAssist.cmd`. It checks for the .NET 8
Desktop Runtime and opens the official Microsoft download page when needed.

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
