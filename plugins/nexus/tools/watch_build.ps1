# CBA auto-build watcher.
#
# Run this ONCE and leave the window open while we work. It watches
# plugins/nexus/src and CMakeLists.txt for changes (including edits made
# by Claude through the device bridge) and automatically rebuilds Release
# ~2s after things settle - no manual "cmake --build" needed anymore.
# CMakeLists.txt already auto-deploys cba.dll to the GW2 addons folder on
# a successful build, so this also covers deploy + hot-reload readiness.
#
# Output goes to build_log.txt (full log, appended) and build_status.txt
# (just "SUCCESS <timestamp>" or "FAILED <timestamp>" for a quick check).
#
# Run with:  powershell -ExecutionPolicy Bypass -File tools\watch_build.ps1

$ErrorActionPreference = "Continue"

$repoRoot   = Split-Path -Parent $PSScriptRoot   # plugins/nexus
$srcDir     = Join-Path $repoRoot "src"
$cmakeFile  = Join-Path $repoRoot "CMakeLists.txt"
$logFile    = Join-Path $repoRoot "build_log.txt"
$statusFile = Join-Path $repoRoot "build_status.txt"
$settleSeconds = 2

function Get-LatestMtime {
    $files = @(Get-ChildItem -Path $srcDir -Recurse -File -ErrorAction SilentlyContinue)
    $files += Get-Item $cmakeFile -ErrorAction SilentlyContinue
    if ($files.Count -eq 0) { return [datetime]::MinValue }
    return ($files | Measure-Object -Property LastWriteTime -Maximum).Maximum
}

$lastBuilt = Get-LatestMtime   # don't rebuild immediately on startup
$lastSeenChange = $lastBuilt

Write-Host "CBA auto-build watcher started. Watching: $srcDir"
Write-Host "Log: $logFile"
Write-Host "Press Ctrl+C to stop."

while ($true) {
    Start-Sleep -Seconds 1
    $latest = Get-LatestMtime

    if ($latest -gt $lastSeenChange) {
        $lastSeenChange = $latest
    }

    if ($latest -gt $lastBuilt -and ((Get-Date) - $lastSeenChange).TotalSeconds -ge $settleSeconds) {
        $ts = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
        Write-Host "[$ts] Change detected, building..."
        "=== Build triggered $ts ===" | Out-File -FilePath $logFile -Append -Encoding utf8

        Push-Location $repoRoot
        $output = cmake --build build --config Release 2>&1
        $exitCode = $LASTEXITCODE
        Pop-Location

        $output | Out-File -FilePath $logFile -Append -Encoding utf8

        if ($exitCode -eq 0) {
            "SUCCESS $ts" | Out-File -FilePath $statusFile -Encoding utf8
            Write-Host "[$ts] Build OK."
        } else {
            "FAILED $ts" | Out-File -FilePath $statusFile -Encoding utf8
            Write-Host "[$ts] Build FAILED - see build_log.txt"
        }

        $lastBuilt = $latest
    }
}
