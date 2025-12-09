# FlashFloppy OSD - CMake Build System

This document describes the CMake build system setup for the FlashFloppy OSD project, supporting both STM32F103C8T6 and STM32F103CBT6 microcontrollers with VS Code debugging via ST-Link.

## Prerequisites

### Required Tools
1. **ARM GCC Toolchain**: `arm-none-eabi-gcc` and related tools
2. **CMake**: Version 3.15 or higher
3. **ST-Link Tools**: `st-flash`, `st-info` for flashing and debugging
4. **VS Code**: With the recommended extensions (see below)

### Installation on Windows

#### 1. ARM GCC Toolchain
Download and install from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm

Add to PATH: `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin`

#### 2. CMake
Download from: https://cmake.org/download/
Or install via chocolatey: `choco install cmake`

#### 3. ST-Link Tools
Download STM32CubeIDE or install stlink tools separately:
- STM32CubeIDE: https://www.st.com/en/development-tools/stm32cubeide.html
- Standalone stlink: https://github.com/stlink-org/stlink

#### 4. Verify Installation
```powershell
arm-none-eabi-gcc --version
cmake --version
st-flash --version
st-info --version
```

## VS Code Extensions

Install the following extensions (VS Code will prompt automatically with the provided `.vscode/extensions.json`):

1. **C/C++** (`ms-vscode.cpptools`) - IntelliSense, debugging
2. **CMake Tools** (`ms-vscode.cmake-tools`) - CMake integration
3. **CMake** (`twxs.cmake`) - CMake syntax highlighting
4. **Cortex-Debug** (`marus25.cortex-debug`) - ARM debugging
5. **ARM Assembly** (`dan-c-underwood.arm`) - ARM assembly syntax
6. **Linker Script** (`zixuanwang.linkerscript`) - Linker script support
7. **Hex Editor** (`ms-vscode.hexeditor`) - Binary file viewing
8. **Native Debug** (`webfreak.debug`) - Additional debugging support

## Project Structure

```
flashfloppy-osd/
├── CMakeLists.txt              # Main CMake configuration
├── linker/                     # Linker scripts for different targets
│   ├── stm32f103c8t6.ld      # 64KB Flash variant
│   └── stm32f103cbt6.ld      # 128KB Flash variant
├── .vscode/                    # VS Code configuration
│   ├── tasks.json             # Build and flash tasks
│   ├── launch.json            # Debug configurations
│   ├── c_cpp_properties.json  # IntelliSense configuration
│   ├── cmake-variants.json    # Target selection
│   ├── settings.json          # VS Code settings
│   └── extensions.json        # Recommended extensions
├── build_f103c8t6/            # Build output for STM32F103C8T6
├── build_f103cbt6/            # Build output for STM32F103CBT6
├── src/                       # Source files
└── inc/                       # Header files
```

## Building the Project

### Using VS Code (Recommended)

1. **Open the project** in VS Code
2. **Install extensions** when prompted
3. **Select target variant**:
   - Press `Ctrl+Shift+P`
   - Type "CMake: Select Variant"
   - Choose "STM32F103C8T6" or "STM32F103CBT6"
4. **Configure**:
   - Press `Ctrl+Shift+P`
   - Type "CMake: Configure"
5. **Build**:
   - Press `F7` or use `Ctrl+Shift+P` → "CMake: Build"
   - Or use the build tasks: `Ctrl+Shift+P` → "Tasks: Run Task"

### Using Command Line

#### Configure and Build for STM32F103C8T6 (64KB Flash)
```powershell
cmake -B build_f103c8t6 -DSTM32_TARGET=F103C8T6 -DCMAKE_BUILD_TYPE=Debug .
cmake --build build_f103c8t6
```

#### Configure and Build for STM32F103CBT6 (128KB Flash) 
```powershell
cmake -B build_f103cbt6 -DSTM32_TARGET=F103CBT6 -DCMAKE_BUILD_TYPE=Debug .
cmake --build build_f103cbt6
```

#### Release Build
```powershell
cmake -B build_f103cbt6_release -DSTM32_TARGET=F103CBT6 -DCMAKE_BUILD_TYPE=Release .
cmake --build build_f103cbt6_release
```

## Flashing the Firmware

### Using VS Code Tasks
1. Connect ST-Link to your STM32 board
2. Press `Ctrl+Shift+P` → "Tasks: Run Task"
3. Select "Flash F103C8T6" or "Flash F103CBT6"

### Using Command Line
```powershell
# Flash STM32F103C8T6
st-flash write build_f103c8t6/FF_OSD-v1.9-F103C8T6.bin 0x8000000

# Flash STM32F103CBT6  
st-flash write build_f103cbt6/FF_OSD-v1.9-F103CBT6.bin 0x8000000
```

### Verify ST-Link Connection
```powershell
st-info --probe
```

## Debugging

### Prerequisites for Debugging
1. **Hardware**: ST-Link V2 or compatible debugger
2. **Connection**: SWD interface (SWDIO, SWCLK, GND, 3.3V)
3. **Extensions**: Cortex-Debug extension installed

### Debug Configurations Available
- **Debug STM32F103C8T6**: Launch debugging for C8T6 variant
- **Debug STM32F103CBT6**: Launch debugging for CBT6 variant  
- **Attach to STM32F103C8T6**: Attach to running C8T6 target
- **Attach to STM32F103CBT6**: Attach to running CBT6 target

### Starting a Debug Session
1. **Build the project** first (debug builds include debug symbols)
2. **Connect ST-Link** to your target board
3. **Set breakpoints** in your code
4. **Start debugging**:
   - Press `F5` or use the debug panel
   - Select the appropriate debug configuration
   - VS Code will automatically build, flash, and start debugging

### Debug Features
- **Breakpoints**: Set/clear with F9
- **Step functions**: F10 (over), F11 (into), Shift+F11 (out)
- **Variables**: View in debug panel
- **Registers**: ARM CPU and peripheral registers
- **Memory**: Examine memory contents
- **Call Stack**: Function call hierarchy

## Target Differences

| Feature | STM32F103C8T6 | STM32F103CBT6 |
|---------|---------------|---------------|
| Flash   | 64KB          | 128KB         |
| RAM     | 20KB          | 20KB          |
| Package | LQFP48        | LQFP48        |
| Build   | build_f103c8t6 | build_f103cbt6 |

Both targets use the same Cortex-M3 core and have identical peripheral sets.

## Troubleshooting

### Common Issues

#### 1. Toolchain Not Found
```
Error: arm-none-eabi-gcc not found
```
**Solution**: Add ARM GCC toolchain to PATH or specify `CMAKE_C_COMPILER` path.

#### 2. ST-Link Not Detected
```
Error: Could not find ST-Link
```
**Solution**:
- Install ST-Link drivers
- Check USB connection
- Run `st-info --probe` to verify

#### 3. Permission Denied (Linux/WSL)
```
Error: Permission denied accessing ST-Link
```
**Solution**: Add udev rules or run with sudo (not recommended for production).

#### 4. Build Errors
- Check that all source files exist
- Verify toolchain installation
- Clean build directory: `rm -rf build_*`

### Debug Issues

#### 1. Cannot Start Debug Session
- Verify ST-Link connection with `st-info --probe`
- Check that Cortex-Debug extension is installed
- Ensure build completed successfully
- Try "Attach" configuration instead of "Launch"

#### 2. Breakpoints Not Hit
- Verify debug build (not release)
- Check that code is actually executed
- Ensure proper ELF file is loaded

#### 3. Variables Not Visible
- Use debug build with `-g` flag (automatic in Debug configuration)
- Check optimization level (Debug uses `-Og`)

## Advanced Configuration

### Custom Build Configurations
Modify `CMakeLists.txt` to add custom build configurations:

```cmake
# Custom configuration example
if(CMAKE_BUILD_TYPE STREQUAL "MinSize")
    set(CMAKE_C_FLAGS "${COMMON_FLAGS} -g -Os -DNDEBUG")
endif()
```

### Adding Source Files
Add new source files to the `SOURCES` list in `CMakeLists.txt`:

```cmake
set(SOURCES
    src/amiga.c
    src/your_new_file.c  # Add here
    # ... other files
)
```

### Memory Layout Customization
Modify linker scripts in `linker/` directory to change memory layout.

## Integration with Original Makefile

The original Makefile system is preserved and can be used alongside CMake:

```bash
# Original Makefile build
make clean
make all

# CMake build  
cmake --build build_f103cbt6
```

Both systems generate compatible output files.