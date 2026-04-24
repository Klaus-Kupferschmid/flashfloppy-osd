<#
.SYNOPSIS
    Flash firmware via USB-DFU.

.DESCRIPTION
    Flashes the application firmware to the STM32 via USB-DFU.
    The device must be in DFU mode (Boot-Select held at startup or 4+ seconds).

.PARAMETER List
    Only list DFU devices, don't flash.

.PARAMETER InstallDriver
    Launch Zadig to install WinUSB driver for the DFU device.

.PARAMETER BinFile
    Path to .bin file to flash (default: src\FF_OSD_bootloader.bin)

.EXAMPLE
    .\flash-dfu.ps1
    # Flashes src\FF_OSD_bootloader.bin

.EXAMPLE
    .\flash-dfu.ps1 -List
    # Lists connected DFU devices

.EXAMPLE
    .\flash-dfu.ps1 -InstallDriver
    # Opens Zadig to install USB driver
#>

param(
    [switch]$List,
    [switch]$InstallDriver,
    [string]$BinFile = "src\FF_OSD_bootloader.bin"
)

$ErrorActionPreference = "Stop"
$DfuUtil = "$PSScriptRoot\tools\dfu-util\dfu-util-0.9-win64\dfu-util.exe"
$Zadig = "$PSScriptRoot\tools\dfu-util\zadig-2.9.exe"

if (-not (Test-Path $DfuUtil)) {
    Write-Error "dfu-util not found at: $DfuUtil"
    exit 1
}

if ($InstallDriver) {
    if (-not (Test-Path $Zadig)) {
        Write-Error "Zadig not found at: $Zadig"
        exit 1
    }
    Write-Host "`n=== Installing USB Driver ===" -ForegroundColor Cyan
    Write-Host "1. In Zadig: Options -> List All Devices"
    Write-Host "2. Select the STM32/Unknown device"
    Write-Host "3. Choose 'WinUSB' driver"
    Write-Host "4. Click 'Replace Driver' or 'Install Driver'"
    Write-Host ""
    Start-Process $Zadig -Verb RunAs
    exit 0
}

if ($List) {
    Write-Host "`n=== DFU Devices ===" -ForegroundColor Cyan
    & $DfuUtil -l
    exit 0
}

if (-not (Test-Path $BinFile)) {
    Write-Error "Firmware file not found: $BinFile"
    exit 1
}

Write-Host "`n=== Flashing via USB-DFU ===" -ForegroundColor Cyan
Write-Host "File: $BinFile"
Write-Host "Target: 0x08002000 (Application area)`n"

& $DfuUtil -a 0 -D $BinFile -s 0x08002000:leave

if ($LASTEXITCODE -eq 0) {
    Write-Host "`nFlash successful!" -ForegroundColor Green
} else {
    Write-Host "`nFlash failed. Make sure:" -ForegroundColor Red
    Write-Host "  1. Device is in DFU mode (LED heartbeat)"
    Write-Host "  2. USB cable is connected"
    Write-Host "  3. WinUSB driver is installed (use: .\flash-dfu.ps1 -InstallDriver)"
    exit 1
}
