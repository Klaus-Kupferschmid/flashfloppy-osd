# Build FF_OSD with HID Bootloader
# Creates combined firmware: Bootloader (4KB) + App

param(
    [switch]$Debug,
    [switch]$FlashOnly,
    [switch]$Clean
)

$ErrorActionPreference = "Stop"

$ROOT = Split-Path -Parent $MyInvocation.MyCommand.Path
$MAKE = "C:\Users\KlausKupferschmid\AppData\Local\Microsoft\WinGet\Packages\ezwinports.make_Microsoft.Winget.Source_8wekyb3d8bbwe\bin\make.exe"
$OBJCOPY = "arm-none-eabi-objcopy"
$SREC_CAT = "srec_cat"  # Optional: for hex merging
$STM32_PROGRAMMER = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"

$BOOTLOADER_DIR = "$ROOT\bootloader_hid"
$BOOTLOADER_HEX = "$BOOTLOADER_DIR\build\hid_bootloader.hex"
$BOOTLOADER_BIN = "$BOOTLOADER_DIR\build\hid_bootloader.bin"

$APP_DIR = "$ROOT\src"
$APP_HEX = "$APP_DIR\FF_OSD_hid4k.hex"
$APP_BIN = "$APP_DIR\FF_OSD_hid4k.bin"

$COMBINED_HEX = "$ROOT\FF_OSD_with_bootloader.hex"
$COMBINED_BIN = "$ROOT\FF_OSD_with_bootloader.bin"

Write-Host "============================================" -ForegroundColor Cyan
Write-Host "FF_OSD HID Bootloader Build System" -ForegroundColor Cyan
Write-Host "============================================" -ForegroundColor Cyan

if ($Clean) {
    Write-Host "`n[1/1] Cleaning..." -ForegroundColor Yellow
    Remove-Item -Force -ErrorAction SilentlyContinue "$BOOTLOADER_DIR\build\*"
    Remove-Item -Force -ErrorAction SilentlyContinue "$APP_DIR\*.o"
    Remove-Item -Force -ErrorAction SilentlyContinue "$APP_DIR\FF_OSD_hid4k.*"
    Remove-Item -Force -ErrorAction SilentlyContinue "$COMBINED_HEX"
    Remove-Item -Force -ErrorAction SilentlyContinue "$COMBINED_BIN"
    Write-Host "Clean complete." -ForegroundColor Green
    exit 0
}

if (-not $FlashOnly) {
    # Step 1: Build bootloader
    Write-Host "`n[1/4] Building HID Bootloader..." -ForegroundColor Yellow
    Push-Location $BOOTLOADER_DIR
    try {
        # Create build dir if needed
        if (!(Test-Path build)) { New-Item -ItemType Directory -Path build | Out-Null }
        
        & $MAKE build_flashfloppy-osd V=0
        if ($LASTEXITCODE -ne 0) { throw "Bootloader build failed" }
        
        $blSize = (Get-Item $BOOTLOADER_BIN).Length
        Write-Host "  Bootloader size: $blSize bytes" -ForegroundColor Gray
        if ($blSize -gt 4096) {
            throw "Bootloader too large! ($blSize > 4096 bytes)"
        }
    } finally {
        Pop-Location
    }

    # Step 2: Build app with HID4K linker script
    Write-Host "`n[2/4] Building Application (HID4K)..." -ForegroundColor Yellow
    Push-Location $APP_DIR
    try {
        # Copy HID4K linker script template to FF_OSD_hid4k.ld
        $ldFlags = ""
        if ($Debug) {
            $ldFlags = "debug=y"
            Write-Host "  Debug build enabled" -ForegroundColor Gray
        }
        
        # Build with HID4K linker script
        # The linker script is FF_OSD_hid4k.ld.S -> FF_OSD_hid4k.ld
        $makeArgs = @("-f", "$ROOT\Rules.mk", "FF_OSD_hid4k.elf", "FF_OSD_hid4k.bin", "FF_OSD_hid4k.hex")
        if ($Debug) {
            $env:debug = "y"
        }
        & $MAKE @makeArgs
        if ($LASTEXITCODE -ne 0) { throw "Application build failed" }
        
        $appSize = (Get-Item $APP_BIN).Length
        Write-Host "  Application size: $appSize bytes" -ForegroundColor Gray
    } finally {
        Pop-Location
        $env:debug = $null
    }

    # Step 3: Combine bootloader + app into single hex
    Write-Host "`n[3/4] Combining Bootloader + App..." -ForegroundColor Yellow
    
    # Method 1: Try srec_cat if available (cleaner)
    $useSrec = $false
    try {
        & $SREC_CAT --version 2>&1 | Out-Null
        $useSrec = $true
    } catch {
        $useSrec = $false
    }
    
    if ($useSrec) {
        & $SREC_CAT $BOOTLOADER_HEX -Intel $APP_HEX -Intel -o $COMBINED_HEX -Intel
    } else {
        # Method 2: Manual hex concatenation
        # Intel HEX can simply be concatenated (minus the EOF record from first file)
        $blLines = Get-Content $BOOTLOADER_HEX
        $appLines = Get-Content $APP_HEX
        
        # Remove EOF from bootloader (:00000001FF)
        $blLines = $blLines | Where-Object { $_ -ne ":00000001FF" }
        
        # Write combined file
        $blLines + $appLines | Set-Content $COMBINED_HEX
        Write-Host "  Combined using hex concatenation" -ForegroundColor Gray
    }
    
    # Create combined binary
    & $OBJCOPY -I ihex -O binary $COMBINED_HEX $COMBINED_BIN
    
    $combinedSize = (Get-Item $COMBINED_BIN).Length
    Write-Host "  Combined size: $combinedSize bytes" -ForegroundColor Gray
}

# Step 4: Flash to device
Write-Host "`n[4/4] Flashing to STM32..." -ForegroundColor Yellow
Write-Host "  Erasing chip..." -ForegroundColor Gray
& $STM32_PROGRAMMER -c port=SWD reset=HWrst -e all
if ($LASTEXITCODE -ne 0) { 
    Write-Host "  Warning: Erase may have failed, continuing..." -ForegroundColor Yellow
}

Write-Host "  Programming..." -ForegroundColor Gray
& $STM32_PROGRAMMER -c port=SWD reset=HWrst -w $COMBINED_HEX -v -g
if ($LASTEXITCODE -ne 0) { throw "Flash programming failed" }

Write-Host "`n============================================" -ForegroundColor Green
Write-Host "SUCCESS! Firmware flashed." -ForegroundColor Green
Write-Host "============================================" -ForegroundColor Green
Write-Host ""
Write-Host "To enter bootloader mode:" -ForegroundColor Cyan
Write-Host "  Hold PA4 (Boot-Select) for 4 seconds" -ForegroundColor White
Write-Host ""
Write-Host "To flash via USB HID:" -ForegroundColor Cyan
Write-Host "  .\tools\hid-flash.exe src\FF_OSD_hid4k.bin" -ForegroundColor White
