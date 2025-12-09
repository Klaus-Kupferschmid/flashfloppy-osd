# Toolchain file for ARM Cortex-M3 (STM32F103)
# This file configures CMake for cross-compilation to ARM embedded targets

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Prevent CMake from testing the compiler
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Set the toolchain prefix
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# Define toolchain executables
find_program(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
find_program(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
find_program(CMAKE_AR ${TOOLCHAIN_PREFIX}ar)
find_program(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
find_program(CMAKE_OBJDUMP ${TOOLCHAIN_PREFIX}objdump)
find_program(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)
find_program(CMAKE_RANLIB ${TOOLCHAIN_PREFIX}ranlib)

# Check if toolchain is found
if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "ARM GCC toolchain not found! Please ensure arm-none-eabi-gcc is in PATH")
endif()

# Set compiler flags for Cortex-M3
set(COMMON_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")
set(COMMON_FLAGS "${COMMON_FLAGS} -ffunction-sections -fdata-sections")
set(COMMON_FLAGS "${COMMON_FLAGS} -Wall -Werror -Wno-format")

set(CMAKE_C_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS_INIT "${COMMON_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${COMMON_FLAGS}")

# Where to find the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)