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

# This has to match the version used by GitHub super-linter or results will not match.
$requiredClangFormatVersion = '21.1.2'
$clangFormatVersion = (& $clangFormat.Source '--version' 2>&1 | Out-String).Trim()
if ($LASTEXITCODE -ne 0) {
    throw "Unable to determine clang-format version."
}

if ($clangFormatVersion -notmatch "\b$([regex]::Escape($requiredClangFormatVersion))\b") {
    throw "clang-format version $requiredClangFormatVersion is required; found: $clangFormatVersion"
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
