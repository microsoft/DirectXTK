<#

.SYNOPSIS
Download and extract the 'base' GDK NuGet based on edition number, returning the full version number that can be used to restore other GDK packages in the set.

.DESCRIPTION
This script determines the NuGet package id to use based on the provided GDK edition number. It makes use of MSBuild PackageReference floating version numbers to do the restore operation.

.PARAMETER GDKEditionNumber
The GDK edition number in the form of YYMMQQ.

.PARAMETER OutputDirectory
Directory to write the packages into. Path should not already contain the packages.

.PARAMETER NewLayout
Switch to indicate to use the 'new layout' of GDK packages (October 2025 and later).

.PARAMETER AutoLayout
Switch to indicate to automatically choose layout style based on edition number.

#>

param(
    [Parameter(
        Mandatory,
        Position = 0
    )]
    [string]$GDKEditionNumber,
    [Parameter(
        Mandatory,
        Position = 1
    )]
    [string]$OutputDirectory,
    [switch]$NewLayout,
    [switch]$AutoLayout
)

if ($NewLayout -and $AutoLayout) {
    Write-Error "##[error]Cannot specify both NewLayout and AutoLayout switches" -ErrorAction Stop
}

# Validate output directory
if ([string]::IsNullOrEmpty($OutputDirectory)) {
    Write-Error "##[error]Output Directory is required" -ErrorAction Stop
}

# Parse edition number
if (-not ($GDKEditionNumber -match '^([0-9][0-9])([0-9][0-9])([0-9][0-9])$')) {
    Write-Error "##[error]This script requires a valid GDK edition number!" -ErrorAction Stop
}

$year = $Matches.1
$month = [int]$Matches.2
$qfe = [int]$Matches.3

if ($year -lt 21)
{
    Write-Error "##[error]Edition year not supported: 20$year" -ErrorAction Stop
}

if (($month -lt 1) -or ($month -gt 12))
{
    Write-Error "##[error]Edition month not supported: $month" -ErrorAction Stop
}

if ($qfe -gt 0) {
    Write-Host ("##[debug]GDKEditionNumber = $GDKEditionNumber ({0} 20{1} QFE {2})" -f (Get-Culture).DateTimeFormat.GetMonthName($month), $year, $qfe)
}
else {
    Write-Host ("##[debug]GDKEditionNumber = $GDKEditionNumber ({0} 20{1})" -f (Get-Culture).DateTimeFormat.GetMonthName($month), $year)
}

# Verify NuGet tool is available
$nuget = Get-Command nuget.exe -ErrorAction SilentlyContinue
if (-Not $nuget) {
    Write-Error "##[error]Missing required nuget.exe tool" -ErrorAction Stop
}

# Determine NuGet package ID
if ($GDKEditionNumber -lt 241000) {
    Write-Error "##[error]Script supports October 2024 or later" -ErrorAction Stop
}

if ($AutoLayout) {
    if ($GDKEditionNumber -ge 251000) {
        $NewLayout = $true
    }
    else {
        $NewLayout = $false
    }
}

if ($NewLayout) {
    if ($GDKEditionNumber -lt 251000) {
        Write-Error "##[error]New layout only supported for October 2025 or later" -ErrorAction Stop
    }
    $GDK_ID = "Microsoft.GDK.Core"
}
else {
    $GDK_ID = "Microsoft.GDK.PC"
}

# Check that the package isn't already present
$GDK_DIR = [IO.Path]::Combine($OutputDirectory, $GDK_ID)
if (Test-Path $GDK_DIR) {
    Write-Error "##[error]NuGet Package ID already exists!" -ErrorAction Stop
}

# Restore Nuget packages using floating versions
$propsfile = [IO.Path]::Combine( $PSScriptRoot , "gdkedition.props")
$props = Get-Content -Path $propsfile
$props = $props -replace '<GDKEditionNumber>.+</GDKEditionNumber>', ("<GDKEditionNumber>{0}</GDKEditionNumber>" -f $GDKEditionNumber)
$props = $props -replace '<GDKNuGetPackage>.+</GDKNuGetPackage>', ("<GDKNuGetPackage>{0}</GDKNuGetPackage>" -f $GDK_ID)
Set-Content -Path $propsfile -Value $props

$nugetArgs = "restore RestoreGDK.proj -PackageSaveMode nuspec -packagesDirectory `"{0}`"" -f $OutputDirectory.TrimEnd('\')
Write-Host "##[command]nuget $nugetArgs"
$nugetrun = Start-Process -PassThru -Wait -FilePath $nuget.Path -WorkingDirectory $PSScriptRoot -ArgumentList $nugetArgs -NoNewWindow
if ($nugetrun.ExitCode -gt 0) {
    Write-Error "##[error]nuget restore failed" -ErrorAction Stop
}

# Verify expected output of restore
if (-Not (Test-Path $GDK_DIR)) {
    Write-Error "##[error]Missing NuGet package after restore!" -ErrorAction Stop
}

# Reduce path depth removing version folder
$GDK_VER = Get-ChildItem $GDK_DIR
if ($GDK_VER.Count -ne 1) {
    Write-Error "##[error]Expected a single directory for the version!" -ErrorAction Stop
}

$content = Get-ChildItem $GDK_VER.Fullname
ForEach-Object -InputObject $content { Move-Item $_.Fullname -Destination $GDK_DIR }
Remove-Item $GDK_VER.Fullname

Write-Host ("##[debug]NuGet Package ID: {0}  Version: {1}" -f $GDK_ID, $GDK_VER)

# Read the nuspec files
$GDK_NUSPEC = New-Object xml
$GDK_NUSPEC.PreserveWhitespace = $true
$GDK_NUSPEC.Load([IO.Path]::Combine($GDK_DIR, $GDK_ID + ".nuspec"))

# Log results
Write-Host "##[group]NuGet Nuget Package nuspec"
Write-host $GDK_NUSPEC.outerxml
Write-Host "##[endgroup]"

$ver = $GDK_NUSPEC.package.metadata.version
Write-Host "##vso[task.setvariable variable=GDKNuGetPackageVersion;]$ver"
