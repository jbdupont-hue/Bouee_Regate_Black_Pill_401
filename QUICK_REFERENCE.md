# Quick Reference - Two Board Options

## Available Environments

| Board | Environment Name | CPU | Flash | RAM | Use Case |
|-------|-----------------|-----|-------|-----|----------|
| **STM32F401CB Black Pill** | `genericSTM32F401CB` | 84 MHz | 128 KB | 64 KB | Standard deployments (recommended) |
| **STM32F405RG** | `genericSTM32F405RG` | 168 MHz | 1024 KB | 192 KB | Advanced applications, more features |

## Quick Commands

### Building

```bash
# Build for STM32F401CB (default)
platformio run -e genericSTM32F401CB

# Build for STM32F405RG
platformio run -e genericSTM32F405RG

# Build both
platformio run
```

### Uploading

```bash
# Upload to STM32F401CB
platformio run -t upload -e genericSTM32F401CB

# Upload to STM32F405RG
platformio run -t upload -e genericSTM32F405RG
```

### Monitoring

```bash
# Open serial monitor (115200 baud)
platformio device monitor -b 115200
```

### Cleaning

```bash
# Clean build for specific environment
platformio run -t clean -e genericSTM32F401CB

# Clean all
platformio run -t clean
```

## VS Code PlatformIO

### Switching Environments
1. Look at the **bottom blue toolbar**
2. Click on the **environment name** (current env shown)
3. Select: `genericSTM32F401CB` or `genericSTM32F405RG`

### Quick Actions (Bottom Toolbar)
- ✓ **Checkmark** = Build
- → **Arrow** = Upload
- 🔌 **Plug** = Serial Monitor
- 🗑️ **Trash** = Clean

## Where to Start

1. **First time?** → [DOWNLOAD_SETUP.md](DOWNLOAD_SETUP.md) - Complete setup guide
2. **Quick setup?** → [QUICKSTART.md](QUICKSTART.md) - Fast start
3. **Full docs?** → [README.md](README.md) - Complete reference
4. **Overview?** → [INDEX.md](INDEX.md) - Navigation guide

## Configuration Files

- **platformio.ini** - Defines both environments, shared libraries
- **.vscode/settings.json** - VS Code default environment
- **include/config.h** - Hardware pins, calibration, tuning parameters

## Both Projects Share

✅ Same source code (`src/*.cpp`)  
✅ Same header files (`include/*.h`)  
✅ Same libraries (GPS, compass, sensors)  
✅ Same pin definitions (configurable)  
✅ Same functionality

The only difference is the target board/microcontroller!

## Need Help?

| Issue | Solution Document |
|-------|------------------|
| Can't download/setup | [DOWNLOAD_SETUP.md](DOWNLOAD_SETUP.md) |
| Don't know what to do | [START_HERE.md](START_HERE.md) |
| Hardware connections | [README.md](README.md) - Wiring section |
| Build errors | [DOWNLOAD_SETUP.md](DOWNLOAD_SETUP.md) - Troubleshooting |
| Code questions | [CODE_SUMMARY.md](CODE_SUMMARY.md) |
