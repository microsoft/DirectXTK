<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Runs clang-format on the source code.

.DESCRIPTION
This is use to clang-format the code using the rules in .github\linters\.clang-format.

The clang-format version must match the version used by GitHub Super-Linter or false positives can be found.
You can install this version locally using:

winget install --id=LLVM.LLVM --version 21.1.2

.PARAMETER Check
Runs clang-format without modifying the files in place.

.PARAMETER LLVM
Normally clang-format is found on the path. If you use this switch, then it looks for it in C:\Program Files\LLVM\bin.

.LINKS
https://github.com/microsoft/DirectXTK

#>

[CmdletBinding()]
param(
    [switch] $Check,
    [Alias('UseLLVM')]
    [switch] $LLVM
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$configFile = Join-Path $repoRoot '.github\linters\.clang-format'

if (-not (Test-Path -LiteralPath $configFile -PathType Leaf)) {
    throw "clang-format configuration was not found: $configFile"
}

$clangFormatPath = if ($LLVM) {
    'C:\Program Files\LLVM\bin\clang-format.exe'
}
else {
    (Get-Command clang-format -ErrorAction SilentlyContinue).Source
}

if (-not $clangFormatPath -or -not (Test-Path -LiteralPath $clangFormatPath -PathType Leaf)) {
    if ($LLVM) {
        throw "clang-format was not found at: $clangFormatPath"
    }

    throw 'clang-format was not found on PATH.'
}

# This has to match the version used by GitHub super-linter or results will not match.
$requiredClangFormatVersion = '21.1.2'
$clangFormatVersion = (& $clangFormatPath '--version' 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0) {
    throw "Unable to determine clang-format version."
}

if ($clangFormatVersion -notmatch "\b$([regex]::Escape($requiredClangFormatVersion))\b") {
    throw "clang-format version $requiredClangFormatVersion is required; found: $clangFormatVersion"
}

$sourceFiles = Get-ChildItem -LiteralPath $repoRoot -Recurse|
    Where-Object {
        $_.FullName -notmatch '\\(?:\.git|\.vs|build|Tests|vcpkg_installed)\\' -and $_.Extension -in '.c','.cc','.cpp','.cxx','.h','.hh','.hpp','.inl'
    }

if ($sourceFiles.Count -eq 0) {
    Write-Output 'No C/C++ source files found.'
    exit 0
}

$arguments = @(
    '--style=file:{0}' -f $configFile
)

if ($Check) {
    $arguments += '--dry-run'
    $arguments += '--Werror'
}
else {
    $arguments += '-i'
}

foreach ($sourceFile in $sourceFiles) {
    & $clangFormatPath @arguments $sourceFile.FullName
    if ($LASTEXITCODE -ne 0) {
        throw "clang-format failed for $($sourceFile.FullName)."
    }
}

if ($Check) {
    Write-Output "Checked $($sourceFiles.Count) C/C++ source files."
}
else {
    Write-Output "Formatted $($sourceFiles.Count) C/C++ source files."
}
