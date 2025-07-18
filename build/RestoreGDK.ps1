<#

.SYNOPSIS
Download and extract GDK NuGet based on edition number

.DESCRIPTION
This script determines the NuGet package id to use based on the provided GDK edition number. It makes use of MSBuild PackageReference floating version numbers to do the restore operation.

.PARAMETER GDKEditionNumber
The GDK edition number in the form of YYMMQQ.

.PARAMETER OutputDirectory
Directory to write the packages into. Path should not already contain the packages.

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
    [string]$OutputDirectory
)

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
if ($GDKEditionNumber -ge 241000) {
    $PGDK_ID = "Microsoft.GDK.PC"
}
else {
    Write-Error "##[error]Script supports October 2024 or later" -ErrorAction Stop
}

# Check that the package isn't already present
$PGDK_DIR = [IO.Path]::Combine($OutputDirectory, $PGDK_ID)
if (Test-Path $PGDK_DIR) {
    Write-Error "##[error]PC Package ID already exists!" -ErrorAction Stop
}

# Restore Nuget packages using floating versions
$propsfile = [IO.Path]::Combine( $PSScriptRoot , "gdkedition.props")
$props = Get-Content -Path $propsfile
$props = $props -replace '<GDKEditionNumber>.+</GDKEditionNumber>', ("<GDKEditionNumber>{0}</GDKEditionNumber>" -f $GDKEditionNumber)
Set-Content -Path $propsfile -Value $props

$nugetArgs = "restore RestoreGDK.proj -PackageSaveMode nuspec -packagesDirectory `"{0}`"" -f $OutputDirectory.TrimEnd('\')
Write-Host "##[command]nuget $nugetArgs"
$nugetrun = Start-Process -PassThru -Wait -FilePath $nuget.Path -WorkingDirectory $PSScriptRoot -ArgumentList $nugetArgs -NoNewWindow
if ($nugetrun.ExitCode -gt 0) {
    Write-Error "##[error]nuget restore failed" -ErrorAction Stop
}

# Verify expected output of restore
if (-Not (Test-Path $PGDK_DIR)) {
    Write-Error "##[error]Missing PC package after restore!" -ErrorAction Stop
}

# Reduce path depth removing version folder
$PGDK_VER = Get-ChildItem $PGDK_DIR
if ($PGDK_VER.Count -ne 1) {
    Write-Error "##[error]Expected a single directory for the version!" -ErrorAction Stop
}

$content = Get-ChildItem $PGDK_VER.Fullname
ForEach-Object -InputObject $content { Move-Item $_.Fullname -Destination $PGDK_DIR }
Remove-Item $PGDK_VER.Fullname

Write-Host ("##[debug]PC Package ID: {0}  Version: {1}" -f $PGDK_ID, $PGDK_VER)


# Read the nuspec files
$PGDK_NUSPEC = New-Object xml
$PGDK_NUSPEC.PreserveWhitespace = $true
$PGDK_NUSPEC.Load([IO.Path]::Combine($PGDK_DIR, $PGDK_ID + ".nuspec"))

# Log results
Write-Host "##[group]PC Nuget Package nuspec"
Write-host $PGDK_NUSPEC.outerxml
Write-Host "##[endgroup]"

$id = $PGDK_NUSPEC.package.metadata.id
Write-Host "##vso[task.setvariable variable=PCNuGetPackage;]$id"

$ver = $PGDK_NUSPEC.package.metadata.version
Write-Host "##vso[task.setvariable variable=PCNuGetPackageVersion;]$ver"
