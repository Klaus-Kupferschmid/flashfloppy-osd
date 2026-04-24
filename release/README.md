# FlashFloppy-OSD Firmware Release

## Contents

| File | Description |
|------|-------------|
| `FF_OSD_with_bootloader.hex` | Complete firmware (bootloader + app) for ST-Link |
| `FF_OSD_bootloader.hex` | Bootloader only (4KB) |
| `FF_OSD_hid4k.bin` | Application only (for USB updates) |
| `hid-flash.exe` | USB flash tool |
| `flash-usb.ps1` | PowerShell script for easy USB flashing |

## Initial Installation (ST-Link required)

```powershell
STM32_Programmer_CLI -c port=SWD -w FF_OSD_with_bootloader.hex -v -g
```

## USB Firmware Upgrade (no ST-Link needed)

### Method 1: Using flash-usb.ps1

1. Hold PA4 (Boot-Select) button for 4 seconds
2. LEDs blink blue/pink = bootloader active
3. Run: `.\flash-usb.ps1`
4. Press Enter when prompted
5. LEDs blink pink during flash
6. Solid pink = success, auto-restart

### Method 2: Manual

1. Enter bootloader mode (hold PA4 4s or hold during reset)
2. Run: `.\hid-flash.exe FF_OSD_hid4k.bin COM99`
3. Wait for "Done!" message

## LED States in Bootloader

| LED Pattern | Meaning |
|-------------|---------|
| Blue/Pink alternating | Waiting for flash command |
| Fast pink blink | Flashing in progress |
| Solid pink (2s) | Success, restarting |

## Troubleshooting

- **No USB device detected**: Check USB cable, ensure board has external 5V power
- **Flash fails immediately**: Re-enter bootloader mode, try again
- **LEDs don't light up**: Normal, PB2 LED may not work on some boards

## Technical Details

- Bootloader: 4KB @ 0x08000000
- Application: @ 0x08001000
- USB VID:PID = 1209:BEBA
- Magic word: 0x424C in BKP->DR4
