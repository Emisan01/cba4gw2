# Regenerates src/BuildStamp.h with the current build time, stamped into
# AddonVersion.Build (hour) / .Revision (minute*100+second) - see the
# comment on the cba_gen_buildstamp CMake target for why this exists.

param(
    [Parameter(Mandatory = $true)][string]$OutFile
)

$now = Get-Date
$hour = $now.Hour
$minsec = $now.Minute * 100 + $now.Second

$content = @"
#pragma once
// Auto-generated every build by tools/gen_buildstamp.ps1 - do not edit, do not commit.
#define CBA_BUILD_STAMP_HOUR $hour
#define CBA_BUILD_STAMP_MINSEC $minsec
"@

Set-Content -Path $OutFile -Value $content
Write-Host "Build stamp: Build=$hour Revision=$minsec"
