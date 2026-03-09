# GitHub Copilot Instructions – FlashFloppy-OSD

## Projektübersicht / Project Overview

Dieses Projekt ist ein **STM32F103-basiertes OSD (On-Screen-Display)** für FlashFloppy, einen Amiga-Floppy-Emulator.

- **Mikrocontroller**: STM32F103C8T6 / STM32F103CBT6 (ARM Cortex-M3)
- **Sprache**: C (GNU99)
- **Build-System**: CMake + Ninja / Make
- **Toolchain**: ARM GCC (arm-none-eabi-gcc)
- **Debugger**: ST-Link via Cortex-Debug (VS Code)

## Coding-Standards

- Sprache: C (gnu99 Standard)
- Einrückung: 4 Leerzeichen
- Headerdateien in `inc/`, Quelldateien in `src/`
- Embedded-Stil: Keine dynamische Speicherverwaltung, kein stdlib malloc
- Register-Zugriffe via STM32 HAL oder direkte CMSIS-Register-Zugriffe

## Build-Anweisungen

```bash
cmake -S . -B build -G Ninja
cmake --build build
```

## Wichtige Dateien

- `src/` – C-Quelldateien
- `inc/` – Header-Dateien
- `CMakeLists.txt` – CMake Build-Konfiguration
- `.vscode/` – VS Code IDE-Konfiguration

## KI-Modell-Präferenz

Dieses Repository verwendet **GitHub Copilot** mit dem Modell **GPT-4.5** (gpt-4.5) für Code-Vervollständigung und Chat-Unterstützung.
