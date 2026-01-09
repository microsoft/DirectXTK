<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Downloads build artifacts from Azure DevOps for DirectXTK.

.DESCRIPTION
This script is used as part of the internal release process for DirectXTK.

.PARAMETER BuildId
This is the specific build to get artifacts from.

.PARAMETER PAT
Requires an ADO PAT with 'Build > Read' scope. Can be provided via the ADO_PERSONAL_ACCESS_TOKEN environment variable or as a parameter.

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

param(
    [Parameter(Mandatory)]
    [int]$BuildId,
    [string]$PAT = ""
)

# Parse PAT
if ($PAT.Length -eq 0) {
    $PAT = $env:ADO_PERSONAL_ACCESS_TOKEN

    if ($PAT.Length -eq 0) {
        Write-Error "##[error]This script requires a valid ADO Personal Access Token!" -ErrorAction Stop
    }
}

# Initial REST query
$headers = @{
    "Content-Type" = "application/json"
    Authorization = "Basic " + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(":$PAT"))
}

$uriFormat = "https://dev.azure.com/MSCodeHub/ab27a052-7f0e-4cba-9bec-d298c5942ab9/_apis/build/builds/{0}/artifacts?artifactName={1}&api-version=7.1"

$uriarm64 = $uriFormat -f $BuildId, "DirectXTK_Binaries_Release_ARM64"
$uriamd64 = $uriFormat -f $BuildId, "DirectXTK_Binaries_Release_x64"

try
{
    Write-Host "Checking if build and artifacts exist..."
    $responseamd64 = Invoke-RestMethod -Uri $uriamd64 -Method Get -Headers $headers
    $responsearm64 = Invoke-RestMethod -Uri $uriarm64 -Method Get -Headers $headers
}
catch
{
    Write-Error "##[error]Build $BuildId not found!" -ErrorAction Continue
}

$ProgressPreference = 'SilentlyContinue'

$tempFolderPath = Join-Path $Env:Temp $(New-Guid)
New-Item -Type Directory -Path $tempFolderPath | Out-Null

Write-Host $tempFolderPath

$headers = @{
    Authorization = "Basic " + [Convert]::ToBase64String([Text.Encoding]::ASCII.GetBytes(":$PAT"))
    Accept = "application/zip"
}

Add-Type -A System.IO.Compression.FileSystem

# Download artifacts for ARM64
foreach ($artifact in $responsearm64) {
    $artifactName = $artifact.name
    $downloadUrl = $artifact.resource.downloadUrl
    $outputFile = Join-Path $tempFolderPath "$artifactName.zip"

    try
    {
        Write-Host "Downloading $artifactName to $outputFile..."
        Invoke-WebRequest -Uri $downloadUrl -Headers $headers -OutFile $outputFile
    }
    catch
    {
        Write-Error "##[error]Failed to download $artifactName!" -ErrorAction Continue
    }

    try
    {
        Write-Host "Extracting $artifactName..."
        [IO.Compression.ZipFile]::ExtractToDirectory($outputFile, $tempFolderPath)
    }
    catch
    {
        Write-Error "##[error]Failed to extract $artifactName!" -ErrorAction Continue
    }
}

# Download artifacts for X64
foreach ($artifact in $responseamd64) {
    $artifactName = $artifact.name
    $downloadUrl = $artifact.resource.downloadUrl
    $outputFile = Join-Path $tempFolderPath "$artifactName.zip"

    try
    {
        Write-Host "Downloading $artifactName to $outputFile..."
        Invoke-WebRequest -Uri $downloadUrl -Headers $headers -OutFile $outputFile
    }
    catch
    {
        Write-Error "##[error]Failed to download $artifactName!" -ErrorAction Continue
    }

    try
    {
        Write-Host "Extracting $artifactName..."
        [IO.Compression.ZipFile]::ExtractToDirectory($outputFile, $tempFolderPath)
    }
    catch
    {
        Write-Error "##[error]Failed to extract $artifactName!" -ErrorAction Continue
    }
}

# Extract command-line tool binary
$exe = "XWBTool"

$binPath = Join-Path $tempFolderPath "DirectXTK_Binaries_Release_ARM64"
$srcPath = "{0}\{1}\Bin\Desktop_2022\ARM64\Release\{1}_arm64.exe" -f $binPath, $exe
Copy-Item -Path $srcPath -Destination "." -ErrorAction Stop

$binPath = Join-Path $tempFolderPath "DirectXTK_Binaries_Release_x64"
$srcPath = "{0}\{1}\Bin\Desktop_2022\x64\Release\{1}.exe" -f $binPath, $exe
Copy-Item -Path $srcPath -Destination "." -ErrorAction Stop

$exe = "MakeSpriteFont"
$srcPath = "{0}\{1}\Bin\Release\{1}.exe" -f $binPath, $exe
Copy-Item -Path $srcPath -Destination "." -ErrorAction Stop
