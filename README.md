# Vibration_Sensor_Driver

GPIO driver control system with interrupt and timer for audio device (Speaker).

## Features
- Control GPIO output (Speaker) via GPIO input interrupt
- Auto-off timer after 5 seconds
- Interrupt debouncing
- Activity logging
- Menu-driven control interface


## Hardware
- GPIO 22: Output (Speaker)
- GPIO 27: Input (Sensor)
Cross-Compilation Setup
This project uses cross-compilation to build on Ubuntu and deploy to Raspberry Pi. Cross-compilation allows you to compile source code on a powerful host machine (Ubuntu) to generate executable files for the target device (Raspberry Pi) with different architecture.
Prerequisites

Ubuntu development machine (17.01)
Raspberry Pi with kernel headers installed
Cross-compilation toolchain for ARM
### Control Menu
- `0` - Turn off Speaker
- `1` - Turn on Speaker  
- `2` - Check Speaker status
- `3` - Activate sensor (auto mode)
- `4` - Deactivate sensor (manual mode)
- `q` - Quit

- When interrupt from GPIO 27 → Speaker turns on
- Auto turn off after 5 seconds
- Next interrupt will turn off Speaker immediately
