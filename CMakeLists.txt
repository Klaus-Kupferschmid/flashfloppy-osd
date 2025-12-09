# FlashFloppy-OSD CMake Project
# This CMake configuration works alongside the original Makefile system
# and provides VS Code integration for STM32F103C8T6 and F103CBT6 targets

cmake_minimum_required(VERSION 3.20)

# Use our custom toolchain file
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake)

# Project definition (matches original FW_VER in Makefile)
project(FlashFloppyOSD VERSION 1.9 LANGUAGES C ASM)

# STM32 target selection (default: STM32F103CBT6)
if(NOT DEFINED STM32_TARGET)
    set(STM32_TARGET "f103cbt6" CACHE STRING "STM32 target: f103c8t6 or f103cbt6")
endif()

message(STATUS "Building FlashFloppy-OSD v${PROJECT_VERSION} for STM32${STM32_TARGET}")

# Memory layout based on target (for VS Code IntelliSense)
if(STM32_TARGET STREQUAL "f103c8t6")
    set(FLASH_SIZE 64K)
    set(RAM_SIZE 20K)
    set(TARGET_SUFFIX "_F103C8T6")
    add_compile_definitions(STM32F103C8=1 FLASH_KB=64 SRAM_KB=20)
elseif(STM32_TARGET STREQUAL "f103cbt6")
    set(FLASH_SIZE 128K)
    set(RAM_SIZE 20K)
    set(TARGET_SUFFIX "_F103CBT6")
    add_compile_definitions(STM32F103CB=1 FLASH_KB=128 SRAM_KB=20)
else()
    message(FATAL_ERROR "Unsupported target: ${STM32_TARGET}. Use 'f103c8t6' or 'f103cbt6'")
endif()

# Compiler flags (exact match to original Rules.mk)
set(COMMON_FLAGS "-g -Os -nostdlib -std=gnu99")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wall -Werror -Wno-format -Wdeclaration-after-statement")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wstrict-prototypes -Wredundant-decls -Wnested-externs")
set(COMMON_FLAGS "${COMMON_FLAGS} -fno-common -fno-exceptions -fno-strict-aliasing")
set(COMMON_FLAGS "${COMMON_FLAGS} -mlittle-endian -mthumb -mcpu=cortex-m3 -mfloat-abi=soft")

set(CMAKE_C_FLAGS "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS} -D__ASSEMBLY__")
set(CMAKE_EXE_LINKER_FLAGS "${COMMON_FLAGS} -Wl,--gc-sections")

# Include directories (matches original Rules.mk -iquote)
include_directories(inc)

# Source files (from original src/ directory)
set(SOURCES
    src/amiga.c
    src/build_info.c
    src/cancellation.c
    src/config.c
    src/console.c
    src/i2c.c
    src/main.c
    src/string.c
    src/stm32f10x.c
    src/time.c
    src/timer.c
    src/util.c
    src/vectors.S
)

# Create target for VS Code IntelliSense and debugging
# This creates a library target that VS Code can analyze without building
add_library(FF_OSD_LIB ${SOURCES})

# Force include decls.h (matches original Rules.mk -include decls.h)
target_compile_options(FF_OSD_LIB PRIVATE -include decls.h)

# Add preprocessor definitions for IntelliSense
target_compile_definitions(FF_OSD_LIB PRIVATE
    __GNUC__=14
    __GNUC_MINOR__=3
    __ARM_ARCH_7M__=1
    __thumb__=1
    __THUMBEL__=1
)

# Custom target to call original Makefile system
add_custom_target(build_original
    COMMAND make all
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building with original Makefile system"
    USES_TERMINAL
)

# Custom target to clean original build
add_custom_target(clean_original
    COMMAND powershell -Command "Get-ChildItem -Name -Filter 'FF_OSD-v1.9*' | ForEach-Object { Remove-Item $_ -Recurse -Force -ErrorAction SilentlyContinue }; Get-ChildItem -Recurse -Filter '*.o' | Remove-Item -Force -ErrorAction SilentlyContinue; Get-ChildItem -Recurse -Filter '*.d' | Remove-Item -Force -ErrorAction SilentlyContinue; Get-ChildItem -Path src -Filter '*.elf' | Remove-Item -Force -ErrorAction SilentlyContinue; Get-ChildItem -Path src -Filter '*.bin' | Remove-Item -Force -ErrorAction SilentlyContinue; Get-ChildItem -Path src -Filter '*.hex' | Remove-Item -Force -ErrorAction SilentlyContinue; Get-ChildItem -Path src -Filter '*.ld' -Exclude '*.ld.S' | Remove-Item -Force -ErrorAction SilentlyContinue"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Cleaning original build artifacts"
    USES_TERMINAL
)

# Custom target to flash with ST-Link (using STM32CubeProgrammer)
add_custom_target(flash_stlink
    COMMAND "C:/ST/STM32CubeIDE_1.18.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.stm32cubeprogrammer.win32_2.19.100.202412201438/tools/bin/STM32_Programmer_CLI.exe" -c port=SWD -w src/FF_OSD.hex -v -g
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Flashing FF_OSD.hex via ST-Link"
    DEPENDS build_original
    USES_TERMINAL
)

# Information target
add_custom_target(info
    COMMAND ${CMAKE_COMMAND} -E echo "FlashFloppy-OSD CMake Configuration"
    COMMAND ${CMAKE_COMMAND} -E echo "=================================="
    COMMAND ${CMAKE_COMMAND} -E echo "Version: ${PROJECT_VERSION}"
    COMMAND ${CMAKE_COMMAND} -E echo "Target: STM32${STM32_TARGET} (${FLASH_SIZE} Flash, ${RAM_SIZE} RAM)"
    COMMAND ${CMAKE_COMMAND} -E echo "Toolchain: ${CMAKE_C_COMPILER}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Available targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  build_original  - Build using original Makefile"
    COMMAND ${CMAKE_COMMAND} -E echo "  clean_original  - Clean build artifacts"
    COMMAND ${CMAKE_COMMAND} -E echo "  flash_stlink    - Flash via ST-Link"
    COMMAND ${CMAKE_COMMAND} -E echo "  info           - Show this information"
    COMMENT "Project information"
)