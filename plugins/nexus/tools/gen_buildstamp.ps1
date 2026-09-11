# Regenerates src/BuildStamp.h with a simple incrementing local-build counter,
# stamped into AddonVersion.Build - see the comment on the cba_gen_buildstamp
# CMake target for why this exists. Used to be Build=hour/Revision=minute*100+
# second (any number that provably changes every build, without needing
# persisted state) - changed 2026-09-11 after Emi pointed out that display
# ("1.0.8.5439") reads as "build #5439", not a timestamp, and asked for an
# actual +1-per-build counter instead, simple as that.

param(
    [Parameter(Mandatory = $true)][string]$OutFile
)

# Counter file lives next to this script (not inside build/), so it survives
# a clean rebuild of the CMake build directory - it only resets if this file
# itself is deleted. Gitignored (plugins/nexus/tools/.build_counter) - purely
# local dev bookkeeping, never meant to be shared/synced across machines.
$counterFile = Join-Path $PSScriptRoot ".build_counter"
$counter = 1
if (Test-Path $counterFile) {
    $raw = (Get-Content -Path $counterFile -Raw).Trim()
    $parsed = 0
    if ([int]::TryParse($raw, [ref]$parsed)) { $counter = $parsed + 1 }
}
Set-Content -Path $counterFile -Value $counter -NoNewline

$content = @"
#pragma once
// Auto-generated every build by tools/gen_buildstamp.ps1 - do not edit, do not commit.
#define CBA_BUILD_STAMP_COUNTER $counter
"@

Set-Content -Path $OutFile -Value $content
Write-Host "Build stamp: Build=$counter"
