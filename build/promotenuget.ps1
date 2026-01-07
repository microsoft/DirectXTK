<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
This promotes the NuGet packages on the project-scoped feed.

.DESCRIPTION
This script promotes the views of the DirectXTK NuGet packages on the project-scoped feed. It always promotes to Prerelease view, and if the Release switch is set, it also promotes to Release view.

.PARAMETER Version
Indicates which version of the packages to promote.

.PARAMETER PAT
Requires an ADO PAT with 'Packaging > Read, write, and manage' scope. Can be provided via the ADO_PERSONAL_ACCESS_TOKEN environment variable or as a parameter.

.PARAMETER Release
By default promotes to prerelease. If this switch is set, promotes to release as well.

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

param(
    [Parameter(Mandatory)]
    [string]$Version,
    [string]$PAT = "",
    [switch]$Release
)

# Parse PAT
if ($PAT.Length -eq 0) {
    $PAT = $env:ADO_PERSONAL_ACCESS_TOKEN

    if ($PAT.Length -eq 0) {
        Write-Error "##[error]This script requires a valid ADO Personal Access Token!" -ErrorAction Stop
    }
}

# Project-scoped feed root (package name and version to be filled in later)
$uriFormat = "https://pkgs.dev.azure.com/MSCodeHub/ab27a052-7f0e-4cba-9bec-d298c5942ab9/_apis/packaging/feeds/a95f2e27-1bf9-40f4-b609-f113d1a50de6/nuget/packages/{0}/versions/{1}?api-version=7.1"

$headers = @{
    "Content-Type" = "application/json"
    Authorization = "Basic " + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(":$PAT"))
}

$bodyPrerelease = @{
    views = @{
        op    = "add"
        path  = "/views/-"
        value = "Prerelease"
    }
} | ConvertTo-Json

$bodyRelease = @{
    views = @{
        op    = "add"
        path  = "/views/-"
        value = "Release"
    }
} | ConvertTo-Json

$packages = @('directxtk_desktop_2019', 'directxtk_desktop_win10', 'directxtk_uwp')

# Check if all packages exist
$allPackagesSucceeded = $true
foreach ($package in $packages) {
    $uri = $uriFormat -f $package, $Version

    try
    {
        Write-Host "Checking if $package version $Version exists..."
        Invoke-RestMethod -Uri $uri -Method Get -Headers $headers
    }
    catch
    {
        Write-Error "##[error]Package $package version $Version not found!" -ErrorAction Continue
        $allPackagesSucceeded = $false
    }
}

if (-not $allPackagesSucceeded) {
    Write-Error "##[error]Not all packages found. Aborting promotion." -ErrorAction Stop
}

# Promote package to Prerelease view
foreach ($package in $packages) {
    $uri = $uriFormat -f $package, $Version

    try
    {
        # Promote to Prerelease view
        Write-Host "Promoting $package version $Version to Prerelease view..."
        Invoke-RestMethod -Uri $uri -Method Patch -Headers $headers -Body $bodyPrerelease
    }
    catch
    {
        Write-Error "##[error]Package $package version $Version failed to promote" -ErrorAction Continue
        $allPackagesSucceeded = $false
    }
}

if (-not $allPackagesSucceeded) {
    Write-Error "##[error]Not all packages promoted to Prerelease." -ErrorAction Stop
}

# Optionally promote package to Release view
if ($Release.IsPresent) {
    foreach ($package in $packages) {
        $uri = $uriFormat -f $package, $Version

        try
        {
            # Promote to Release view
            Write-Host "Promoting $package version $Version to Release view..."
            Invoke-RestMethod -Uri $uri -Method Patch -Headers $headers -Body $bodyRelease
        }
        catch
        {
            Write-Error "##[error]Package $package version $Version failed to promote" -ErrorAction Continue
            $allPackagesSucceeded = $false
        }
    }

    if (-not $allPackagesSucceeded) {
        Write-Error "##[error]Not all packages promoted to Release." -ErrorAction Stop
    }
}
