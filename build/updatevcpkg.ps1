<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Updates the vcpkg port for DirectXTK to match a GitHub release.

.DESCRIPTION
This script updates the vcpkg port at D:\vcpkg\ports\directxtk to match a specific
GitHub release by tag. It updates the version-date in vcpkg.json, the tag and SHA512
hashes in portfile.cmake for the source archive, MakeSpriteFont.exe, xwbtool.exe,
and xwbtool_arm64.exe.

.PARAMETER Tag
The GitHub release tag (e.g., 'may2026', 'mar2026'). Defaults to the latest tag.

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

param(
    [string]$Tag = ""
)

$repoRoot = Split-Path -Path $PSScriptRoot -Parent
$readme = Join-Path $repoRoot "README.md"
$portDir = "D:\vcpkg\ports\directxtk"
$vcpkgJson = Join-Path $portDir "vcpkg.json"
$portfile = Join-Path $portDir "portfile.cmake"

if (-Not (Test-Path $readme)) {
    Write-Error "ERROR: Cannot find README.md at $readme" -ErrorAction Stop
}

if ((-Not (Test-Path $vcpkgJson)) -Or (-Not (Test-Path $portfile))) {
    Write-Error "ERROR: Cannot find vcpkg port files at $portDir" -ErrorAction Stop
}

# Determine tag from latest git tag if not provided
if ($Tag.Length -eq 0) {
    $Tag = (git --no-pager -C $repoRoot tag --sort=-creatordate | Select-Object -First 1).Trim()
    if ($Tag.Length -eq 0) {
        Write-Error "ERROR: Failed to determine latest tag!" -ErrorAction Stop
    }
}

Write-Host "Release Tag: $Tag"

# Parse release date from README.md (format: "## Month Day, Year")
$rawReleaseDate = (Get-Content $readme) | Select-String -Pattern "^##\s+[A-Z][a-z]+\s+\d+,?\s+\d{4}" | Select-Object -First 1
if ([string]::IsNullOrEmpty($rawReleaseDate)) {
    Write-Error "ERROR: Failed to find release date in README.md!" -ErrorAction Stop
}

$releaseDateStr = ($rawReleaseDate -replace '^##\s+', '').Trim()
$releaseDate = [datetime]::Parse($releaseDateStr, [System.Globalization.CultureInfo]::InvariantCulture)
$versionDate = $releaseDate.ToString("yyyy-MM-dd")

Write-Host "Release Date: $releaseDateStr"
Write-Host "Version Date: $versionDate"

# --- Update vcpkg.json ---
Write-Host "`nUpdating vcpkg.json..."

$jsonContent = Get-Content $vcpkgJson -Raw
$jsonContent = $jsonContent -replace '"version-date":\s*"[^"]*"', "`"version-date`": `"$versionDate`""
$jsonContent = $jsonContent -replace ',\s*"port-version":\s*\d+', ''
$jsonContent = $jsonContent -replace '"port-version":\s*\d+,?\s*', ''
Set-Content -Path $vcpkgJson -Value $jsonContent -NoNewline

Write-Host "  version-date set to $versionDate"

# --- Update portfile.cmake tag ---
Write-Host "`nUpdating portfile.cmake tag..."

$portContent = Get-Content $portfile -Raw
$portContent = $portContent -replace 'set\(DIRECTXTK_TAG\s+\S+\)', "set(DIRECTXTK_TAG $Tag)"
Set-Content -Path $portfile -Value $portContent -NoNewline

Write-Host "  Tag set to $Tag"

# --- Download and hash source archive ---
$ProgressPreference = 'SilentlyContinue'
$tempDir = Join-Path $Env:Temp $(New-Guid)
New-Item -Type Directory -Path $tempDir | Out-Null

$sourceUrl = "https://github.com/Microsoft/DirectXTK/archive/refs/tags/$Tag.tar.gz"
$sourcePath = Join-Path $tempDir "$Tag.tar.gz"

Write-Host "`nDownloading source archive from $sourceUrl..."
try {
    Invoke-WebRequest -Uri $sourceUrl -OutFile $sourcePath -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download source archive!" -ErrorAction Stop
}

$sourceHash = (Get-FileHash -Path $sourcePath -Algorithm SHA512).Hash.ToLower()
Write-Host "  Source SHA512: $sourceHash"

# Replace SHA512 in vcpkg_from_github block
$portContent = Get-Content $portfile -Raw
$portContent = $portContent -replace '(vcpkg_from_github\s*\([^)]*SHA512\s+)[0-9a-fA-F]+', "`${1}$sourceHash"
Set-Content -Path $portfile -Value $portContent -NoNewline

# --- Download and hash MakeSpriteFont.exe ---
$makespriteFontUrl = "https://github.com/Microsoft/DirectXTK/releases/download/$Tag/MakeSpriteFont.exe"
$makespriteFontPath = Join-Path $tempDir "MakeSpriteFont.exe"

Write-Host "`nDownloading MakeSpriteFont.exe from $makespriteFontUrl..."
try {
    Invoke-WebRequest -Uri $makespriteFontUrl -OutFile $makespriteFontPath -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download MakeSpriteFont.exe!" -ErrorAction Stop
}

$makespriteFontHash = (Get-FileHash -Path $makespriteFontPath -Algorithm SHA512).Hash.ToLower()
Write-Host "  MakeSpriteFont.exe SHA512: $makespriteFontHash"

# --- Download and hash XWBTool.exe ---
$xwbtoolUrl = "https://github.com/Microsoft/DirectXTK/releases/download/$Tag/XWBTool.exe"
$xwbtoolPath = Join-Path $tempDir "XWBTool.exe"

Write-Host "`nDownloading XWBTool.exe from $xwbtoolUrl..."
try {
    Invoke-WebRequest -Uri $xwbtoolUrl -OutFile $xwbtoolPath -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download XWBTool.exe!" -ErrorAction Stop
}

$xwbtoolHash = (Get-FileHash -Path $xwbtoolPath -Algorithm SHA512).Hash.ToLower()
Write-Host "  XWBTool.exe SHA512: $xwbtoolHash"

# --- Download and hash XWBTool_arm64.exe ---
$xwbtoolArm64Url = "https://github.com/Microsoft/DirectXTK/releases/download/$Tag/XWBTool_arm64.exe"
$xwbtoolArm64Path = Join-Path $tempDir "XWBTool_arm64.exe"

Write-Host "`nDownloading XWBTool_arm64.exe from $xwbtoolArm64Url..."
try {
    Invoke-WebRequest -Uri $xwbtoolArm64Url -OutFile $xwbtoolArm64Path -ErrorAction Stop
}
catch {
    Write-Error "ERROR: Failed to download XWBTool_arm64.exe!" -ErrorAction Stop
}

$xwbtoolArm64Hash = (Get-FileHash -Path $xwbtoolArm64Path -Algorithm SHA512).Hash.ToLower()
Write-Host "  XWBTool_arm64.exe SHA512: $xwbtoolArm64Hash"

# --- Replace SHA512 hashes for tool binaries in portfile ---
$portContent = Get-Content $portfile -Raw

# Match the vcpkg_download_distfile block for MakeSpriteFont.exe
$portContent = $portContent -replace `
    '(vcpkg_download_distfile\s*\(\s*\n\s*MAKESPRITEFONT_EXE\s*\n\s*URLS\s+"[^"]*MakeSpriteFont\.exe"\s*\n\s*FILENAME\s+"[^"]*"\s*\n\s*SHA512\s+)[0-9a-fA-F]+', `
    "`${1}$makespriteFontHash"

# Match the vcpkg_download_distfile block for XWBTool.exe (x64)
$portContent = $portContent -replace `
    '(vcpkg_download_distfile\s*\(\s*\n\s*XWBTOOL_EXE\s*\n\s*URLS\s+"[^"]*XWBTool\.exe"\s*\n\s*FILENAME\s+"[^"]*"\s*\n\s*SHA512\s+)[0-9a-fA-F]+', `
    "`${1}$xwbtoolHash"

# Match the vcpkg_download_distfile block for XWBTool_arm64.exe
$portContent = $portContent -replace `
    '(vcpkg_download_distfile\s*\(\s*\n\s*XWBTOOL_EXE\s*\n\s*URLS\s+"[^"]*XWBTool_arm64\.exe"\s*\n\s*FILENAME\s+"[^"]*"\s*\n\s*SHA512\s+)[0-9a-fA-F]+', `
    "`${1}$xwbtoolArm64Hash"

Set-Content -Path $portfile -Value $portContent -NoNewline

# --- Cleanup ---
Remove-Item -Recurse -Force $tempDir

Write-Host "`nvcpkg port updated successfully!"
Write-Host "`nUpdated files:"
Write-Host "  $vcpkgJson"
Write-Host "  $portfile"

$portContent = Get-Content $portfile -Raw
if ($portContent -match '\bPATCHES\b') {
    Write-Warning "This port includes patches. Review them to either remove or update."
}
