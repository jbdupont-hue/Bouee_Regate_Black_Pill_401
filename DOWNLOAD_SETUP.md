# Download and Setup Guide - Two Board Options

This repository contains firmware for autonomous buoy systems that can be built for **two different STM32 boards**. This guide will help you download and set up the project in your Visual Studio Code / PlatformIO environment.

## Supported Hardware Options

### Option 1: STM32F401CB Black Pill (Recommended for most users)
- **CPU**: ARM Cortex-M4 @ 84 MHz
- **Flash Memory**: 128KB
- **RAM**: 64KB
- **Environment Name**: `genericSTM32F401CB`
- **Best for**: Standard autonomous buoy deployments

### Option 2: STM32F405RG (For advanced users)
- **CPU**: ARM Cortex-M4 @ 168 MHz
- **Flash Memory**: 1024KB (1MB)
- **RAM**: 192KB
- **Environment Name**: `genericSTM32F405RG`
- **Best for**: Applications requiring more resources, faster processing, or future expansion

## Prerequisites

### Required Software
1. **Visual Studio Code** - Download from https://code.visualstudio.com/
2. **PlatformIO IDE Extension** - Install from VS Code Extensions Marketplace
3. **Git** - For cloning the repository (optional, can also download ZIP)

### Required Hardware
- STM32F401CB Black Pill **OR** STM32F405RG board
- ST-Link V2 programmer/debugger
- USB cable for ST-Link connection
- Additional hardware as per project requirements (GPS, compass, motors, etc.)

## Step-by-Step Setup

### Step 1: Clone or Download the Repository

#### Option A: Using Git (Recommended)
```bash
git clone https://github.com/jbdupont-hue/Bouee_Regate_Black_Pill_401.git
cd Bouee_Regate_Black_Pill_401
```

#### Option B: Download ZIP
1. Go to https://github.com/jbdupont-hue/Bouee_Regate_Black_Pill_401
2. Click the green "Code" button
3. Select "Download ZIP"
4. Extract the ZIP file to your preferred location

### Step 2: Open in Visual Studio Code

1. Launch **Visual Studio Code**
2. Click **File → Open Folder**
3. Navigate to and select the `Bouee_Regate_Black_Pill_401` folder
4. Click **Select Folder**

### Step 3: Install PlatformIO Extension (if not already installed)

1. Click on the **Extensions** icon in the left sidebar (or press `Ctrl+Shift+X`)
2. Search for **"PlatformIO IDE"**
3. Click **Install**
4. Wait for the installation to complete (may take a few minutes)
5. Restart VS Code when prompted

### Step 4: Let PlatformIO Initialize

When you first open the project:
1. PlatformIO will automatically detect the `platformio.ini` file
2. It will download the required platform packages and libraries
3. This may take 5-10 minutes depending on your internet connection
4. You'll see progress in the PlatformIO terminal at the bottom

### Step 5: Select Your Target Environment

The project supports two environments. You can select which one to use:

#### In VS Code with PlatformIO:
1. Look at the **bottom toolbar** (blue bar)
2. Click on the **environment selector** (shows current environment)
3. Choose your target:
   - **`genericSTM32F401CB`** - for STM32F401CB Black Pill
   - **`genericSTM32F405RG`** - for STM32F405RG

#### Or edit `.vscode/settings.json`:
```json
{
    "platformio-ide.defaultEnvironment": "genericSTM32F401CB"
    // Change to "genericSTM32F405RG" for the other board
}
```

## Building the Firmware

### Using VS Code PlatformIO GUI:
1. Click the **checkmark** icon (✓) in the bottom toolbar to build
2. Or click the **PlatformIO** icon in the left sidebar
3. Expand **PROJECT TASKS**
4. Select your environment (`genericSTM32F401CB` or `genericSTM32F405RG`)
5. Click **Build**

### Using Command Line:

For **STM32F401CB**:
```bash
platformio run -e genericSTM32F401CB
```

For **STM32F405RG**:
```bash
platformio run -e genericSTM32F405RG
```

To build both environments:
```bash
platformio run
```

## Uploading to Your Board

### Prerequisites:
- Connect your STM32 board to ST-Link V2
- Connect ST-Link to your computer via USB
- Ensure proper wiring:
  - ST-Link SWDIO → Board SWDIO
  - ST-Link SWCLK → Board SWCLK
  - ST-Link GND → Board GND
  - ST-Link 3.3V → Board 3.3V (optional, if board is not powered separately)

### Using VS Code PlatformIO GUI:
1. Click the **arrow** icon (→) in the bottom toolbar to upload
2. Or navigate to **PlatformIO → PROJECT TASKS → [your environment] → Upload**

### Using Command Line:

For **STM32F401CB**:
```bash
platformio run -t upload -e genericSTM32F401CB
```

For **STM32F405RG**:
```bash
platformio run -t upload -e genericSTM32F405RG
```

## Monitoring Serial Output

### Using VS Code PlatformIO:
1. Click the **plug** icon in the bottom toolbar
2. Or navigate to **PlatformIO → PROJECT TASKS → [your environment] → Monitor**

### Using Command Line:
```bash
platformio device monitor -b 115200
```

Press `Ctrl+C` to exit the monitor.

## Project Structure

```
Bouee_Regate_Black_Pill_401/
├── .vscode/                  # VS Code configuration
│   ├── extensions.json       # Recommended extensions
│   └── settings.json         # PlatformIO settings
├── include/                  # Header files
│   ├── config.h             # Main configuration (EDIT HERE FOR YOUR SETUP)
│   ├── gps.h
│   ├── compass.h
│   ├── receiver.h
│   ├── motor_controller.h
│   ├── control_system.h
│   └── telemetry.h
├── src/                      # Source files
│   ├── main.cpp             # Main application
│   └── [other .cpp files]
├── lib/                      # Custom libraries (if any)
├── test/                     # Unit tests
├── platformio.ini           # PlatformIO configuration (DEFINES BOTH ENVIRONMENTS)
├── README.md                # Complete project documentation
├── QUICKSTART.md            # Quick start guide
├── START_HERE.md            # Getting started guide
└── DOWNLOAD_SETUP.md        # This file
```

## Switching Between Environments

You can easily switch between STM32F401CB and STM32F405RG:

### Method 1: Using PlatformIO Toolbar
- Click the environment selector in the bottom blue bar
- Choose the desired environment

### Method 2: Command Line Argument
- Just specify the environment with `-e` flag:
  ```bash
  pio run -e genericSTM32F401CB    # For F401
  pio run -e genericSTM32F405RG    # For F405
  ```

### Method 3: Edit settings.json
- Open `.vscode/settings.json`
- Change the `platformio-ide.defaultEnvironment` value

## Customizing for Your Hardware

### 1. Update Pin Definitions
Edit `include/config.h` to match your hardware connections:
```cpp
// Pin definitions
#define PPM_PIN           PA0
#define ESC_PORT_PIN      PB6
#define ESC_STARBOARD_PIN PB7
// ... etc
```

### 2. Calibrate Sensors
Update compass calibration values in `include/config.h`:
```cpp
#define MAG_OFFSET_X      -12.4
#define MAG_OFFSET_Y      8.9
#define MAG_DECLINATION   2.5
```

### 3. Tune Control Parameters
Adjust control gains based on your buoy's behavior:
```cpp
#define DIST_GAIN_CALM    1.5
#define YAW_GAIN_CALM     1.4
#define MAX_THRUST_CALM   300
```

## Common Issues and Solutions

### Issue: "PlatformIO: command not found"
**Solution**: Install PlatformIO extension in VS Code or install PlatformIO Core CLI

### Issue: Platform installation fails
**Solution**: 
- Check your internet connection
- Try running `pio platform install ststm32` manually
- Clear PlatformIO cache: `pio system prune`

### Issue: Upload fails - "ST-Link not found"
**Solution**:
- Verify ST-Link is properly connected
- Check USB cable and port
- Install ST-Link drivers (Windows)
- Try different USB port

### Issue: Build errors about missing libraries
**Solution**:
- Let PlatformIO finish initialization first
- Manually install: `pio lib install`
- Check `platformio.ini` for correct library dependencies

### Issue: Code compiles but doesn't work as expected
**Solution**:
- Verify you selected the correct environment for your board
- Check `include/config.h` for proper pin assignments
- Review sensor wiring and connections

## Next Steps

After successful setup:

1. **Read the Documentation**
   - [START_HERE.md](START_HERE.md) - Overview and quick reference
   - [README.md](README.md) - Complete technical documentation
   - [QUICKSTART.md](QUICKSTART.md) - Fast setup guide

2. **Configure Your Hardware**
   - Edit `include/config.h` with your pin assignments
   - Calibrate sensors (compass, GPS)

3. **Build and Test**
   - Build for your target board
   - Upload firmware
   - Test with serial monitor

4. **Deploy**
   - Follow safety procedures
   - Test in controlled environment first
   - Deploy to water

## Support and Documentation

- **Technical Reference**: [README.md](README.md)
- **Architecture Details**: [ARCHITECTURE.md](ARCHITECTURE.md)
- **Code Summary**: [CODE_SUMMARY.md](CODE_SUMMARY.md)
- **Project Completion**: [PROJECT_COMPLETE.md](PROJECT_COMPLETE.md)

## Summary

You now have access to **two complete PlatformIO projects** in one repository:
- ✅ **STM32F401CB** environment (`genericSTM32F401CB`)
- ✅ **STM32F405RG** environment (`genericSTM32F405RG`)

Both share the same source code and can be built/uploaded by simply selecting the appropriate environment in PlatformIO.

**Happy coding! 🚀**
