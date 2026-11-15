param(
    [ValidateSet('Debug', 'Release', 'All')]
    [string]$Configuration = 'Debug',
    [switch]$Rebuild
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (!(Test-Path -LiteralPath $vswhere)) {
    throw 'Install Visual Studio 2026 with Desktop development with C++.'
}
$msbuild = & $vswhere -latest -products * -requires Microsoft.Component.MSBuild -find 'MSBuild\Current\Bin\MSBuild.exe' | Select-Object -First 1
if (!$msbuild) {
    throw 'MSBuild was not found.'
}
$configurations = if ($Configuration -eq 'All') { @('Debug', 'Release') } else { @($Configuration) }
$target = if ($Rebuild) { 'Rebuild' } else { 'Build' }
foreach ($name in $configurations) {
    & $msbuild (Join-Path $root 'JamEngine.sln') /m:4 "/t:$target" "/p:Configuration=$name" /p:Platform=x64 /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
