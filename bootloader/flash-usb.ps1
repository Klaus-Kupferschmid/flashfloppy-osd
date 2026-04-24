# USB HID Flash Script for FlashFloppy OSD
# Usage: Start bootloader on device (hold PA4 for 4s), then run this script
# Run from: bootloader/ folder

$ErrorActionPreference = "Stop"

# Get script directory and project root
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectRoot = Split-Path -Parent $scriptDir

Write-Host "=== FlashFloppy OSD USB Flash ===" -ForegroundColor Cyan
Write-Host ""

$binFile = Join-Path $projectRoot "src\FF_OSD_hid4k.bin"
$flashTool = Join-Path $scriptDir "tools\hid_flash\hid-flash.exe"

if (!(Test-Path $binFile)) {
    Write-Host "ERROR: $binFile not found!" -ForegroundColor Red
    Write-Host "Run 'make hid4k' first." -ForegroundColor Yellow
    exit 1
}

if (!(Test-Path $flashTool)) {
    Write-Host "ERROR: $flashTool not found!" -ForegroundColor Red
    exit 1
}

$fileSize = (Get-Item $binFile).Length
Write-Host "Firmware: $binFile ($fileSize bytes)" -ForegroundColor Green
Write-Host ""
Write-Host "Make sure the device is in bootloader mode!" -ForegroundColor Yellow
Write-Host "(Hold PA4 for 4 seconds, then release)" -ForegroundColor Yellow
Write-Host ""
Write-Host "Press Enter to flash, or Ctrl+C to cancel..."
Read-Host

Write-Host ""
Write-Host "Flashing..." -ForegroundColor Cyan
& $flashTool $binFile

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=== Flash successful! ===" -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "=== Flash FAILED! ===" -ForegroundColor Red
    Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor Red
}
