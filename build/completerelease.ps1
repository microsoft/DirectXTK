<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Completes the release process by creating verified GitHub tags and a release.

.DESCRIPTION
Creates GPG-signed annotated tags on both the DirectXTK repository and the
DirectXTK test suite repository (Tests/), then publishes a GitHub release on
the DirectXTK repository using the signed tag.

Tags are signed locally with 'git tag -s', which requires GPG signing to be
configured in git and the signing key to be registered with GitHub. This
produces the Verified badge in the GitHub UI.

Run this script after the release PR (prepared by preparerelease.ps1) has been
merged into the main branch.

.PARAMETER PAT
GitHub Personal Access Token with 'repo' scope, used to publish the GitHub
release on microsoft/DirectXTK. Can also be provided via the GITHUB_TOKEN
environment variable. If neither is provided, the script attempts to obtain a
token from the 'gh' CLI.

.PARAMETER SkipTestRepo
If set, skips creating a tag on the test suite repository (Tests/).

.PARAMETER WhatIf
Shows what would happen without creating tags, pushing, or publishing a release.

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

[Diagnostics.CodeAnalysis.SuppressMessageAttribute('PSAvoidUsingEmptyCatchBlock', '')]
param(
    [string]$PAT = "",
    [switch]$SkipTestRepo,
    [switch]$WhatIf
)

$reporoot  = Split-Path -Path $PSScriptRoot -Parent
$readme    = Join-Path $reporoot "README.md"
$history   = Join-Path $reporoot "CHANGELOG.md"
$testsroot = Join-Path $reporoot "Tests"

#--- Validate script location ---

if ((-Not (Test-Path $readme)) -Or (-Not (Test-Path $history))) {
    Write-Error "ERROR: Unexpected location of script file!" -ErrorAction Stop
}

#--- Validate local repo state ---

$branch = git -C $reporoot branch --show-current
if ($branch -ne "main") {
    Write-Error "ERROR: Must be on the 'main' branch (currently on '$branch')!" -ErrorAction Stop
}

Write-Host "Fetching from origin..."
git -C $reporoot fetch -q origin
if ($LastExitCode -ne 0) {
    Write-Error "ERROR: Failed to fetch from origin!" -ErrorAction Stop
}

$headHash   = git -C $reporoot rev-parse HEAD
$remoteHash = git -C $reporoot rev-parse "origin/main"
if ($headHash -ne $remoteHash) {
    Write-Error "ERROR: Local 'main' is not in sync with origin. Run 'git pull' first." -ErrorAction Stop
}

#--- Derive release info from README.md ---

$rawreleasedate = $(Get-Content $readme) | Select-String -Pattern "^## [A-Z][a-z]+ \d+,?\s*\d{4}" | Select-Object -First 1
if ([string]::IsNullOrEmpty($rawreleasedate)) {
    Write-Error "ERROR: Failed to find a release date header in README.md!" -ErrorAction Stop
}

$releasename = ($rawreleasedate.ToString() -replace '^## ', '').Trim()

try {
    $releaseDateTime = [datetime]::Parse($releasename)
}
catch {
    Write-Error "ERROR: Failed to parse release date '$releasename': $_" -ErrorAction Stop
}

$releasetag = (Get-Date -Date $releaseDateTime -Format "MMMyyyy").ToLower()

Write-Host "  Release Name: $releasename"
Write-Host "   Release Tag: $releasetag"

#--- Extract release notes from CHANGELOG.md ---

$changelog  = Get-Content $history
$notesStart = -1
$notesEnd   = $changelog.Count - 1

for ($i = 0; $i -lt $changelog.Count; $i++) {
    if ($changelog[$i] -match "^### $([regex]::Escape($releasename))") {
        $notesStart = $i + 1
    }
    elseif ($notesStart -ge 0 -and $changelog[$i] -match "^### ") {
        $notesEnd = $i - 1
        break
    }
}

if ($notesStart -lt 0) {
    Write-Error "ERROR: Could not find release notes for '$releasename' in CHANGELOG.md!" -ErrorAction Stop
}

$releaseNotes = (($changelog[$notesStart..$notesEnd] | Where-Object { $_ -ne "" }) -join "`n").Trim()

Write-Host "Release Notes:"
Write-Host $releaseNotes
Write-Host ""

#--- Acquire GitHub token ---

if ($PAT.Length -eq 0) {
    $PAT = [string]$env:GITHUB_TOKEN
}

if ($PAT.Length -eq 0) {
    try {
        $ghToken = & gh auth token 2>$null
        if ($LASTEXITCODE -eq 0 -and -not [string]::IsNullOrWhiteSpace($ghToken)) {
            $PAT = $ghToken.Trim()
            Write-Host "Using token from 'gh' CLI."
        }
    }
    catch {
        # gh CLI not available
    }
}

if ($PAT.Length -eq 0) {
    Write-Error "ERROR: No GitHub token found. Provide -PAT, set GITHUB_TOKEN, or sign in with 'gh auth login'." -ErrorAction Stop
}

$apiHeaders = @{
    "Accept"               = "application/vnd.github+json"
    "Authorization"        = "Bearer $PAT"
    "X-GitHub-Api-Version" = "2022-11-28"
}

#--- Helper: create a GPG-signed tag locally and push it ---

function Push-SignedTag {
    param(
        [Parameter(Mandatory)] [string]$RepoPath,
        [Parameter(Mandatory)] [string]$TagName,
        [Parameter(Mandatory)] [string]$Message
    )

    # Check whether the tag already exists locally
    $existing = git -C $RepoPath tag -l $TagName
    if (-not [string]::IsNullOrEmpty($existing)) {
        Write-Error "ERROR: Tag '$TagName' already exists in '$RepoPath'!" -ErrorAction Stop
    }

    if ($WhatIf) {
        Write-Host "[WhatIf] Would create signed tag '$TagName' in '$RepoPath' and push to origin"
        return
    }

    Write-Host "Creating signed tag '$TagName'..."
    git -C $RepoPath tag -s $TagName -m $Message
    if ($LastExitCode -ne 0) {
        Write-Error "ERROR: Failed to create signed tag '$TagName'. Ensure GPG signing is configured." -ErrorAction Stop
    }

    Write-Host "Pushing tag '$TagName' to origin..."
    git -C $RepoPath push origin $TagName
    if ($LastExitCode -ne 0) {
        git -C $RepoPath tag -d $TagName 2>$null
        Write-Error "ERROR: Failed to push tag '$TagName' to origin." -ErrorAction Stop
    }
}

#--- Helper: create a GitHub release ---

function New-GitHubRelease {
    param(
        [Parameter(Mandatory)] [string]$Owner,
        [Parameter(Mandatory)] [string]$Repo,
        [Parameter(Mandatory)] [string]$TagName,
        [Parameter(Mandatory)] [string]$ReleaseName,
        [Parameter(Mandatory)] [string]$ReleaseBody
    )

    # Check whether a release already exists for this tag
    $checkUri = "https://api.github.com/repos/$Owner/$Repo/releases/tags/$TagName"
    $releaseExists = $false

    try {
        $null = Invoke-RestMethod -Uri $checkUri -Method Get -Headers $apiHeaders -ErrorAction Stop
        $releaseExists = $true
    }
    catch {
        $sc = $null
        try { $sc = [int]$_.Exception.Response.StatusCode } catch { }
        if ($sc -ne 404) {
            Write-Error "ERROR: Failed to check for existing release '$TagName' on ${Owner}/${Repo}: $_" -ErrorAction Stop
        }
        # 404 = no release exists yet, which is expected
    }

    if ($releaseExists) {
        Write-Error "ERROR: Release '$TagName' already exists on ${Owner}/${Repo}!" -ErrorAction Stop
    }

    if ($WhatIf) {
        Write-Host "[WhatIf] Would create release '$TagName' on ${Owner}/${Repo}"
        return
    }

    $payload = @{
        tag_name    = $TagName
        name        = $ReleaseName
        body        = $ReleaseBody
        draft       = $false
        prerelease  = $false
        make_latest = "true"
    } | ConvertTo-Json

    Write-Host "Creating release '$TagName' on ${Owner}/${Repo}..."

    try {
        $result = Invoke-RestMethod -Uri "https://api.github.com/repos/$Owner/$Repo/releases" `
            -Method Post -Headers $apiHeaders -Body $payload -ContentType "application/json" -ErrorAction Stop
        Write-Host "  Created: $($result.html_url)"
    }
    catch {
        Write-Error "ERROR: Failed to create release '$TagName' on ${Owner}/${Repo}: $_" -ErrorAction Stop
    }
}

#--- Create verified tag and release on microsoft/DirectXTK ---

Push-SignedTag -RepoPath $reporoot -TagName $releasetag -Message $releasename

New-GitHubRelease `
    -Owner         "microsoft" `
    -Repo          "DirectXTK" `
    -TagName       $releasetag `
    -ReleaseName   $releasename `
    -ReleaseBody   $releaseNotes

#--- Create verified tag on walbourn/directxtktest ---

if (-Not $SkipTestRepo) {
    if (-Not (Test-Path $testsroot)) {
        Write-Warning "WARNING: Tests/ folder not found at '$testsroot'. Skipping test suite tag."
    }
    else {
        Push-SignedTag -RepoPath $testsroot -TagName $releasetag -Message $releasename
    }
}

#--- Done ---

if (-Not $WhatIf) {
    Write-Host ""
    Write-Host "Release complete. Sync the new tags locally with:"
    Write-Host "  git pull --tags"
    if (-Not $SkipTestRepo) {
        Write-Host "  git -C Tests pull --tags"
    }
}