<#

.NOTES
Copyright (c) Microsoft Corporation.
Licensed under the MIT License.

.SYNOPSIS
Copies all test assets and required files to a directory.

.DESCRIPTION
This copies all the test binaries and assets to a directory, typically a file share.

A few tests are skipped as they are only intended to be run by ctest, and all Xbox content is ignored as well.

.PARAMETER Destination
Location for all test content to be copied into.

.PARAMETER Platform
Which platform to deploy (defaults to x64)

.PARAMETER Configuration
Which configuration to deploy (defaults to Debug)

.PARAMETER DebugCRT
If set, copies the DebugCRT into the directory

.LINK
https://github.com/microsoft/DirectXTK/wiki

#>

param(
    [Parameter(Mandatory)]
    [string]$Destination,
    [string]$Platform="x64",
    [string]$Configuration="Debug",
    [switch]$DebugCRT,
    [switch]$Clean
)

if (-Not (Test-Path $Destination)) {
    Write-Error "ERROR: -Destination folder does not exist" -ErrorAction Stop
}

$destdir = Join-Path $Destination -ChildPath "Tests"

if($Clean) {
    Write-Host "Clean..."
    Remove-Item $destdir -Recurse -force -ErrorAction SilentlyContinue | Out-Null
}

New-Item -Path $Destination -Name "Tests" -ItemType Directory -ErrorAction SilentlyContinue | Out-Null

$testFolder = Split-Path -Path $PSScriptRoot -Parent

$outFolder = Split-Path -Path $testFolder -Parent
$outFolder = Join-Path -Path $outFolder -ChildPath out

if (-Not (Test-Path $outFolder)) {
    Write-Error "ERROR: Must build test using cmake first" -ErrorAction Stop
}

$outFolder = Join-Path -Path $outFolder -ChildPath "build"
$outFolder = Join-Path -Path $outFolder -ChildPath ($Platform + "-" + $Configuration)

if (-Not (Test-Path $outFolder)) {
    Write-Error "ERROR: Failed to find platform-configuration combination" -ErrorAction Stop
}

$binFolder = Join-Path -Path $outFolder -ChildPath bin

if (-Not (Test-Path $outFolder)) {
    Write-Error "ERROR: Must build tests using cmake --build first" -ErrorAction Stop
}

$exes = Get-ChildItem -Path $binFolder -Filter "*.exe"
$pdbs = Get-ChildItem -Path $binFolder -Filter "*.pdb"

if ($exes.Count -eq 0) {
    Write-Error "ERROR: No test executables found." -ErrorAction Stop
}

if ($DebugCRT) {

    $vcInstallDir = $null

    $vsEditions = @( "Community", "Professional", "Enterprise", "Preview", "BuildTools" )

    foreach($edition in $vsEditions) {
        $testDir = Join-Path -Path $ENV:ProgramFiles -ChildPath "Microsoft Visual Studio"
        $testDir = Join-Path -Path $testDir -ChildPath "2022"
        $testDir = Join-Path -Path $testDir -ChildPath $edition
        $testDir = Join-Path -Path $testDir -ChildPath "VC"

        if (Test-Path $testDir) {
            $vcInstallDir = $testDir
            break;
        }
    }

    if ($vcInstallDir -eq $null) {
        Write-Error "ERROR: Failed to find Visual C++ installation" -ErrorAction Stop
    }

    $redistDir = Join-Path $vcInstallDir -ChildPath "Redist"
    $redistDir = Join-Path $redistDir -ChildPath "MSVC"
    $versions = Get-ChildItem $redistDir -Filter "14.*"

    if ($versions.Count -eq 0) {
        Write-Error "ERROR: Failed to find redist files in Visual C++ installation" -ErrorAction Stop
    }

    $versions = $versions | Sort-Object -Descending

    $crtDir = Join-Path $redistDir -ChildPath $versions[0]
    $crtDir = Join-Path $crtDir -ChildPath "debug_nonredist"
    $crtDir = Join-Path $crtDir -ChildPath $Platform
    $crtDir = Join-Path $crtDir -ChildPath "Microsoft.VC143.DebugCRT"

    if (-Not (Test-Path $crtDir)) {
        Write-Error "ERROR: Failed to find CRT files in Visual C++ installation" -ErrorAction Stop
    }

    Write-Host "Copying debug CRT..."
    $files = Get-ChildItem $crtDir -Filter "*.dll"
    $files | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $destdir
    }

    $wsdkBinDir = Join-Path -Path ${ENV:ProgramFiles(x86)} -ChildPath "Windows Kits"
    $wsdkBinDir = Join-Path -Path $wsdkBinDir -ChildPath "10"
    $wsdkBinDir = Join-Path -Path $wsdkBinDir -ChildPath "bin"

    if (-Not (Test-Path $wsdkBinDir)) {
        Write-Error "ERROR: Failed to find Windows SDK bin folder" -ErrorAction Stop
    }

    $versions = Get-ChildItem $wsdkBinDir -Filter "10.*"

    if ($versions.Count -eq 0) {
        Write-Error "ERROR: Failed to find redist files in Visual C++ installation" -ErrorAction Stop
    }

    $versions = $versions | Sort-Object -Descending

    $ucrtDir = Join-Path $wsdkBinDir -ChildPath $versions[0]
    $ucrtDir = Join-Path $ucrtDir -ChildPath $Platform
    $ucrtDir = Join-Path $ucrtDir -ChildPath "ucrt"

    if (-Not (Test-Path $ucrtDir)) {
        Write-Error "ERROR: Failed to find UCRT folder in Windows SDK installation" -ErrorAction Stop
    }

    Write-Host "Copying debug Unified CRT..."
    $files = Get-ChildItem $ucrtDir -Filter "*.dll"
    $files | ForEach-Object {
        Copy-Item -Path $_.FullName -Destination $destdir
    }
}

Write-Host "Copying binaries..."
$ignoretest = @( "apitest", "ddswictest", "fuzzloaders", "headertest", "wavtest" )
$exes | ForEach-Object {
    if (-Not ($ignoretest -contains $_.BaseName)) {
        Copy-Item -Path $_.FullName -Destination $destdir
    }
}

$pdbs | ForEach-Object {
    if (-Not ($ignoretest -contains $_.BaseName)) {
        Copy-Item -Path $_.FullName -Destination $destdir
    }
}

Write-Host "Copying media..."
$filters = @("*.spritefont", "*.bmp", "*.dds", "*.png", "*.jpg", "*.tif", "*.tiff",
    "*.cso",
    "*.sdkmesh*", "*.cmo", "*._obj", "*.mtl", "*.vbo",
    "*.wav", "*.xwb" )

$filters | ForEach-Object {
    $files = Get-ChildItem -Path $testFolder -Recurse -Filter $_
    $files | ForEach-Object {
        if ($_.Directory -notlike "*XboxLoadTest") {
            Copy-Item -Path $_.FullName -Destination $destdir
        }
    }
}
