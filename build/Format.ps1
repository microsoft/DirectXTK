[CmdletBinding()]
param(
    [switch] $Check
)

$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$configFile = Join-Path $repoRoot '.github\linters\.clang-format'

if (-not (Test-Path -LiteralPath $configFile -PathType Leaf)) {
    throw "clang-format configuration was not found: $configFile"
}

$clangFormat = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    throw 'clang-format was not found on PATH.'
}

$sourceFiles = Get-ChildItem -LiteralPath $repoRoot -Recurse|
    Where-Object {
        $_.FullName -notmatch '\\(?:\.git|\.vs|build|Tests|vcpkg_installed)\\' -and $_.Extension -in '.c','.cc','.cpp','.cxx','.h','.hh','.hpp'
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
    & $clangFormat.Source @arguments $sourceFile.FullName
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
