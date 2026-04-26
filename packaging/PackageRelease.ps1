[CmdletBinding()]
param(
    [string]$ProjectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path,
    [string]$Configuration = "RelWithDebInfo",
    [string]$BuildDir = "build-vs",
    [string]$Version,
    [switch]$IncludeSymbols
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-ProjectVersion {
    param([Parameter(Mandatory = $true)][string]$CMakeListsPath)

    $content = Get-Content -LiteralPath $CMakeListsPath -Raw
    $match = [regex]::Match($content, 'project\s*\(\s*RedFrame\s+VERSION\s+([0-9]+\.[0-9]+\.[0-9]+)', 'IgnoreCase')
    if (-not $match.Success) {
        throw "Could not read RedFrame version from '$CMakeListsPath'."
    }

    return $match.Groups[1].Value
}

$ProjectRoot = (Resolve-Path -LiteralPath $ProjectRoot).Path
if ([string]::IsNullOrWhiteSpace($Version)) {
    $Version = Get-ProjectVersion -CMakeListsPath (Join-Path $ProjectRoot "CMakeLists.txt")
}

$buildOutputDir = Join-Path $ProjectRoot (Join-Path $BuildDir $Configuration)
$dllPath = Join-Path $buildOutputDir "RedFrame.dll"
$pdbPath = Join-Path $buildOutputDir "RedFrame.pdb"
$redsPath = Join-Path $ProjectRoot "scripts\RedFrame\Capture.reds"
$readmePath = Join-Path $ProjectRoot "README.md"

foreach ($requiredPath in @($dllPath, $redsPath, $readmePath)) {
    if (-not (Test-Path -LiteralPath $requiredPath)) {
        throw "Required package input not found: $requiredPath"
    }
}

$distDir = Join-Path $ProjectRoot "dist"
$stageRoot = Join-Path $ProjectRoot "dist\stage"
$packageName = "RedFrame-$Version"
$packageStage = Join-Path $stageRoot $packageName
$zipPath = Join-Path $distDir "$packageName.zip"
$symbolsZipPath = Join-Path $distDir "$packageName-symbols.zip"

if (Test-Path -LiteralPath $stageRoot) {
    Remove-Item -LiteralPath $stageRoot -Recurse -Force
}
New-Item -ItemType Directory -Force -Path $distDir | Out-Null
New-Item -ItemType Directory -Force -Path $packageStage | Out-Null

$pluginDir = Join-Path $packageStage "red4ext\plugins\RedFrame"
$scriptDir = Join-Path $packageStage "r6\scripts\RedFrame"
New-Item -ItemType Directory -Force -Path $pluginDir, $scriptDir | Out-Null

Copy-Item -LiteralPath $dllPath -Destination (Join-Path $pluginDir "RedFrame.dll") -Force
Copy-Item -LiteralPath $redsPath -Destination (Join-Path $scriptDir "Capture.reds") -Force
Copy-Item -LiteralPath $readmePath -Destination (Join-Path $pluginDir "README.md") -Force

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
Compress-Archive -Path (Join-Path $packageStage "*") -DestinationPath $zipPath -Force

$outputs = [ordered]@{
    Package = $zipPath
}

if ($IncludeSymbols) {
    if (-not (Test-Path -LiteralPath $pdbPath)) {
        throw "Symbols requested but PDB was not found: $pdbPath"
    }

    $symbolsStage = Join-Path $stageRoot "$packageName-symbols"
    New-Item -ItemType Directory -Force -Path $symbolsStage | Out-Null
    Copy-Item -LiteralPath $pdbPath -Destination (Join-Path $symbolsStage "RedFrame.pdb") -Force
    if (Test-Path -LiteralPath $symbolsZipPath) {
        Remove-Item -LiteralPath $symbolsZipPath -Force
    }
    Compress-Archive -Path (Join-Path $symbolsStage "*") -DestinationPath $symbolsZipPath -Force
    $outputs.Symbols = $symbolsZipPath
}

Remove-Item -LiteralPath $stageRoot -Recurse -Force

$outputs.GetEnumerator() | ForEach-Object {
    Write-Host ("{0}: {1}" -f $_.Key, $_.Value)
}
