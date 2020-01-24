param(
[string]$version
)
$versionComma = $version.Replace(".", ",")
$files = 'XWBTool\xwbtool.rc', 'MakeSpriteFont\Properties\AssemblyInfo.cs'
foreach ($file in $files) { (Get-Content $file).replace('1,0,0,0', $versionComma).replace('1.0.0.0', $version) | Set-Content $file }
