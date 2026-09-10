# Deploys the freshly built cba.dll to the GW2 addons directory.
#
# If GW2/Nexus still has the current cba.dll loaded, the copy simply fails
# and is skipped - no renaming, no backup files. Close the game (or
# unload the addon in Nexus) and rebuild to deploy.
#
# Always exits 0 - a blocked deploy should not fail the whole build.
#
# Read-only marker (2026-09-09): Nexus appears to manage cba4gw2 as a
# "Library" addon and keeps silently reverting addons/cba.dll back to an
# older build (same byte size every time) - observed repeatedly even with
# "Disable Auto-Updates" checked, across multiple game restarts, with no
# manual copying involved. Marking the file read-only after each of our own
# deploys is a countermeasure: our own Copy-Item -Force below still
# overwrites it fine (Force clears read-only first), but if whatever is
# reverting it writes without -Force, it should now fail visibly instead of
# silently succeeding - which would at least confirm what's doing this.

param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Destination
)

try {
    if (Test-Path $Destination) {
        Set-ItemProperty -Path $Destination -Name IsReadOnly -Value $false -ErrorAction SilentlyContinue
    }
    Copy-Item -Path $Source -Destination $Destination -Force -ErrorAction Stop
    Set-ItemProperty -Path $Destination -Name IsReadOnly -Value $true -ErrorAction SilentlyContinue
    Write-Host "Deployed cba.dll -> $Destination (marked read-only)"

    # Not ours - Nexus's own hot-reload appears to rename the previously-
    # loaded DLL aside to *.old when it picks up a changed file while the
    # game is running. Wrong extension so it can never be loaded as an
    # addon, but tidy up any leftovers after a clean deploy anyway.
    Get-ChildItem -Path (Split-Path $Destination) -Filter "cba.dll.old*" -ErrorAction SilentlyContinue |
        Remove-Item -Force -ErrorAction SilentlyContinue
}
catch {
    Write-Warning "Deploy skipped - cba.dll still loaded (close GW2 or unload the addon, then rebuild)."
}

exit 0
