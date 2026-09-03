@echo off
setlocal

where dotnet >nul 2>nul
if errorlevel 1 goto runtime_missing

dotnet --list-runtimes | findstr /C:"Microsoft.WindowsDesktop.App 8." >nul
if errorlevel 1 goto runtime_missing

start "Colorblind Assist" "%~dp0ColorblindAssist.exe"
exit /b 0

:runtime_missing
echo The Microsoft .NET 8 Desktop Runtime is required.
echo Opening the official Microsoft download page...
start "" "https://dotnet.microsoft.com/en-us/download/dotnet/8.0"
pause
exit /b 1
