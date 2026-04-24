# Flash FF_OSD firmware via USB HID bootloader
# Usage: .\flash-hid.ps1 [optional: path-to-bin-file]
#
# The device must be in HID bootloader mode (LED blinking).
# To enter bootloader mode: Hold PA4 for 4+ seconds until LED blinks.

$ErrorActionPreference = "Stop"

$BinFile = if ($args[0]) { $args[0] } else { "src\FF_OSD_hid.bin" }
$HidFlash = "tools\hid-bootloader\hid_flash\hid-flash.exe"

if (-not (Test-Path $BinFile)) {
    Write-Host "Error: Firmware file not found: $BinFile" -ForegroundColor Red
    Write-Host "Run 'make hid-app' first to build the firmware." -ForegroundColor Yellow
    exit 1
}

if (-not (Test-Path $HidFlash)) {
    Write-Host "Error: hid-flash.exe not found: $HidFlash" -ForegroundColor Red
    exit 1
}

$FileSize = (Get-Item $BinFile).Length
Write-Host "Flashing $BinFile ($FileSize bytes) via USB HID..." -ForegroundColor Cyan

# COM99 is a dummy - the tool only needs it as argument but doesn't use it
# when bootloader is already active
& $HidFlash $BinFile COM99

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nFlash complete! Device is rebooting." -ForegroundColor Green
} else {
    Write-Host "`nFlash failed!" -ForegroundColor Red
    Write-Host "Make sure the device is in bootloader mode (LED blinking)." -ForegroundColor Yellow
    Write-Host "To enter bootloader: Hold PA4 for 4+ seconds." -ForegroundColor Yellow
    exit 1
}
