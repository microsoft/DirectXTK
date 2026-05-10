<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Updates the winget manifests for MakeSpriteFont and XWBTool to match a GitHub release.

.DESCRIPTION
This script creates new winget manifest versions for MakeSpriteFont and XWBTool under
D:\winget-pkgs based on the most recent release date in README.md. It copies the previous
version's manifests, then updates PackageVersion, ReleaseDate, InstallerSha256, InstallerUrl,
and ReleaseNotesUrl to match the new release.

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
$wingetPkgs = "D:\winget-pkgs"

$makespriteFontManifestBase = Join-Path $wingetPkgs "manifests\m\Microsoft\DirectX\ToolKit\MakeSpriteFont"
$xwbtoolManifestBase = Join-Path $wingetPkgs "manifests\m\Microsoft\DirectX\ToolKit\XWBTool"

if (-Not (Test-Path $readme)) {
    Write-Error "ERROR: Cannot find README.md at $readme" -ErrorAction Stop
}

if (-Not (Test-Path $wingetPkgs)) {
    Write-Error "ERROR: Cannot find winget-pkgs at $wingetPkgs" -ErrorAction Stop
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

# winget version format: YYYY.M.D (no leading zeros)
$packageVersion = $releaseDate.ToString("yyyy.M.d")

# winget ReleaseDate format: YYYY-MM-DD
$releaseDateYaml = $releaseDate.ToString("yyyy-MM-dd")

Write-Host "Release Date: $releaseDateStr"
Write-Host "Package Version: $packageVersion"
Write-Host "Release Date (YAML): $releaseDateYaml"

# --- Find previous version directories ---
function Get-LatestVersionDir {
    param([string]$BasePath)
    $dirs = Get-ChildItem -Path $BasePath -Directory | Sort-Object Name
    if ($dirs.Count -eq 0) {
        Write-Error "ERROR: No existing version directories found in $BasePath" -ErrorAction Stop
    }
    return $dirs[-1]
}

$prevMakeSpriteFont = Get-LatestVersionDir $makespriteFontManifestBase
$prevXwbtool = Get-LatestVersionDir $xwbtoolManifestBase

Write-Host "`nPrevious MakeSpriteFont version: $($prevMakeSpriteFont.Name)"
Write-Host "Previous XWBTool version: $($prevXwbtool.Name)"

# Check if new version already exists
$newMakeSpriteFontDir = Join-Path $makespriteFontManifestBase $packageVersion
$newXwbtoolDir = Join-Path $xwbtoolManifestBase $packageVersion

if (Test-Path $newMakeSpriteFontDir) {
    Write-Error "ERROR: MakeSpriteFont version $packageVersion already exists at $newMakeSpriteFontDir" -ErrorAction Stop
}

if (Test-Path $newXwbtoolDir) {
    Write-Error "ERROR: XWBTool version $packageVersion already exists at $newXwbtoolDir" -ErrorAction Stop
}

# --- Download release assets and compute SHA256 hashes ---
$ProgressPreference = 'SilentlyContinue'
$tempDir = Join-Path $Env:Temp $(New-Guid)
New-Item -Type Directory -Path $tempDir | Out-Null

$assets = @(
    @{ Name = "MakeSpriteFont.exe"; Url = "https://github.com/microsoft/DirectXTK/releases/download/$Tag/MakeSpriteFont.exe" },
    @{ Name = "XWBTool.exe"; Url = "https://github.com/microsoft/DirectXTK/releases/download/$Tag/XWBTool.exe" },
    @{ Name = "XWBTool_arm64.exe"; Url = "https://github.com/microsoft/DirectXTK/releases/download/$Tag/XWBTool_arm64.exe" }
)

$hashes = @{}

foreach ($asset in $assets) {
    $outPath = Join-Path $tempDir $asset.Name
    Write-Host "`nDownloading $($asset.Name) from $($asset.Url)..."
    try {
        Invoke-WebRequest -Uri $asset.Url -OutFile $outPath -ErrorAction Stop
    }
    catch {
        Write-Error "ERROR: Failed to download $($asset.Name)!" -ErrorAction Stop
    }
    $hash = (Get-FileHash -Path $outPath -Algorithm SHA256).Hash.ToLower()
    $hashes[$asset.Name] = $hash
    Write-Host "  SHA256: $hash"
}

# --- Create new MakeSpriteFont manifest ---
Write-Host "`nCreating MakeSpriteFont $packageVersion manifests..."
Copy-Item -Path $prevMakeSpriteFont.FullName -Destination $newMakeSpriteFontDir -Recurse

foreach ($file in Get-ChildItem -Path $newMakeSpriteFontDir -Filter "*.yaml") {
    $content = Get-Content $file.FullName -Raw
    $content = $content -replace "PackageVersion:\s+\S+", "PackageVersion: $packageVersion"

    if ($file.Name -match "installer") {
        $content = $content -replace "ReleaseDate:\s+\S+", "ReleaseDate: $releaseDateYaml"
        $content = $content -replace "(InstallerUrl:\s+).+", "`${1}https://github.com/microsoft/DirectXTK/releases/download/$Tag/MakeSpriteFont.exe"
        $content = $content -replace "InstallerSha256:\s+[0-9a-fA-F]+", "InstallerSha256: $($hashes['MakeSpriteFont.exe'])"
    }

    if ($file.Name -match "locale") {
        $content = $content -replace "(ReleaseNotesUrl:\s+).+", "`${1}https://github.com/microsoft/DirectXTK/releases/tag/$Tag"
    }

    Set-Content -Path $file.FullName -Value $content -NoNewline
}

# Rename files to match new version if filenames contain version
foreach ($file in Get-ChildItem -Path $newMakeSpriteFontDir -Filter "*.yaml") {
    Write-Host "  $($file.Name)"
}

# --- Create new XWBTool manifest ---
Write-Host "`nCreating XWBTool $packageVersion manifests..."
Copy-Item -Path $prevXwbtool.FullName -Destination $newXwbtoolDir -Recurse

foreach ($file in Get-ChildItem -Path $newXwbtoolDir -Filter "*.yaml") {
    $content = Get-Content $file.FullName -Raw
    $content = $content -replace "PackageVersion:\s+\S+", "PackageVersion: $packageVersion"

    if ($file.Name -match "installer") {
        $content = $content -replace "ReleaseDate:\s+\S+", "ReleaseDate: $releaseDateYaml"

        # Update x64 installer URL and hash
        $content = $content -replace "(InstallerUrl:\s+).+XWBTool\.exe", "`${1}https://github.com/microsoft/DirectXTK/releases/download/$Tag/XWBTool.exe"
        $content = $content -replace "(InstallerUrl:\s+).+XWBTool_arm64\.exe", "`${1}https://github.com/microsoft/DirectXTK/releases/download/$Tag/XWBTool_arm64.exe"

        # Update SHA256 hashes per architecture block
        # Split into lines for per-block replacement since there are two InstallerSha256 entries
        $lines = $content -split "`n"
        $currentArch = ""
        for ($i = 0; $i -lt $lines.Count; $i++) {
            if ($lines[$i] -match "Architecture:\s+x64") {
                $currentArch = "x64"
            }
            elseif ($lines[$i] -match "Architecture:\s+arm64") {
                $currentArch = "arm64"
            }

            if ($lines[$i] -match "InstallerSha256:") {
                if ($currentArch -eq "x64") {
                    $lines[$i] = "  InstallerSha256: $($hashes['XWBTool.exe'])"
                }
                elseif ($currentArch -eq "arm64") {
                    $lines[$i] = "  InstallerSha256: $($hashes['XWBTool_arm64.exe'])"
                }
            }
        }
        $content = $lines -join "`n"
    }

    if ($file.Name -match "locale") {
        $content = $content -replace "(ReleaseNotesUrl:\s+).+", "`${1}https://github.com/microsoft/DirectXTK/releases/tag/$Tag"
    }

    Set-Content -Path $file.FullName -Value $content -NoNewline
}

foreach ($file in Get-ChildItem -Path $newXwbtoolDir -Filter "*.yaml") {
    Write-Host "  $($file.Name)"
}

# --- Cleanup ---
Remove-Item -Recurse -Force $tempDir

Write-Host "`nwinget manifests created successfully!"
Write-Host "`nNew manifest directories:"
Write-Host "  $newMakeSpriteFontDir"
Write-Host "  $newXwbtoolDir"
Write-Host "`nNext steps:"
Write-Host "  1. Review the generated manifest files"
Write-Host "  2. Validate with: winget validate $newMakeSpriteFontDir"
Write-Host "     Validate with: winget validate $newXwbtoolDir"
Write-Host "  3. Test with: winget install --manifest $newMakeSpriteFontDir"
Write-Host "     Test with: winget install --manifest $newXwbtoolDir"
Write-Host "  4. Submit PRs to the winget-pkgs repository"
Write-Host ""
Write-Host "  NOTE: Each tool must be submitted as its own separate PR per winget-pkgs policy."
