# dfu-util für FlashFloppy-OSD

## Installation
dfu-util 0.9 und Zadig sind bereits entpackt in:
```
tools\dfu-util\dfu-util-0.9-win64\
tools\dfu-util\zadig-2.9.exe
```

## USB-Treiber installieren (einmalig)

Falls das DFU-Gerät nicht erkannt wird ("Unbekanntes Gerät"):
```powershell
.\flash-dfu.ps1 -InstallDriver
```

Dann in Zadig:
1. **Options → List All Devices** aktivieren
2. Das STM32/Unbekannte Gerät auswählen
3. **WinUSB** als Treiber wählen
4. **Replace Driver** oder **Install Driver** klicken

## Verwendung

### DFU-Gerät suchen
```powershell
.\flash-dfu.ps1 -List
```

### Firmware flashen (im DFU-Modus)
```powershell
.\flash-dfu.ps1
```

## DFU-Modus aktivieren

1. **Beim Einschalten:** Boot-Select-Taster gedrückt halten
2. **Im Betrieb:** Boot-Select-Taster 4+ Sekunden halten

Die LED (PB2) blinkt im Herzschlag-Muster wenn DFU bereit ist.
