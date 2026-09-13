# Deploys the freshly built addon DLL to the GW2 addons directory.
#
# If GW2/Nexus still has the current DLL loaded, the copy simply fails
# and is skipped - no renaming, no backup files. Close the game (or
# unload the addon in Nexus) and rebuild to deploy.
#
# Always exits 0 - a blocked deploy should not fail the whole build.
#
# Adapted from cba4gw2's own deploy_dll.ps1 (2026-09-13) - see that
# project's copy for the read-only-marker rationale (Nexus's "Library"
# addon handling silently reverting a deployed DLL on its own).

param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Destination
)

$destName = Split-Path -Leaf $Destination

try {
    if (Test-Path $Destination) {
        Set-ItemProperty -Path $Destination -Name IsReadOnly -Value $false -ErrorAction SilentlyContinue
    }
    Copy-Item -Path $Source -Destination $Destination -Force -ErrorAction Stop
    Set-ItemProperty -Path $Destination -Name IsReadOnly -Value $true -ErrorAction SilentlyContinue
    Write-Host "Deployed $destName -> $Destination (marked read-only)"

    Get-ChildItem -Path (Split-Path $Destination) -Filter "$destName.old*" -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
}
catch {
    Write-Warning "Deploy skipped - $destName still loaded (close GW2 or unload the addon, then rebuild)."
}

exit 0
