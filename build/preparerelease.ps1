<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Prepares a PR for release

.DESCRIPTION
This script is used to do the edits required for preparing a release PR.

.PARAMETER BaseBranch
This the branch to use as the base of the release. Defaults to 'main'.

.PARAMETER TargetBranch
This is the name of the newly created branch for the release PR. Defaults to '<DATETAG>release'. If set to 'none', then no branch is created.

.PARAMETER UpdateVersion
This is a $true or $false value that indicates if the library version number should be incremented. Defaults to $true.

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

param(
    [string]$BaseBranch = "main",
    [string]$TargetBranch = $null,
    [bool]$UpdateVersion = $true
)

$reporoot = Split-Path -Path $PSScriptRoot -Parent
$cmake = $reporoot + "\CMakeLists.txt"
$readme = $reporoot + "\README.md"
$history = $reporoot + "\CHANGELOG.md"

if ((-Not (Test-Path $cmake)) -Or (-Not (Test-Path $readme)) -Or (-Not (Test-Path $history))) {
    Write-Error "ERROR: Unexpected location of script file!" -ErrorAction Stop
}

$branch = git branch --show-current
if ($branch -ne $BaseBranch) {
    Write-Error "ERROR: Must be in the $BaseBranch branch!" -ErrorAction Stop
}

git pull -q
if ($LastExitCode -ne 0) {
    Write-Error "ERROR: Failed to sync branch!" -ErrorAction Stop
}

$version = Get-Content ($cmake) | Select-String -Pattern "set\(DIRECTXTK_VERSION" -CaseSensitive
if (-Not ($version -match "([0-9]?\.[0-9]?\.[0-9]?)")) {
    Write-Error "ERROR: Failed to current version!" -ErrorAction Stop
}
$version = $Matches.0
$rawversion = $version.replace('.','')

$newreleasedate = Get-Date -Format "MMMM d, yyyy"
$newreleasetag = (Get-Date -Format "MMMyyyy").ToLower()

if($UpdateVersion) {
    [string]$newrawversion = ([int]$rawversion + 1)
}
else {
    $newrawversion = $rawversion
}

$newversion = $newrawversion[0] + "." + $newrawversion[1] + "." + $newrawversion[2]

$rawreleasedate = $(Get-Content $readme) | Select-String -Pattern "\#\#\s.[A-Z][a-z]+\S.\d+,?\S.\d\d\d\d"
if ([string]::IsNullOrEmpty($rawreleasedate)) {
    Write-Error "ERROR: Failed to current release date!" -ErrorAction Stop
}
$releasedate = $rawreleasedate -replace '## ',''

if($releasedate -eq $newreleasedate) {
    Write-Error ("ERROR: Release "+$releasedate+" already exists!") -ErrorAction Stop
}

if ($TargetBranch -ne 'none') {
    if ([string]::IsNullOrEmpty($TargetBranch)) {
        $TargetBranch = $newreleasetag + "release"
    }

    git checkout -b $TargetBranch
    if ($LastExitCode -ne 0) {
        Write-Error "ERROR: Failed to create new topic branch!" -ErrorAction Stop
    }
}

Write-Host "     Old Version: " $version
Write-Host "Old Release Date: " $releasedate
Write-Host "->"
Write-Host "    Release Date: " $newreleasedate
Write-Host "     Release Tag: " $newreleasetag
Write-Host " Release Version: " $newversion

if($UpdateVersion) {
    (Get-Content $cmake).Replace("set(DIRECTXTK_VERSION $version)","set(DIRECTXTK_VERSION $newversion)") | Set-Content $cmake
}

(Get-Content $readme).Replace("$rawreleasedate", "# $newreleasedate") | Set-Content $readme

Get-ChildItem -Path ($reporoot + "\.nuget") -Filter *.nuspec | Foreach-Object {
    (Get-Content -Path $_.Fullname).Replace("$releasedate", "$newreleasedate") | Set-Content -Path $_.Fullname -Encoding utf8
    }

[System.Collections.ArrayList]$file = Get-Content $history
$inserthere = @()

for ($i=0; $i -lt $file.count; $i++) {
    if ($file[$i] -match "## Release History") {
        $inserthere += $i + 1
    }
}

$file.insert($inserthere[0], "`n### $newreleasedate`n* change history here")
Set-Content -Path $history -Value $file

code $history $readme
if ($LastExitCode -ne 0) {
    Write-Error "ERROR: Failed to launch VS Code!" -ErrorAction Stop
}
