# FlashFloppy OSD Build Script for Windows PowerShell
# Usage: .\build.ps1 [target] [build_type]
# target: f103c8t6, f103cbt6, both (default: f103cbt6)  
# build_type: Debug, Release (default: Debug)

param(
    [Parameter(Position=0)]
    [ValidateSet("f103c8t6", "f103cbt6", "both")]
    [string]$Target = "f103cbt6",
    
    [Parameter(Position=1)]
    [ValidateSet("Debug", "Release")]
    [string]$BuildType = "Debug",
    
    [switch]$Clean,
    [switch]$Flash,
    [switch]$Help
)

function Show-Help {
    Write-Host "FlashFloppy OSD Build Script" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "Usage: .\build.ps1 [options] [target] [build_type]" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Parameters:" -ForegroundColor Green
    Write-Host "  target      Target MCU: f103c8t6, f103cbt6, both (default: f103cbt6)"
    Write-Host "  build_type  Build type: Debug, Release (default: Debug)"
    Write-Host ""
    Write-Host "Options:" -ForegroundColor Green  
    Write-Host "  -Clean      Clean build directories before building"
    Write-Host "  -Flash      Flash firmware after successful build"
    Write-Host "  -Help       Show this help message"
    Write-Host ""
    Write-Host "Examples:" -ForegroundColor Yellow
    Write-Host "  .\build.ps1                          # Build F103CBT6 Debug"
    Write-Host "  .\build.ps1 f103c8t6 Release         # Build F103C8T6 Release"
    Write-Host "  .\build.ps1 both -Clean              # Clean and build both targets"
    Write-Host "  .\build.ps1 f103cbt6 Debug -Flash    # Build and flash F103CBT6"
}

function Test-Prerequisites {
    Write-Host "Checking prerequisites..." -ForegroundColor Cyan
    
    $tools = @(
        @{Name = "cmake"; Command = "cmake --version"},
        @{Name = "arm-none-eabi-gcc"; Command = "arm-none-eabi-gcc --version"},
        @{Name = "STM32_Programmer_CLI"; Command = "`"C:\Program Files\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.100.202412061334\tools\bin\STM32_Programmer_CLI.exe`" --version"}
    )
    
    $missing = @()
    
    foreach ($tool in $tools) {
        try {
            $null = Invoke-Expression $tool.Command 2>$null
            Write-Host "✓ $($tool.Name) found" -ForegroundColor Green
        }
        catch {
            Write-Host "✗ $($tool.Name) not found" -ForegroundColor Red
            $missing += $tool.Name
        }
    }
    
    if ($missing.Count -gt 0) {
        Write-Host ""
        Write-Host "Missing tools: $($missing -join ', ')" -ForegroundColor Red
        Write-Host "Please install the missing tools and try again." -ForegroundColor Red
        Write-Host "See CMAKE_README.md for installation instructions." -ForegroundColor Yellow
        exit 1
    }
    
    Write-Host ""
}

function Clean-BuildDirs {
    Write-Host "Cleaning build directories..." -ForegroundColor Cyan
    
    $buildDirs = Get-ChildItem -Directory -Name "build_*" -ErrorAction SilentlyContinue
    
    if ($buildDirs) {
        foreach ($dir in $buildDirs) {
            Write-Host "  Removing $dir" -ForegroundColor Yellow
            Remove-Item -Recurse -Force $dir -ErrorAction SilentlyContinue
        }
    } else {
        Write-Host "  No build directories to clean" -ForegroundColor Gray
    }
    
    Write-Host ""
}

function Build-Target {
    param(
        [string]$TargetName,
        [string]$BuildType
    )
    
    $targetUpper = $TargetName.ToUpper()
    $buildDir = "build_$TargetName"
    
    Write-Host "Building $targetUpper ($BuildType)..." -ForegroundColor Cyan
    Write-Host "  Build directory: $buildDir" -ForegroundColor Gray
    
    # Configure
    Write-Host "  Configuring..." -ForegroundColor Yellow
    $configCmd = "cmake -B $buildDir -DSTM32_TARGET=$targetUpper -DCMAKE_BUILD_TYPE=$BuildType ."
    Write-Host "  Command: $configCmd" -ForegroundColor Gray
    
    try {
        Invoke-Expression $configCmd
        if ($LASTEXITCODE -ne 0) {
            throw "Configuration failed"
        }
    }
    catch {
        Write-Host "  ✗ Configuration failed for $targetUpper" -ForegroundColor Red
        return $false
    }
    
    # Build
    Write-Host "  Building..." -ForegroundColor Yellow
    $buildCmd = "cmake --build $buildDir"
    Write-Host "  Command: $buildCmd" -ForegroundColor Gray
    
    try {
        Invoke-Expression $buildCmd
        if ($LASTEXITCODE -ne 0) {
            throw "Build failed"
        }
    }
    catch {
        Write-Host "  ✗ Build failed for $targetUpper" -ForegroundColor Red
        return $false
    }
    
    # Check output files
    $elfFile = "$buildDir\FF_OSD.elf"
    $binFile = "$buildDir\FF_OSD-v1.9-$targetUpper.bin"
    $hexFile = "$buildDir\FF_OSD-v1.9-$targetUpper.hex"
    
    if ((Test-Path $elfFile) -and (Test-Path $binFile) -and (Test-Path $hexFile)) {
        Write-Host "  ✓ Build successful for $targetUpper" -ForegroundColor Green
        
        # Show file sizes
        $binSize = (Get-Item $binFile).Length
        $elfSize = (Get-Item $elfFile).Length
        Write-Host "    Binary size: $binSize bytes" -ForegroundColor Gray
        Write-Host "    ELF size: $elfSize bytes" -ForegroundColor Gray
        
        return $true
    } else {
        Write-Host "  ✗ Output files missing for $targetUpper" -ForegroundColor Red
        return $false
    }
}

function Flash-Target {
    param([string]$TargetName)
    
    $targetUpper = $TargetName.ToUpper()
    $buildDir = "build_$TargetName"
    $binFile = "$buildDir\FF_OSD-v1.9-$targetUpper.bin"
    
    Write-Host "Flashing $targetUpper..." -ForegroundColor Cyan
    
    if (-not (Test-Path $binFile)) {
        Write-Host "  ✗ Binary file not found: $binFile" -ForegroundColor Red
        return $false
    }
    
    # Check ST-Link connection
    Write-Host "  Checking ST-Link connection..." -ForegroundColor Yellow
    try {
        $stm32ProgPath = "C:\Program Files\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.100.202412061334\tools\bin\STM32_Programmer_CLI.exe"
        $probeOutput = & $stm32ProgPath -l 2>&1
        if ($LASTEXITCODE -ne 0) {
            throw "ST-Link probe failed"
        }
        Write-Host "  ST-Link detected" -ForegroundColor Green
    }
    catch {
        Write-Host "  ✗ No ST-Link detected" -ForegroundColor Red
        Write-Host "    Make sure ST-Link is connected and drivers are installed" -ForegroundColor Yellow
        return $false
    }
    
    # Flash
    Write-Host "  Flashing..." -ForegroundColor Yellow
    $stm32ProgPath = "C:\Program Files\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.100.202412061334\tools\bin\STM32_Programmer_CLI.exe"
    $flashCmd = "& `"$stm32ProgPath`" -c port=SWD -w `"$binFile`" 0x08000000 -v -rst"
    Write-Host "  Command: $flashCmd" -ForegroundColor Gray
    
    try {
        & $stm32ProgPath -c "port=SWD" -w $binFile "0x08000000" -v -rst
        if ($LASTEXITCODE -ne 0) {
            throw "Flash failed"
        }
        Write-Host "  ✓ Flash successful for $targetUpper" -ForegroundColor Green
        return $true
    }
    catch {
        Write-Host "  ✗ Flash failed for $targetUpper" -ForegroundColor Red
        return $false
    }
}

# Main script
if ($Help) {
    Show-Help
    exit 0
}

Write-Host "FlashFloppy OSD Build Script" -ForegroundColor Cyan
Write-Host "===========================" -ForegroundColor Cyan
Write-Host ""

# Check prerequisites
Test-Prerequisites

# Clean if requested
if ($Clean) {
    Clean-BuildDirs
}

# Determine targets to build
$targets = @()
if ($Target -eq "both") {
    $targets = @("f103c8t6", "f103cbt6")
} else {
    $targets = @($Target)
}

# Build targets
$buildResults = @{}
foreach ($t in $targets) {
    $buildResults[$t] = Build-Target -TargetName $t -BuildType $BuildType
    Write-Host ""
}

# Flash if requested and build was successful
if ($Flash) {
    if ($Target -eq "both") {
        Write-Host "Flash option not supported with 'both' target. Please specify a single target." -ForegroundColor Yellow
    } elseif ($buildResults[$Target]) {
        Flash-Target -TargetName $Target
    } else {
        Write-Host "Skipping flash due to build failure" -ForegroundColor Yellow
    }
    Write-Host ""
}

# Summary
Write-Host "Build Summary:" -ForegroundColor Cyan
Write-Host "==============" -ForegroundColor Cyan
foreach ($t in $targets) {
    $status = if ($buildResults[$t]) { "SUCCESS" } else { "FAILED" }
    $color = if ($buildResults[$t]) { "Green" } else { "Red" }
    Write-Host "  $($t.ToUpper()): $status" -ForegroundColor $color
}

# Exit with error code if any build failed
$failedBuilds = $buildResults.Values | Where-Object { $_ -eq $false }
if ($failedBuilds.Count -gt 0) {
    Write-Host ""
    Write-Host "One or more builds failed. Check the output above for details." -ForegroundColor Red
    exit 1
} else {
    Write-Host ""
    Write-Host "All builds completed successfully!" -ForegroundColor Green
    exit 0
}