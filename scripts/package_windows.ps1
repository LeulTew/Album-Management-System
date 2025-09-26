param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [string]$Generator = "MinGW Makefiles",
    [string]$CMakePath = "cmake"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$versionFile = Join-Path $repoRoot "VERSION"
if (!(Test-Path $versionFile)) {
    throw "VERSION file not found."
}
$version = (Get-Content $versionFile | Select-Object -First 1).Trim()
if ([string]::IsNullOrWhiteSpace($version)) {
    throw "VERSION file is empty."
}

$buildDir = Join-Path $repoRoot "build\windows-$Configuration"
$distRoot = Join-Path $repoRoot "dist"
$packageName = "AlbumManagementSystem-$version-windows-$Configuration"
$staging = Join-Path $distRoot $packageName
$archive = Join-Path $distRoot "$packageName.zip"

Write-Host "Building Album Management System $version ($Configuration)..." -ForegroundColor Cyan

if (!(Test-Path $buildDir)) {
    New-Item -ItemType Directory -Path $buildDir | Out-Null
}

& $CMakePath -S $repoRoot -B $buildDir -G $Generator -DCMAKE_BUILD_TYPE=$Configuration
& $CMakePath --build $buildDir --config $Configuration

if (Test-Path $staging) {
    Remove-Item -Recurse -Force $staging
}

New-Item -ItemType Directory -Path $staging | Out-Null

$binaryName = if ($Configuration -eq "Release") { "album_management.exe" } else { "album_management.exe" }
$binarySource = Join-Path $buildDir $binaryName
if (!(Test-Path $binarySource)) {
    $binarySource = Join-Path $buildDir "Release\album_management.exe"
}
if (!(Test-Path $binarySource)) {
    throw "Failed to locate built executable at $binarySource"
}

Copy-Item $binarySource -Destination (Join-Path $staging "album_management.exe")
Copy-Item (Join-Path $repoRoot "config.json") -Destination $staging -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "README.md") -Destination $staging
Copy-Item (Join-Path $repoRoot "LICENSE") -Destination $staging
Copy-Item (Join-Path $repoRoot "docs") -Destination $staging -Recurse
Copy-Item (Join-Path $repoRoot "tasks.md") -Destination $staging
Copy-Item (Join-Path $repoRoot "CHANGELOG.md") -Destination $staging -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "RELEASE.md") -Destination $staging -ErrorAction SilentlyContinue
Copy-Item (Join-Path $repoRoot "VERSION") -Destination $staging
Copy-Item (Join-Path $repoRoot "version.h") -Destination $staging

if (!(Test-Path $distRoot)) {
    New-Item -ItemType Directory -Path $distRoot | Out-Null
}

if (Test-Path $archive) {
    Remove-Item $archive
}

Compress-Archive -Path (Join-Path $staging '*') -DestinationPath $archive

Write-Host "Created package: $archive" -ForegroundColor Green
Write-Host "Staging directory: $staging" -ForegroundColor Green
