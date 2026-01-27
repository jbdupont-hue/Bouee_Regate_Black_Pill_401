# VS Code PlatformIO Settings

This directory contains configuration for Visual Studio Code and the PlatformIO extension.

## Files

### settings.json
Contains PlatformIO-specific settings:
- **defaultEnvironment**: Which board to build for by default
  - `genericSTM32F401CB` - STM32F401CB Black Pill (default)
  - `genericSTM32F405RG` - STM32F405RG
- **serialMonitorBaudRate**: 115200 (for GPS/debug output)

### extensions.json
Recommends the PlatformIO IDE extension for this project.

## Switching Between Boards

### Method 1: Edit settings.json
Change the `platformio-ide.defaultEnvironment` value:
```json
{
    "platformio-ide.defaultEnvironment": "genericSTM32F405RG"
}
```

### Method 2: Use PlatformIO UI
Click the environment name in the bottom blue toolbar in VS Code and select your target board.

### Method 3: Command Line
Use the `-e` flag with PlatformIO commands:
```bash
pio run -e genericSTM32F401CB
pio run -e genericSTM32F405RG
```

## More Information

See [DOWNLOAD_SETUP.md](../DOWNLOAD_SETUP.md) for complete setup instructions.
