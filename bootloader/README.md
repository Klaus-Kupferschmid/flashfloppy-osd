# FlashFloppy OSD USB HID Bootloader

4KB USB HID bootloader for STM32F103 (Blue Pill).

## Features
- USB HID protocol (no drivers needed on Windows/Linux/Mac)
- I2C LED feedback via ADG715 Frontpanel
- PA4 button triggers bootloader mode
- Magic word (0x424C) for software-triggered updates

## LED Sequence
1. **Button pressed:** Blue blinks fast (max 4s)
2. **Button released:** Red/Blue alternating (2s)
3. **Waiting for flash:** Red LED on, PB2 blinks
4. **After flash:** 3x Blue blink, then app starts

## Building
```powershell
cd bootloader
make build_flashfloppy-osd
```

Output: `build/hid_bootloader.hex` (~2.4KB)

## Flashing

### Initial Flash (ST-Link)
From project root:
```powershell
# Build combined image
.\build-with-bootloader.ps1

# Flash via ST-Link
& "C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe" `
    -c port=SWD -w "FF_OSD_with_bootloader.hex" -v -g
```

### USB Updates
1. Hold PA4 button and reset device
2. Wait for Red/Blue LED sequence
3. Run flash tool:
```powershell
.\flash-usb.ps1
# or directly:
.\tools\hid_flash\hid-flash.exe ..\src\FF_OSD_hid4k.bin COM99
```

## Files
- `Src/main.c` - Bootloader main with LED state machine
- `Src/hid.c` - USB HID protocol and flash handling
- `tools/hid_flash/` - Windows flash tool (hid-flash.exe)
- `flash-usb.ps1` - Flash script

## Technical Details
- USB VID: 0x1209, PID: 0xBEBA
- Bootloader: 0x08000000 - 0x08000FFF (4KB)
- Application: 0x08001000+ 
- MIN_PAGE=4 in hid.c

## IMPORTANT
**NO I2C during USB operation!** I2C calls between USB_Init() and USB_Shutdown() 
cause USB timing conflicts and crashes. All LED changes must happen:
- BEFORE USB_Init() - safe
- AFTER USB_Shutdown() - safe
