# FlashFloppy-OSD USB Firmware Upgrade Script
# Usage: .\flash-usb.ps1 [firmware.bin]

param(
    [string]$FirmwareFile = "FF_OSD_hid4k.bin"
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$FlashTool = Join-Path $ScriptDir "hid-flash.exe"
$BinFile = Join-Path $ScriptDir $FirmwareFile

if (!(Test-Path $FlashTool)) {
    Write-Host "ERROR: hid-flash.exe not found!" -ForegroundColor Red
    exit 1
}

if (!(Test-Path $BinFile)) {
    Write-Host "ERROR: Firmware file '$FirmwareFile' not found!" -ForegroundColor Red
    exit 1
}

Write-Host "FlashFloppy-OSD USB Firmware Upgrade" -ForegroundColor Cyan
Write-Host "=====================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Firmware: $FirmwareFile" -ForegroundColor Yellow
Write-Host "Size: $((Get-Item $BinFile).Length) bytes" -ForegroundColor Yellow
Write-Host ""
Write-Host "Instructions:" -ForegroundColor Green
Write-Host "1. Hold PA4 button for 4 seconds (or hold during reset)"
Write-Host "2. LEDs should blink blue/pink"
Write-Host "3. Press Enter to flash..."
Write-Host ""

Read-Host "Press Enter when bootloader is active"

Write-Host "Flashing..." -ForegroundColor Yellow
& $FlashTool $BinFile COM99

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "SUCCESS! Firmware updated." -ForegroundColor Green
} else {
    Write-Host ""
    Write-Host "FAILED! Check if bootloader is active." -ForegroundColor Red
}
