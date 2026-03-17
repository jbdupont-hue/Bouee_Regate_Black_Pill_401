# Quick Start Guide - Autonomous Buoy Firmware

## What Was Created?

A complete, modular firmware for an STM32F401 autonomous buoy with:
- ✅ **Autonomous Navigation**: GPS-based position holding with compass steering
- ✅ **Manual Control**: Direct RC transmitter override
- ✅ **Water Current Estimation**: Automatic drift detection and compensation
- ✅ **Telemetry**: Real-time position/status transmission
- ✅ **Adaptive Control**: Different tuning for calm/low/strong currents

## File Overview

```
Bouee_Regate_Black_Pill_401_IST3810/
├── include/
│   ├── config.h              ← TUNE HERE: pins, gains, thresholds
│   ├── gps.h                 ← GPS interface (TinyGPS++)
│   ├── compass.h             ← Compass interface (IST8310)
│   ├── receiver.h            ← PPM receiver (FRIA6B)
│   ├── motor_controller.h    ← ESC control (dual motors)
│   ├── control_system.h      ← Core navigation logic
│   └── telemetry.h           ← SIK radio transmission
├── src/
│   ├── main.cpp              ← Application entry point
│   ├── gps.cpp               ← GPS implementation
│   ├── compass.cpp           ← Compass implementation
│   ├── receiver.cpp          ← PPM receiver implementation
│   ├── motor_controller.cpp  ← Motor control implementation
│   ├── control_system.cpp    ← Navigation control implementation
│   └── telemetry.cpp         ← Telemetry implementation
├── README.md                 ← Complete documentation
├── CODE_SUMMARY.md          ← Architecture & implementation details
└── QUICKSTART.md            ← This file
```

## 60-Second Setup

### 1. Connect Hardware
```
ST-Link V2 → STM32F401 (Black Pill)
GPS (L80-R) → USART1 (PA9/PA10)
Compass (IST8310) → I2C1 (PB8/PB9)
Receiver (FRIA6B) → PPM Input (PA0)
Motors (ESC) → PWM Outputs (PB6, PB7)
Telemetry (SIK) → USART3 (optional, or USART2)
```

### 2. Build
```bash
platformio run -e genericSTM32F401CB
```

### 3. Upload
```bash
platformio run -t upload -e genericSTM32F401CB
```

### 4. Monitor
```bash
platformio device monitor -b 115200
```

You should see:
```
=== Autonomous Buoy Starting ===
All sensors initialized successfully
Waiting for GPS fix...
GPS Fix obtained! System ready.
```

## How It Works

### Startup Sequence
1. Initialize all sensors (GPS, compass, receiver, motors)
2. Wait for GPS fix (LED blinks during wait)
3. Store startup GPS location as target position
4. Ready for operation

### Autonomous Mode (Default)
```
Power On
  ↓
Get GPS Fix
  ↓
Navigate to Target Position
  ├─ Compass determines heading
  ├─ GPS determines distance
  ├─ Motor speed = distance × gain
  ├─ Motor steering = heading error × gain
  └─ Repeat until within 2-3 meters
  ↓
Hold Position
  ├─ Motors stop when target reached
  ├─ Track drift due to water current
  ├─ Estimate current direction/strength
  └─ Use current estimate to counter-steer in future navigation
```

### Manual Mode (RC Switch Up)
```
Pilot Moves Stick
  ↓
Receiver decodes PPM
  ↓
Motor Throttle ← RC Throttle (CH3)
Motor Steering ← RC Steering (CH1)
  ↓
Buoy moves directly with stick inputs
(No GPS or compass feedback)
```

## LED Status Meanings

| LED State | Meaning | Action |
|-----------|---------|--------|
| **Steady ON** | GPS fix, good quality | System ready ✓ |
| **Blinking** | GPS fix, poor quality | Wait for fix improvement |
| **OFF** | No GPS signal | Move outdoors, wait 2-3 min |

## RC Transmitter Setup

### Channel Mapping
- **CH1** (Stick Left/Right) → Steering (port/starboard)
- **CH3** (Stick Up/Down) → Throttle (forward/backward)
- **CH5** (Switch 2-pos) → Mode: DOWN=Autonomous, UP=Manual
- **CH6** (Switch 3-pos) → Water: DOWN=Calm, MID=Low, UP=Strong

### Recommended Switches
- CH5: Standard 2-position switch
  - Down position = autonomous (default)
  - Up position = manual control
- CH6: 3-position switch
  - Down = calm waters (2m holding radius, 1.5x gain)
  - Mid = low current (3m holding radius, 1.8x gain)
  - Up = strong current (3.5m holding radius, 2.0x gain)

## Field Testing Procedure

### Test 1: Receiver Check
1. Power on system with propellers **OFF**
2. Open serial monitor
3. Move RC sticks
4. Verify PPM values decoded correctly (should see 1000-2000 range)

### Test 2: Motor Check
1. Switch to **manual mode** (CH5 up)
2. Move throttle stick slowly
3. Both motors should spin together (forward direction)
4. Move steering stick left/right
5. Starboard motor speed should increase when steering left (port takes priority)

### Test 3: Compass Check
1. While holding buoy level
2. Rotate it slowly (like compass rose)
3. Watch serial output
4. Heading should smoothly change 0→360°

### Test 4: GPS Check
1. In open area without overhead obstructions
2. Wait for GPS LED to turn solid (usually <2 min)
3. Check serial output for lat/lon values
4. Verify position updates every 1 second

### Test 5: Autonomous Navigation (Dry Run)
1. Place buoy on table
2. Switch to autonomous mode (CH5 down)
3. Buoy will try to return to startup position (should show 0m distance)
4. Switch back to manual mode

## Tuning Guide

### On-Water Test Matrix (3 Parameter Sets)

Use this sequence in order (A → B → C). For each set:
1. Edit values in `include/config.h`
2. Build/upload firmware
3. Run **2 tests**:
  - **Manual straight test**: 20-30 m forward with steering stick centered
  - **Autonomous hold test**: hold position for 2-3 min and watch for spinning

Record:
- Left/right drift in manual mode (none / slight / strong)
- Spin tendency at hold point (none / occasional / continuous)
- Motor twitching near target (low / medium / high)

#### Set A - Baseline Stable
```cpp
#define MANUAL_STEER_TRIM_US 0
#define MANUAL_GYRO_STRAIGHT_GAIN 1.6f
#define MANUAL_STRAIGHT_MAX_YAW_CORR 70
#define MANUAL_STRAIGHT_MIN_THROTTLE 40

#define AUTO_HOLD_HEADING_DEADBAND_DEG 8.0f
#define AUTO_HOLD_NEAR_YAW_LIMIT 35.0f
#define YAW_RATE_GAIN_CALM 1.0f
```

#### Set B - Straight-Line Priority (Manual)
```cpp
#define MANUAL_STEER_TRIM_US 10
#define MANUAL_GYRO_STRAIGHT_GAIN 2.1f
#define MANUAL_STRAIGHT_MAX_YAW_CORR 110
#define MANUAL_STRAIGHT_MIN_THROTTLE 30

#define AUTO_HOLD_HEADING_DEADBAND_DEG 8.0f
#define AUTO_HOLD_NEAR_YAW_LIMIT 35.0f
#define YAW_RATE_GAIN_CALM 1.0f
```

#### Set C - Anti-Spin Priority (Autonomous)
```cpp
#define MANUAL_STEER_TRIM_US 10
#define MANUAL_GYRO_STRAIGHT_GAIN 1.8f
#define MANUAL_STRAIGHT_MAX_YAW_CORR 90
#define MANUAL_STRAIGHT_MIN_THROTTLE 35

#define AUTO_HOLD_HEADING_DEADBAND_DEG 10.0f
#define AUTO_HOLD_NEAR_YAW_LIMIT 25.0f
#define YAW_RATE_GAIN_CALM 1.3f
```

Acceptance criteria:
- Keep the first set that gives both: (1) manual centered-stick run with only slight drift, and (2) autonomous hold without persistent spin for at least 2 minutes.
- If drift direction is always the same, keep the best set and fine-trim only `MANUAL_STEER_TRIM_US` by ±5 until centered.

### If Buoy Overshoots Target
**Problem**: Buoy oscillates around target  
**Solution**: Reduce gains in `config.h`
```cpp
#define DIST_GAIN_CALM 1.2  // was 1.5 (less speed gain)
#define YAW_GAIN_CALM 0.9   // was 1.4 (less aggressive steering)
```

### If Buoy Struggles in Current
**Problem**: Can't hold position, drifts away  
**Solution**: Increase max thrust
```cpp
#define MAX_THRUST_STRONG_CURRENT 500  // was 420
```

### If Heading Drifts
**Problem**: Compass heading slowly rotates  
**Solution**: Run compass calibration
- Move buoy in 8-figure pattern
- Record hard-iron offsets from calibration routine
- Update MAG_OFFSET_X, MAG_OFFSET_Y in config.h

### If Motors Don't Stop Smoothly
**Problem**: Vibration when near target  
**Solution**: Increase holding radius
```cpp
#define HOLD_RADIUS_CALM 3.0  // was 2.0 (give more stopping distance)
```

## Telemetry Output Format

The system sends telemetry at 1 Hz to serial port USART3 (or USB):

```
$TELEM,<current_lat>,<current_lon>,<target_lat>,<target_lon>,<heading>,<hdop>,<distance_m>,<current_m_s>,<current_dir>,<gps_fixed>,<mode>,<water_condition>
```

Example:
```
$TELEM,48.85339,2.34880,48.85350,2.34890,185.5,0.85,15.3,0.25,270.0,1,0,1
```

**Parsing**:
- Lat/Lon: ± degrees decimal
- Heading: 0-360° (compass bearing)
- HDOP: position accuracy (lower is better, <1.0 is good)
- Distance: meters to target
- Current: m/s drift speed
- GPS Fixed: 1=yes, 0=no
- Mode: 0=autonomous, 1=manual
- Water: 0=calm, 1=low, 2=strong

## Troubleshooting

| Problem | Check | Fix |
|---------|-------|-----|
| Firmware won't upload | ST-Link connected? | Verify SWDIO/SWCLK wiring |
| GPS never fixes | Outdoors? 30+ satellites? | Move to clear sky area, wait 5 min |
| Motors don't move | ESC armed? PWM values 1000-2000? | Check receiver.cpp isSignalValid() |
| Compass reads wrong heading | Calibrated for this location? | Run compass 8-figure calibration |
| Telemetry not received | USART3 connected to modem? | Check serial port assignment |
| Buoy drifts away in autonomous | Current condition set correctly? | Test with low/strong current presets |

## Next Steps

1. **Update config.h** with your calibration values (compass offsets)
2. **Build & upload** the firmware
3. **Test on land** (all sensors responding)
4. **Pool test** (shallow water, propellers on)
5. **Field test** (at regatta location, full deployment)
6. **Monitor telemetry** (verify position/heading on ground station)
7. **Tune gains** based on observed behavior

## Safety Checklist Before Each Test

- [ ] **Propellers removed** (testing only with props off initially)
- [ ] **Battery voltage checked** (11.1V nominal for 3S LiPo)
- [ ] **All connections tight** (GPS, compass, receiver, motors)
- [ ] **RC transmitter paired** and functioning
- [ ] **ST-Link disconnected** (don't run with debug connection)
- [ ] **Area clear** (no people/animals near buoy)
- [ ] **Kill switch ready** (immediate motor stop if needed)

## Firmware Capabilities Summary

| Feature | Status | Notes |
|---------|--------|-------|
| GPS Navigation | ✅ | L80-R GNSS module, 115200 baud |
| Compass Heading | ✅ | IST8310, calibrated for location |
| PPM Receiver | ✅ | 8 channels from FRIA6B receiver |
| Dual Motor Control | ✅ | Port/starboard differential mixing |
| Autonomous Hold | ✅ | GPS position maintenance, current compensation |
| Manual Control | ✅ | Direct RC stick → motor mapping |
| Water Current Est. | ✅ | Drift measurement while stationary |
| Telemetry | ✅ | SIK radio protocol, 1 Hz update |
| LED Status | ✅ | GPS fix quality indication |
| Mode Switching | ✅ | 2-position switch autonomous/manual |
| Condition Adaptation | ✅ | 3-position switch: calm/low/strong |

---

**Questions?** Check [README.md](README.md) for complete documentation or [CODE_SUMMARY.md](CODE_SUMMARY.md) for architecture details.

**Ready to deploy!** 🚀