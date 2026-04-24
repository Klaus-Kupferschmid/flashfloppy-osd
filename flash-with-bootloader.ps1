<#
.SYNOPSIS
    Flash bootloader and application to STM32 via ST-Link.

.DESCRIPTION
    Flashes the USB-DFU bootloader to 0x08000000 and the application
    to 0x08002000. Requires ST-Link connected via SWD.

.PARAMETER BootloaderOnly
    Flash only the bootloader, not the application.

.PARAMETER ApplicationOnly
    Flash only the application (assumes bootloader already present).

.EXAMPLE
    .\flash-with-bootloader.ps1
    # Flashes both bootloader and application

.EXAMPLE
    .\flash-with-bootloader.ps1 -BootloaderOnly
    # Flashes only the bootloader
#>

param(
    [switch]$BootloaderOnly,
    [switch]$ApplicationOnly
)

$ErrorActionPreference = "Stop"

$STM32Programmer = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
$BootloaderHex = "bootloader\bootloader.hex"
$ApplicationHex = "src\FF_OSD_bootloader.hex"

if (-not (Test-Path $STM32Programmer)) {
    Write-Error "STM32CubeProgrammer not found at: $STM32Programmer"
    exit 1
}

# Connect to ST-Link
Write-Host "`n=== Connecting to ST-Link ===" -ForegroundColor Cyan
& $STM32Programmer -c port=SWD
if ($LASTEXITCODE -ne 0) {
    Write-Error "Failed to connect to ST-Link"
    exit 1
}

if (-not $ApplicationOnly) {
    # Flash bootloader
    if (-not (Test-Path $BootloaderHex)) {
        Write-Host "`n=== Building bootloader ===" -ForegroundColor Cyan
        Push-Location bootloader
        & make clean
        & make all
        Pop-Location
        if (-not (Test-Path $BootloaderHex)) {
            Write-Error "Failed to build bootloader"
            exit 1
        }
    }
    
    Write-Host "`n=== Flashing bootloader to 0x08000000 ===" -ForegroundColor Cyan
    & $STM32Programmer -c port=SWD -w $BootloaderHex -v
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to flash bootloader"
        exit 1
    }
    Write-Host "Bootloader flashed successfully!" -ForegroundColor Green
}

if (-not $BootloaderOnly) {
    # Flash application
    if (-not (Test-Path $ApplicationHex)) {
        Write-Error "Application not found: $ApplicationHex`nBuild with: make all BOOTLOADER=y"
        exit 1
    }
    
    Write-Host "`n=== Flashing application to 0x08002000 ===" -ForegroundColor Cyan
    & $STM32Programmer -c port=SWD -w $ApplicationHex -v
    if ($LASTEXITCODE -ne 0) {
        Write-Error "Failed to flash application"
        exit 1
    }
    Write-Host "Application flashed successfully!" -ForegroundColor Green
}

# Start execution
Write-Host "`n=== Starting execution ===" -ForegroundColor Cyan
& $STM32Programmer -c port=SWD -g 0x08000000

Write-Host "`n=== Flash complete! ===" -ForegroundColor Green
Write-Host @"

Memory Layout:
  0x08000000 - Bootloader (8KB)
  0x08002000 - Application
  
To enter DFU mode:
  1. Hold Boot-Select button and power on, OR
  2. Hold Boot-Select for 4+ seconds while running

To flash via USB DFU:
  dfu-util -a 0 -D firmware.bin -s 0x08002000

"@
