# System Architecture Diagram

## Hardware Block Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     STM32F401 (Black Pill)                      │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    MAIN PROCESSOR                        │   │
│  │                                                          │   │
│  │  PA0 ───────┐                                ┌───── PB6  │   │
│  │             │                                │           │   │
│  │  PA9/PA10 ──┤ USB Debug ┌─────────────┐      │ ESC Port  │   │
│  │  (USART1)   │  Serial ──│  Serial     │      └───────●   │   │
│  │             │  115200   │  Console    │             │    │   │
│  │  PB10/11  ──┤           │  (Optional) │      PB7 ───┤    │   │
│  │  (USART3)   │           └─────────────┘             │    │   │
│  │             │           MAIN LOOP             ESC Starb  │   │
│  │  PB8/9 ─────┤           20 Hz (50ms)           |          │   │
│  │  (I2C1)     │                                  │         │   │
│  │             │           ┌──────────────┐       └───● PB7 │   │
│  │  PC13 ──-───┤           │   Core Logic │                 │   │
│  │  (GPIO)     │           │   Control    │   ┌──---────┘   │   │
│  │             │           │   System     │   │             │   │
│  │             └───────────│              │───┘             │   │
│  │                         └──────────────┘                 │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
   │              │                │              │         │
   │              │                │              │         │
   ▼              ▼                ▼              ▼         ▼
┌────┐       ┌─────────┐      ┌─────────┐   ┌─────┐    ┌─────┐
│ RC │       │   GPS   │      │ Compass │   │ESC-P│    │ESC-S│
│RX  │       │ L80-R   │      │IST8310  │   │1000-│    │1000-│
│PPM │       │GNSS     │      │I2C      │   │2000 │    │2000 │
└────┘       └─────────┘      │ ┌──────┐│   │ µs  │    │ µs  │
             115200 baud      │ │ CAL. ││   │     │    │     │
                              │ │ 9DOF ││   └─────┘    └─────┘
                              └─┴──────┘┘       │          │
                                              ▼          ▼
                                           ┌─────────────────┐
                                           │  Brushless      │
                                           │  Motor Port     │
                                           │  (CW)           │
                                           └─────────────────┘
                                               
                                           ┌─────────────────┐
                                           │  Brushless      │
                                           │  Motor Starb    │
                                           │  (CCW)          │
                                           └─────────────────┘

Optional Telemetry:
┌──────────────┐
│ SIK Radio    │
│ Modem 57600  │
│ (UART2/3)    │
└──────────────┘
```

## Software Module Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    MAIN APPLICATION                         │
│                      (main.cpp)                             │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  void setup()                                       │    │
│  │    • Initialize all sensors                         │    │
│  │    • Wait for GPS fix                               │    │
│  │    • Set initial target position                    │    │
│  └─────────────────────────────────────────────────────┘    │
│                            ▼                                │
│  ┌─────────────────────────────────────────────────────┐    │
│  │  void loop() [20 Hz Main Loop]                      │    │
│  │    • Update all sensors (GPS, compass)              │    │
│  │    • Process RC receiver input                      │    │
│  │    • Estimate water current (if autonomous)         │    │
│  │    • Execute control (manual or autonomous)         │    │
│  │    • Send telemetry (1 Hz)                          │    │
│  │    • Delay 50ms                                     │    │
│  └─────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────┘
   │         │              │               │            │
   │         │              │               │            │
   ▼         ▼              ▼               ▼            ▼
┌────────┐ ┌──────────┐ ┌─────────────┐ ┌────────┐ ┌──────────┐
│ GPS    │ │ Compass  │ │  Receiver   │ │Motor   │ │Control   │
│Manager │ │Manager   │ │  PPM        │ │Control │ │System    │
│        │ │          │ │             │ │        │ │          │
│Methods │ │Methods   │ │Methods      │ │Methods │ │Methods   │
│        │ │          │ │             │ │        │ │          │
│Update()│ │Read      │ │GetChannel() │ │Drive() │ │SetMode() │
│Has Fix │ │Heading() │ │GetThrottle()│ │Stop()  │ │SetWater()│
│Get Lat/│ │Calibrate │ │GetSteer()   │ │        │ │GetParams │
│Lon     │ │          │ │GetMode()    │ │        │ │          │
│Get HDOP│ │          │ │GetCurrentTy │ │        │ │HeadingErr│
│Get Dist│ │          │ │IsValid()    │ │        │ │          │
│Get Bear│ │          │ │             │ │        │ │Current   │
└────────┘ └──────────┘ └─────────────┘ └────────┘ │Estimate  │
   │          │             │              │       │          │
   │          │             │              │       └──────────┘
   │          │             │              │            │
   └──────────┴─────────────┴──────────────┴────────────┘
                      │
                      ▼
            ┌──────────────────┐
            │ Telemetry        │
            │ Module           │
            │                  │
            │ SendTelemetry()  │
            │ SendDebug()      │
            └──────────────────┘
```

## Control Flow - Autonomous Mode

```
┌─────────────────────────────────────────────────────┐
│ AUTONOMOUS MODE CONTROL FLOW                        │
└─────────────────────────────────────────────────────┘

GPS Locked?
    │
    ├─ NO  → Stop Motors → Return
    │
    └─ YES
       │
       ├─ Get Current Position (Lat, Lon)
       │
       ├─ Calculate Distance to Target
       │        │
       │        ├─ Within Hold Radius?
       │        │    │
       │        │    ├─ YES → Stop Motors
       │        │    │        │
       │        │    │        └─ Estimate Water Current (Drift)
       │        │    │
       │        │    └─ NO → Continue to target
       │
       ├─ Get Bearing to Target (GPS)
       │
       ├─ Get Current Heading (Compass)
       │
       ├─ Apply Current Compensation?
       │    │
       │    ├─ Current Valid & Strong (>3.0 m/s)?
       │    │    │
       │    │    ├─ YES → Adjust desired bearing upstream
       │    │    │
       │    │    └─ NO → Use direct bearing
       │
       ├─ Calculate Heading Error (Wraparound ±180°)
       │
       ├─ Apply Deadband (if |error| < 5°, set to 0)
       │
       ├─ Calculate Yaw Command
       │    └─ yawCmd = error × yawGain  (constrained)
       │
       ├─ Calculate Thrust Command
       │    └─ thrust = distance × distGain × 80  (constrained)
       │
       └─ Send to Motor Controller
            └─ motors.drive(thrust, yawCmd)
               ├─ escPort = neutral + thrust + yaw
               └─ escStarboard = neutral + thrust - yaw
```

## Control Flow - Manual Mode

```
┌─────────────────────────────────────────────────────┐
│ MANUAL MODE CONTROL FLOW                            │
└─────────────────────────────────────────────────────┘

Read RC Receiver
    │
    ├─ Get Throttle (CH3) → 1000-2000µs
    │
    ├─ Get Steer (CH1) → 1000-2000µs
    │
    ├─ Map to Control Range
    │    ├─ throttle = map(ch3, 1000-2000, -500 to +500)
    │    ├─ yaw = map(ch1, 1000-2000, -500 to +500)
    │
    └─ Send to Motor Controller
         └─ motors.drive(throttle, yaw)
            ├─ escPort = neutral + throttle + yaw
            └─ escStarboard = neutral + throttle - yaw

No GPS Navigation  → Direct stick control
No Compass Feedback → Pilot controls heading
No Automatic Current Compensation
```

## Data Flow Diagram

```
                    ┌─────────────────────┐
                    │   SENSORS           │
                    ├─────────────────────┤
    GPS L80-R  ────→│ GPS Manager         │───→ Lat, Lon, HDOP
    IST8310    ────→│ Compass Manager     │───→ Heading (0-360°)
    FRIA6B PPM ────→│ Receiver PPM        │───→ 8 RC Channels
                    └─────────────────────┘
                            │
                            ▼
                    ┌─────────────────────┐
                    │ CONTROL SYSTEM      │
                    ├─────────────────────┤
                    │ • Mode Selection    │
                    │ • Water Condition   │
                    │ • Target Position   │
                    │ • Current Estimate  │
                    │ • Navigation Logic  │
                    └─────────────────────┘
                            │
                            ├─────────────────────┬──────────────────┐
                            ▼                     ▼                  ▼
                    ┌─────────────────┐  ┌─────────────────┐  ┌─────────────┐
                    │ AUTONOMOUS MODE │  │ MANUAL MODE     │  │ CURRENT EST │
                    │                 │  │                 │  │             │
                    │ • GPS→Bearing   │  │ • RC Throttle   │  │ • Drift Meas│
                    │ • Compass→Head  │  │ • RC Steer      │  │ • Direction │
                    │ • Distance Gain │  │ • Direct Ctrl   │  │ • Strength  │
                    │ • Yaw Gain      │  │                 │  │             │
                    │ • Current Comp  │  │ • No Feedback   │  │ • Used in   │
                    │                 │  │                 │  │   Nav Loop  │
                    └─────────────────┘  └─────────────────┘  └─────────────┘
                            │                     │
                            │                     │
                            └─────────────┬───────┘
                                          │
                                          ▼
                    ┌─────────────────────────────────────┐
                    │ MOTOR CONTROLLER                    │
                    │                                     │
                    │ thrust = neutral + thrust ± yaw     │
                    │                                     │
                    │ escPort ← thrust + yaw              │
                    │ escStarboard ← thrust - yaw         │
                    └─────────────────────────────────────┘
                            │                    │
                            ▼                    ▼
                        ┌────────┐          ┌────────┐
                        │ ESC P  │          │ ESC S  │
                        │ 1000-2 │          │ 1000-2 │
                        │ 000 µs │          │ 000 µs │
                        └────────┘          └────────┘
                            │                    │
                            ▼                    ▼
                    ┌────────────────────────────────┐
                    │  Brushless Motors              │
                    │  CW (Port) / CCW (Starboard)   │
                    └────────────────────────────────┘
                            │                    │
                            └────────┬───────────┘
                                     │
                                     ▼
                            ┌─────────────────┐
                            │  Propeller      │
                            │  Thrust Force   │
                            └─────────────────┘
                                     │
                                     ▼
                            ┌─────────────────────┐
                            │ Buoy Position       │
                            │ & Heading Change    │
                            └─────────────────────┘
```

## State Machine: Mode Selection

```
                    ┌──────────────────┐
                    │ INITIAL STATE    │
                    │ Autonomous Mode  │
                    │ (CH5 < 1500µs)   │
                    └────────┬─────────┘
                             │
                    CH5 > 1500µs
                             │
                             ▼
                    ┌──────────────────┐
                    │ MANUAL MODE      │
                    │ Direct Ctrl      │
                    └────────┬─────────┘
                             │
                    CH5 < 1500µs
                             │
                             ▼
                    ┌──────────────────┐
                    │ AUTONOMOUS MODE  │
                    │ Position Hold    │
                    └──────────────────┘
```

## State Machine: Water Condition

```
                    ┌──────────────────┐
                    │ CALM WATERS      │
                    │ (CH6 < 1300µs)   │
                    │                  │
                    │ Params: 2.0m rad │
                    │  1.5x dist gain  │
                    │  1.4x yaw gain   │
                    │  300 max thrust  │
                    └────────┬─────────┘
                             │
               CH6 1300-1700µs or CH6 > 1700µs
                             │
                             ▼
                    ┌──────────────────┐
                    │ LOW CURRENT      │
                    │ (CH6 1300-1700µs)│
                    │                  │
                    │ Params: 3.0m rad │
                    │  1.8x dist gain  │
                    │  1.2x yaw gain   │
                    │  360 max thrust  │
                    └────────┬─────────┘
                             │
               CH6 > 1700µs or CH6 < 1300µs
                             │
                             ▼
                    ┌──────────────────┐
                    │ STRONG CURRENT   │
                    │ (CH6 > 1700µs)   │
                    │                  │
                    │ Params: 3.5m rad │
                    │  2.0x dist gain  │
                    │  1.0x yaw gain   │
                    │  420 max thrust  │
                    └──────────────────┘
```

## Message Flow: Telemetry

```
Every 1 second (1000ms):

Main Loop
    │
    ├─ Collect Telemetry Data
    │  ├─ Current GPS position
    │  ├─ Target position
    │  ├─ Current compass heading
    │  ├─ GPS HDOP value
    │  ├─ Distance to target
    │  ├─ Estimated current speed
    │  ├─ Estimated current direction
    │  ├─ GPS fix status (0/1)
    │  ├─ Current mode (autonomous/manual)
    │  └─ Water condition (calm/low/strong)
    │
    └─ Telemetry Module
       └─ Format as CSV string
          └─ transmit via UART
             └─ $TELEM,<lat>,<lon>,...
                    └─ Ground Station
                       └─ Display/Log
```

---

This architecture provides:
- ✅ **Modularity**: Each system is self-contained
- ✅ **Flexibility**: Easy parameter tuning via config.h
- ✅ **Robustness**: Graceful degradation if sensors fail
- ✅ **Extensibility**: Simple to add new features