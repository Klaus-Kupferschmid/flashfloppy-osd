# STM32 Flash Script - Multiple Methods
# Tries different approaches to flash the STM32 with ST-Link

Write-Host "=== STM32 FlashFloppy-OSD Flash Script ===" -ForegroundColor Cyan
Write-Host "Hardware: STM32F103C8T6/CBT6" -ForegroundColor Green
Write-Host ""

# Check if hex file exists
if (-not (Test-Path "src\FF_OSD.hex")) {
    Write-Host "ERROR: src\FF_OSD.hex not found!" -ForegroundColor Red
    Write-Host "Please build the project first." -ForegroundColor Yellow
    exit 1
}

$hexFile = "src\FF_OSD.hex"
$hexSize = (Get-Item $hexFile).Length

Write-Host "Flash file: $hexFile ($hexSize bytes)" -ForegroundColor Green

# Method 1: STM32CubeProgrammer (Standard)
Write-Host "`nMethod 1: STM32CubeProgrammer..." -ForegroundColor Yellow

$programmer = "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"

if (Test-Path $programmer) {
    Write-Host "Testing ST-Link connection..." -ForegroundColor Cyan
    
    # List available probes
    Write-Host "Available ST-Link probes:" -ForegroundColor Cyan
    & $programmer -l 2>$null
    
    Write-Host "`nAttempting flash with STM32CubeProgrammer..." -ForegroundColor Cyan
    $result = & $programmer -c port=SWD -w $hexFile -v -rst 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "SUCCESS: Flash completed with STM32CubeProgrammer!" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "FAILED: STM32CubeProgrammer error:" -ForegroundColor Red
        Write-Host "$result" -ForegroundColor DarkRed
    }
} else {
    Write-Host "ERROR: STM32CubeProgrammer not found!" -ForegroundColor Red
}

# Method 2: Try alternative parameters
Write-Host "`nMethod 2: Alternative ST-Link parameters..." -ForegroundColor Yellow

$alternatives = @(
    @("-c", "port=SWD", "freq=1000", "-w", $hexFile, "-v", "-rst"),
    @("-c", "port=SWD", "mode=UR", "-w", $hexFile, "-v", "-rst"), 
    @("-c", "port=SWD", "sn=16004A002933353739303541", "-w", $hexFile, "-v", "-rst")
)

foreach ($params in $alternatives) {
    $paramString = $params -join " "
    Write-Host "Trying: $paramString" -ForegroundColor Cyan
    $result = & $programmer $params 2>&1
    
    if ($LASTEXITCODE -eq 0) {
        Write-Host "SUCCESS: Flash completed with alternative parameters!" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "Failed with: $paramString" -ForegroundColor DarkRed
    }
}

# Method 3: Information and troubleshooting
Write-Host "`nMethod 3: Troubleshooting Information..." -ForegroundColor Yellow

Write-Host "`nST-Link Probes detected:" -ForegroundColor Cyan
& $programmer -l

Write-Host "`nUSB Devices (ST-Link related):" -ForegroundColor Cyan
Get-PnpDevice | Where-Object { $_.FriendlyName -like "*STM*" -or $_.FriendlyName -like "*ST-LINK*" } | 
    Format-Table FriendlyName, Status, InstanceId -AutoSize

Write-Host "`nTroubleshooting Steps:" -ForegroundColor Yellow
Write-Host "1. Check ST-Link USB connection" -ForegroundColor White
Write-Host "2. Verify target STM32 power (3.3V)" -ForegroundColor White  
Write-Host "3. Check SWD connections (SWDIO, SWCLK, GND)" -ForegroundColor White
Write-Host "4. Try unplugging and reconnecting ST-Link" -ForegroundColor White
Write-Host "5. Close STM32CubeIDE if open (may lock ST-Link)" -ForegroundColor White
Write-Host "6. Try VS Code debugging (F5) as alternative" -ForegroundColor White

Write-Host "`nAlternative: Use VS Code Debugging" -ForegroundColor Cyan
Write-Host "Press F5 in VS Code to start debugging session" -ForegroundColor White
Write-Host "This may work even when direct flashing fails" -ForegroundColor White

exit 1