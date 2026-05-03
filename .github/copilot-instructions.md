# GitHub Copilot Instructions for FlashFloppy-OSD

## Project Overview

FlashFloppy-OSD is an **On Screen Display (OSD) and keyboard controller** firmware for
[FlashFloppy](https://github.com/keirf/flashfloppy), a floppy-disk emulator for retro computers.
The firmware runs on an **STM32F103C8T6 / STM32F103CBT6** (ARM Cortex-M3) microcontroller.

Key capabilities:
- Sends FlashFloppy output to a computer video display via OSD
- Controls FlashFloppy via an Amiga keyboard (optional)
- Emulates the popular LCD I2C interface for broader hardware compatibility

## Language and Toolchain

- **Language**: C (GNU C99 dialect), with ARM assembly (`.S` files)
- **Compiler**: `arm-none-eabi-gcc`
- **Build system**: GNU Make (`Makefile` / `Rules.mk`) — also a CMake wrapper for VS Code IntelliSense
- **Target MCU**: STM32F103C8T6 (64 KB Flash, 20 KB SRAM) or STM32F103CBT6 (128 KB Flash)
- **Architecture flags**: `-mthumb -mcpu=cortex-m3 -mfloat-abi=soft`

## Repository Layout

```
.
├── src/          # All C and assembly source files (main.c, amiga.c, i2c.c, …)
├── inc/          # Header files shared across sources
├── cmake/        # CMake toolchain file for VS Code integration
├── build/        # CMake build directory (generated, not committed)
├── Makefile      # Primary build entry point
├── Rules.mk      # Compiler rules shared by Make
├── CMakeLists.txt# CMake configuration (IntelliSense / VS Code only)
└── README.md
```

## Coding Conventions

- C99 style; always include `decls.h` (force-included by the build system)
- Avoid dynamic memory allocation — this is a bare-metal, `nostdlib` firmware
- Use `uint8_t`, `uint16_t`, `uint32_t` (from `<stdint.h>`) for hardware registers
- Peripheral registers are accessed through structures defined in `inc/stm32f10x_regs.h`
- Interrupt handlers follow the ARM Cortex-M3 vector table in `src/vectors.S`
- Keep stack usage minimal; prefer static or global buffers

## Building

```bash
# Full build (produces FF_OSD.elf, .bin, .hex in src/)
make all

# Clean
make clean

# Flash via UART bootloader
make flash        # uses stm32flash, /dev/ttyUSB0 at 921600 baud

# Flash via ST-Link (Windows, STM32CubeProgrammer)
.\flash-stm32.ps1
```

## How to Enable GitHub Copilot LLMs in the GitHub Portal

GitHub Copilot (including its LLM-powered features) is enabled at the **account or organisation level**,
not per-repository. Follow these steps:

1. **Personal accounts**
   - Go to **github.com → Your profile picture → Settings**
   - Select **Copilot** in the left sidebar (under "Code, planning, and automation")
   - Click **"Enable GitHub Copilot"** and follow the subscription/trial wizard

2. **Organisation accounts**
   - Go to **github.com → Your organisation → Settings**
   - Under **"Code, planning, and automation"** choose **Copilot → Policies**
   - Set the **"GitHub Copilot in your organization"** policy to *Enabled for all members*
     (or *Enabled for selected members*)
   - Optionally choose which **Copilot models / LLMs** are available under
     **"Copilot in GitHub.com → Models"**

3. **Repository-level configuration** (this file)
   - Once Copilot is enabled at account/org level, placing a
     `.github/copilot-instructions.md` in the repository provides project-specific
     context to the Copilot LLM, improving suggestion quality automatically.

> **Note:** GitHub Copilot requires either a paid individual subscription or an
> organisation seat. Free trials are available at <https://github.com/features/copilot>.
