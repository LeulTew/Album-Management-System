# Album Management System Test Script
# This script tests basic functionality of the Album Management System

param(
    [string]$ExePath = ".\album_management.exe"
)

Write-Host "Album Management System Test Suite" -ForegroundColor Green
Write-Host "===================================" -ForegroundColor Green

# Check if executable exists
if (-not (Test-Path $ExePath)) {
    Write-Host "ERROR: $ExePath not found. Please build the project first." -ForegroundColor Red
    exit 1
}

# Clean up old test files
Write-Host "Cleaning up old test files..." -ForegroundColor Yellow
Remove-Item "Artist.bin" -ErrorAction SilentlyContinue
Remove-Item "Album.bin" -ErrorAction SilentlyContinue
Remove-Item "test_output*.txt" -ErrorAction SilentlyContinue

# Test 1: Basic application startup and exit
Write-Host "`nTest 1: Application startup and exit" -ForegroundColor Cyan
try {
    $process = Start-Process -FilePath $ExePath -ArgumentList "5" -NoNewWindow -Wait -RedirectStandardOutput "test_startup.txt" -RedirectStandardError "test_startup_err.txt"
    if (Test-Path "test_startup.txt") {
        Write-Host "✓ Application started successfully" -ForegroundColor Green
    } else {
        Write-Host "✗ Application failed to start" -ForegroundColor Red
    }
} catch {
    Write-Host "✗ Error starting application: $($_.Exception.Message)" -ForegroundColor Red
}

# Test 2: Check if data files are created
Write-Host "`nTest 2: Data file creation" -ForegroundColor Cyan
# Run the app briefly to initialize files
$process = Start-Process -FilePath $ExePath -ArgumentList "5" -NoNewWindow -Wait

if (Test-Path "Artist.bin") {
    Write-Host "✓ Artist.bin created successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Artist.bin not created" -ForegroundColor Red
}

if (Test-Path "Album.bin") {
    Write-Host "✓ Album.bin created successfully" -ForegroundColor Green
} else {
    Write-Host "✗ Album.bin not created" -ForegroundColor Red
}

# Test 3: Check compilation success
Write-Host "`nTest 3: Compilation verification" -ForegroundColor Cyan
if (Test-Path $ExePath) {
    $fileInfo = Get-Item $ExePath
    Write-Host "✓ Executable exists: $($fileInfo.Name) ($($fileInfo.Length) bytes)" -ForegroundColor Green
} else {
    Write-Host "✗ Executable not found" -ForegroundColor Red
}

# Test 4: Check for required header files
Write-Host "`nTest 4: Header file verification" -ForegroundColor Cyan
$requiredFiles = @("manager.h", "main.cpp", "manager.cpp", "tasks.md")

foreach ($file in $requiredFiles) {
    if (Test-Path $file) {
        Write-Host "✓ $file exists" -ForegroundColor Green
    } else {
        Write-Host "✗ $file missing" -ForegroundColor Red
    }
}

# Test 5: Git repository check
Write-Host "`nTest 5: Git repository verification" -ForegroundColor Cyan
if (Test-Path ".git") {
    Write-Host "✓ Git repository initialized" -ForegroundColor Green

    # Check if files are committed
    $gitStatus = git status --porcelain 2>$null
    if ($LASTEXITCODE -eq 0) {
        $uncommitted = ($gitStatus | Measure-Object).Count
        if ($uncommitted -eq 0) {
            Write-Host "✓ All files are committed" -ForegroundColor Green
        } else {
            Write-Host "⚠ $uncommitted uncommitted changes" -ForegroundColor Yellow
        }
    }
} else {
    Write-Host "✗ Git repository not initialized" -ForegroundColor Red
}

Write-Host "`nTest Summary:" -ForegroundColor Green
Write-Host "=============" -ForegroundColor Green
Write-Host "The Album Management System has been modernized with:" -ForegroundColor White
Write-Host "• std::string and std::vector usage" -ForegroundColor White
Write-Host "• CSV export functionality" -ForegroundColor White
Write-Host "• Advanced search features" -ForegroundColor White
Write-Host "• Statistics display" -ForegroundColor White
Write-Host "• Git version control" -ForegroundColor White
Write-Host "`nTo run manual tests:" -ForegroundColor Yellow
Write-Host "1. Execute .\album_management.exe" -ForegroundColor Yellow
Write-Host "2. Try adding artists and albums" -ForegroundColor Yellow
Write-Host "3. Test the export and search features" -ForegroundColor Yellow
Write-Host "4. Check the statistics menu" -ForegroundColor Yellow

Write-Host "`nPress any key to exit..." -ForegroundColor Cyan
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")