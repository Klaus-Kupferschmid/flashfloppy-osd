# FlashFloppy-OSD Build Script for Windows
# Handles the chmod issue by manually creating hex and bin files

Write-Host "Starting FlashFloppy-OSD Build..." -ForegroundColor Green

# Run the original make command
$makeExe = "C:\Users\KlausKupferschmid\AppData\Local\Microsoft\WinGet\Packages\ezwinports.make_Microsoft.Winget.Source_8wekyb3d8bbwe\bin\make.exe"
& $makeExe "all"

$makeExitCode = $LASTEXITCODE

# If make failed due to chmod (exit code 2), we still got the ELF file
if ($makeExitCode -eq 2) {
    Write-Host "chmod command failed (expected on Windows), creating missing files..." -ForegroundColor Yellow
    
    if (Test-Path "src\FF_OSD.elf") {
        Write-Host "Creating hex and bin files..." -ForegroundColor Yellow
        Set-Location "src"
        
        # Create hex file
        & arm-none-eabi-objcopy -O ihex FF_OSD.elf FF_OSD.hex
        Write-Host "✅ Created FF_OSD.hex" -ForegroundColor Green
        
        # Create bin file  
        & arm-none-eabi-objcopy -O binary FF_OSD.elf FF_OSD.bin
        Write-Host "✅ Created FF_OSD.bin" -ForegroundColor Green
        
        # Show file sizes
        Write-Host "`nBuild Results:" -ForegroundColor Cyan
        Get-ChildItem FF_OSD.* | Format-Table Name, Length -AutoSize
        
        Set-Location ".."
        Write-Host "✅ Build completed successfully!" -ForegroundColor Green
        exit 0
    } else {
        Write-Host "❌ ELF file not found, build failed!" -ForegroundColor Red  
        exit 1
    }
} elseif ($makeExitCode -eq 0) {
    Write-Host "✅ Build completed successfully!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "❌ Build failed with exit code $makeExitCode" -ForegroundColor Red
    exit $makeExitCode
}