# Autonomous Buoy Firmware - STM32F401/F405

Complete firmware implementation for an autonomous buoy system with GPS navigation, compass-based heading control, water current estimation, and manual override.

**🎯 NEW**: This project now supports **two STM32 board options**:
- **STM32F401CB Black Pill** (84 MHz, 128KB Flash) - Recommended
- **STM32F405RG** (168 MHz, 1024KB Flash) - For advanced applications

📥 **[Download and Setup Guide](DOWNLOAD_SETUP.md)** - Complete instructions for downloading to your PC and building both projects

## Project Structure

### Configuration
- **[config.h](include/config.h)** - Centralized configuration file with all pin definitions, constants, and tunable parameters

### Core Modules

#### GPS Management
- **[gps.h](include/gps.h)** / [gps.cpp](src/gps.cpp) - TinyGPS++ wrapper for easy GPS data access
  - Valid fix detection
  - HDOP quality monitoring
  - Distance and bearing calculations to target

#### Compass Heading
- **[compass.h](include/compass.h)** / [compass.cpp](src/compass.cpp) - HMC5883 magnetometer interface
  - Hard-iron and soft-iron calibration
  - Magnetic declination compensation
  - Heading normalization (0-360°)

#### Receiver Input
- **[receiver.h](include/receiver.h)** / [receiver.cpp](src/receiver.cpp) - PPM signal decoding
  - 8-channel PPM support
  - FRIA6B receiver interface
  - Signal validity monitoring
  - Per-channel access (throttle, steering, mode, water condition selector)

#### Motor Control
- **[motor_controller.h](include/motor_controller.h)** / [motor_controller.cpp](src/motor_controller.cpp) - ESC management
  - Port and starboard motor control
  - Mixed throttle + yaw commands
  - Safe neutral point handling

#### Control System
- **[control_system.h](include/control_system.h)** / [control_system.cpp](src/control_system.cpp) - Core logic
  - Mode switching (autonomous/manual)
  - Water condition adaptation (calm/low/strong current)
  - Target position management
  - Water current estimation state
  - Heading error calculation with wraparound

#### Telemetry
- **[telemetry.h](include/telemetry.h)** / [telemetry.cpp](src/telemetry.cpp) - SIK radio protocol
  - Sends position, heading, HDOP, current estimation to ground station
  - 1 Hz telemetry rate (configurable)

#### Main Application
- **[main.cpp](src/main.cpp)** - Application loop orchestration
  - System initialization
  - Sensor update sequencing
  - Autonomous hold-position logic
  - Manual control input processing
  - Water current drift estimation
  - GPS LED status indication

## Operating Modes

### Autonomous Mode (Default)
- **Objective**: Navigate to and hold a target position
- **Operation**:
  1. GPS fix obtained at startup becomes initial target
  2. Buoy navigates toward target using compass-based steering
  3. Distance-based throttle control with dynamic gain tuning
  4. Water current automatically estimated while stationary
  5. Current vector used to adjust desired heading when strong

### Manual Mode
- **Objective**: Direct pilot control via RC transmitter
- **Operation**:
  1. CH3 (Throttle) controls forward/backward speed
  2. CH1 (Steering) controls port/starboard yaw
  3. Motors respond directly to stick inputs
  4. No GPS or compass feedback

## Water Current Adaptation

Three preset configurations adapt system behavior to water conditions:

| Condition | Trigger | Hold Radius | Distance Gain | Yaw Gain | Max Thrust |
|-----------|---------|-------------|---------------|----------|------------|
| Calm Waters | CH6 < 1300µs | 2.0m | 1.5 | 1.4 | 300 |
| Low Current | 1300-1700µs | 3.0m | 1.8 | 1.2 | 360 |
| Strong Current | CH6 > 1700µs | 3.5m | 2.0 | 1.0 | 420 |

## RC Channel Mapping

| Channel | Function | Range | Notes |
|---------|----------|-------|-------|
| CH1 | Steering/Yaw | 1000-2000µs | -500 to +500 mapped to motors |
| CH3 | Throttle | 1000-2000µs | Ignored in autonomous mode |
| CH5 | Mode Switch | >1500µs = Manual, <1500µs = Autonomous | 2-position switch |
| CH6 | Current Type | <1300µs = Calm, 1300-1700µs = Low, >1700µs = Strong | 3-position switch |

## GPS LED Status

- **Steady ON (Low)**: Valid fix with HDOP < 1.0
- **Blinking (0.5 Hz)**: Fix obtained but HDOP >= 1.0
- **OFF (High)**: No GPS fix

## Telemetry Format

Simple comma-separated format sent to ground station:

```
$TELEM,<current_lat>,<current_lon>,<target_lat>,<target_lon>,<heading>,<hdop>,<distance_m>,<current_m_s>,<current_dir>,<gps_fixed>,<mode>,<water_condition>
```

Example:
```
$TELEM,48.85339,2.34880,48.85350,2.34890,185.5,0.85,15.3,0.25,270.0,1,0,1
```

## Calibration Values

Magnetometer calibration stored in [config.h](include/config.h):

```cpp
#define MAG_OFFSET_X      -12.4   // Hard-iron offset (device-specific)
#define MAG_OFFSET_Y      8.9     // Hard-iron offset (device-specific)
#define MAG_SCALE_X       1.08    // Soft-iron scale (device-specific)
#define MAG_SCALE_Y       0.94    // Soft-iron scale (device-specific)
#define MAG_DECLINATION   2.5     // Paris area (adjust for your location)
```

Run magnetometer calibration routine before field deployment.

## Building & Uploading

### Prerequisites
- PlatformIO CLI or VS Code + PlatformIO extension
- ST-Link v2 debugger connected

### Supported Boards
This project supports two STM32 board variants:

1. **STM32F401CB Black Pill** (Default)
   - 84 MHz CPU, 128KB Flash, 64KB RAM
   - Recommended for standard deployments
   - Environment: `genericSTM32F401CB`

2. **STM32F405RG**
   - 168 MHz CPU, 1024KB Flash, 192KB RAM
   - Higher performance with more resources
   - Environment: `genericSTM32F405RG`

### Build

For **STM32F401CB** (default):
```bash
platformio run -e genericSTM32F401CB
```

For **STM32F405RG**:
```bash
platformio run -e genericSTM32F405RG
```

Or build both environments:
```bash
platformio run
```

### Upload

For **STM32F401CB**:
```bash
platformio run -t upload -e genericSTM32F401CB
```

For **STM32F405RG**:
```bash
platformio run -t upload -e genericSTM32F405RG
```

### Monitor Serial Output
```bash
platformio device monitor -b 115200
```

## System Limitations & Notes

- **PPM Only**: FRIA6B receiver on PPM protocol (8 channels max)
- **Single Compass**: No gyroscope integration (use only when moving)
- **Open-Loop Current Estimation**: Assumes zero initial drift; error accumulates
- **Serial Rate**: GPS at 115200 baud for fast L80-R module
- **No IMU**: Heading relies solely on magnetometer (compass only)
- **Blocking Startup**: System blocks in setup() until GPS fix acquired

## Future Enhancements

1. **Gyroscope Integration**: ICM42688P IMU for heading hold between compass updates
2. **Kalman Filter**: Sensor fusion for more robust heading estimation
3. **Closed-Loop Current Estimation**: Compare GPS drift with dead-reckoning
4. **OSD Support**: AT7456E on-screen display integration for visual feedback
5. **Multiple Waypoints**: Autonomous mission planning with waypoint sequencing
6. **Radio Failsafe**: Auto-return-to-home on signal loss
7. **Logging**: SD card data logging for post-mission analysis

## Wiring Reference

### STM32F401 Pins (Black Pill)
- **PA0** - PPM receiver input
- **PA9/PA10** - USART1 GPS (TX/RX)
- **PB10/PB11** - USART3 Telemetry (TX/RX)
- **PB6** - PWM ESC port motor
- **PB7** - PWM ESC starboard motor
- **PC13** - GPIO LED GPS status (inverted)
- **PB8/PB9** - I2C1 compass (SDA/SCL)

### External Connections
- **GPS**: L80-R on USART1 (115200 baud)
- **Compass**: HMC5883 on I2C1
- **Receiver**: FRIA6B PPM output on PA0
- **ESCs**: BLDCs on PB6, PB7 (1000-2000µs pulse)
- **Telemetry**: SIK radio modem on USART3 (57600 baud)

## Safety Warnings

⚠️ **Before powering on:**
1. Remove propellers from motors
2. Verify all motor connections
3. Test RC signal integrity
4. Ensure GPS has outdoor fix
5. Calibrate compass in field location
6. Check battery polarity and voltage (11.1V for 3S LiPo)

## License

This firmware is provided as-is for autonomous buoy development.
