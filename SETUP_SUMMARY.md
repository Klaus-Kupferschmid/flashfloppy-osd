# FlashFloppy OSD - CMake Setup Zusammenfassung

## ✅ Was bereits erledigt ist:

### 1. Installierte Tools:
- ✅ ARM GCC Toolchain (arm-none-eabi-gcc)
- ✅ CMake 
- ✅ STM32CubeIDE mit ST-Link Tools

### 2. Erstellte Dateien:
- ✅ `CMakeLists.txt` - CMake Konfiguration für beide MCU-Varianten
- ✅ `linker/stm32f103c8t6.ld` - Linker-Skript für 64KB Flash
- ✅ `linker/stm32f103cbt6.ld` - Linker-Skript für 128KB Flash
- ✅ `.vscode/tasks.json` - Build und Flash Tasks
- ✅ `.vscode/launch.json` - Debug-Konfigurationen
- ✅ `.vscode/c_cpp_properties.json` - IntelliSense
- ✅ `.vscode/settings.json` - VS Code Einstellungen
- ✅ `.vscode/cmake-variants.json` - Target-Auswahl
- ✅ `.vscode/extensions.json` - Empfohlene Extensions
- ✅ `build_fixed.ps1` - PowerShell Build-Skript
- ✅ `CMAKE_README.md` - Vollständige Dokumentation

### 3. Konfigurierte Pfade (STM32CubeIDE 1.18.1):
```
STM32_Programmer_CLI: 
C:\Program Files\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.cubeprogrammer.win32_2.2.100.202412061334\tools\bin\STM32_Programmer_CLI.exe

ARM Toolchain:
C:\Program Files\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344\tools\bin

ST-Link GDB Server:
C:\Program Files\ST\STM32CubeIDE_1.18.1\STM32CubeIDE\plugins\com.st.stm32cube.ide.mcu.externaltools.stlink-gdb-server.win32_2.2.100.202501151542\tools\bin
```

## 🚀 Nächste Schritte:

### 1. VS Code Extensions installieren:
```
- C/C++ (ms-vscode.cpptools)
- CMake Tools (ms-vscode.cmake-tools)
- CMake (twxs.cmake)
- Cortex-Debug (marus25.cortex-debug)
- ARM Assembly (dan-c-underwood.arm)
- Linker Script (zixuanwang.linkerscript)
- Hex Editor (ms-vscode.hexeditor)
- Native Debug (webfreak.debug)
```

### 2. Ersten Build testen:
```powershell
# Mit PowerShell Skript
.\build_fixed.ps1 f103cbt6 Debug

# Oder mit CMake direkt
cmake -B build_f103cbt6 -DSTM32_TARGET=F103CBT6 -DCMAKE_BUILD_TYPE=Debug .
cmake --build build_f103cbt6
```

### 3. VS Code Tasks nutzen:
- `Ctrl+Shift+P` → "Tasks: Run Task"
- Wählen Sie "Build F103CBT6" oder "Build F103C8T6"

### 4. Debugging einrichten:
- ST-Link anschließen
- `F5` drücken für Debug-Session
- Oder `Ctrl+Shift+P` → "Debug: Select and Start Debugging"

## 🛠️ Unterstützte Targets:
- **STM32F103C8T6**: 64KB Flash, 20KB RAM
- **STM32F103CBT6**: 128KB Flash, 20KB RAM

## 📁 Build-Verzeichnisse:
- `build_f103c8t6/` - für STM32F103C8T6
- `build_f103cbt6/` - für STM32F103CBT6

## 🔧 Debugging:
- Hardware: ST-Link V2 oder kompatibel
- Interface: SWD (SWDIO, SWCLK, GND, 3.3V)
- Breakpoints: F9
- Step: F10 (over), F11 (into)

## 📖 Vollständige Dokumentation:
Siehe `CMAKE_README.md` für detaillierte Anweisungen.

---
Erstellt am: 9. Dezember 2025
STM32CubeIDE Version: 1.18.1