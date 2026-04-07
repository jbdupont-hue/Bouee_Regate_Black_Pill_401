#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>

#include "config.h"
#include "gps.h"
#include "compass.h"
#include "gyro.h"
#include "receiver.h"
#include "motor_controller.h"
#include "control_system.h"
#include "telemetry.h"

/* ===================== SYSTEM OBJECTS ===================== */
HardwareSerial SerialGPS(USART2);      // GPS on USART2 (PA2/PA3)
HardwareSerial SerialTelemetry(USART1); // HC12/Telemetry on USART1 (PA9/PA10)

GPSManager gpsManager;
CompassManager compassManager;
GyroManager gyroManager;
ReceiverPPM receiver(PPM_PIN);
MotorController motors(ESC_PORT_PIN, ESC_STARBOARD_PIN);
ControlSystem control;
Telemetry telemetry;

/* ===================== STATE VARIABLES ===================== */
unsigned long gpsLedTimer = 0;
bool gpsLedState = false;
unsigned long lastGyroUpdate = 0;
unsigned long lastGPSDebugTime = 0;
unsigned long lastRCWarningTime = 0;

// Calibration tracking
uint8_t lastCalibrationMode = 1; // 0=compass, 1=off, 2=gyro
OperationMode lastOperationMode = MODE_MANUAL;

// Current estimation tracking
double driftStartLat = 0;
double driftStartLon = 0;
unsigned long driftStartTime = 0;
bool driftingDetected = false;

// Telemetry
unsigned long lastTelemetryTime = 0;
float lastManualYawCmd = 0.0f;
float lastAutoYawCmd = 0.0f;
unsigned long lastManualControlTime = 0;
unsigned long lastAutoControlTime = 0;

static float applySlewLimit(float target, float previous, float ratePerSec, unsigned long& lastTimeMs) {
  unsigned long now = millis();
  if (lastTimeMs == 0 || ratePerSec <= 0.0f) {
    lastTimeMs = now;
    return target;
  }

  float dt = (now - lastTimeMs) / 1000.0f;
  lastTimeMs = now;
  float maxStep = ratePerSec * dt;
  float delta = target - previous;

  if (delta > maxStep) return previous + maxStep;
  if (delta < -maxStep) return previous - maxStep;
  return target;
}

/* ===================== FORWARD DECLARATIONS ===================== */
void setupSensors();
void updateGPSLed();
void processReceiver();
void autonomousMode();
void manualMode();
void estimateWaterCurrent();
void sendTelemetryData();
/* ===================== SETUP ===================== */
void setup() {
  // Initialize pins
  pinMode(LED_GPS, OUTPUT);
  digitalWrite(LED_GPS, HIGH); // LED off (inverted logic on PC13)
  
  // Startup LED blink sequence (3 blinks to confirm system is starting)
  for (int i = 0; i < 3; i++) {
    digitalWrite(LED_GPS, LOW);  // LED on
    delay(200);
    digitalWrite(LED_GPS, HIGH); // LED off
    delay(200);
  }
  
  // Initialize serial ports
  DEBUG_SERIAL.begin(DEBUG_BAUD);    // Debug output (USB or HC12)
  SerialGPS.begin(GPS_BAUD);         // GPS UART
  #ifndef USE_HC12_DEBUG
  SerialTelemetry.begin(TELEMETRY_BAUD); // Telemetry UART (if not used for debug)
  #endif
  
  delay(500);
  DEBUG_SERIAL.println("\n=== Autonomous Buoy Starting ===");
  DEBUG_SERIAL.print("Debug output via: ");
  #ifdef USE_HC12_DEBUG
  DEBUG_SERIAL.println("HC12 (USART1)");
  #else
  DEBUG_SERIAL.println("USB Serial");
  #endif
  
  // Initialize subsystems
  setupSensors();
  
#ifndef SKIP_GPS_WAIT
  // Wait for initial GPS fix
  DEBUG_SERIAL.println("Waiting for GPS fix...");
  unsigned long gpsWaitStart = millis();
  unsigned long lastGPSWaitMsg = 0;
  const unsigned long GPS_TIMEOUT_MS = 30000; // 30 second timeout
  
  while (!gpsManager.hasValidFix() || !gpsManager.hasGoodHDOP(GPS_HDOP_MAX)) {
    gpsManager.update();
    updateGPSLed();
    
    // Print GPS status every 2 seconds during wait
    if (millis() - lastGPSWaitMsg > 2000) {
      lastGPSWaitMsg = millis();
      DEBUG_SERIAL.print("GPS Status - Chars: ");
      DEBUG_SERIAL.print(gpsManager.getCharsProcessed());
      DEBUG_SERIAL.print(", Sentences: ");
      DEBUG_SERIAL.print(gpsManager.getValidSentences());
      DEBUG_SERIAL.print(", Sats: ");
      DEBUG_SERIAL.print(gpsManager.getSatellites());
      DEBUG_SERIAL.print(", HDOP: ");
      DEBUG_SERIAL.print(gpsManager.getHDOP(), 2);
      DEBUG_SERIAL.print(", Fix: ");
      DEBUG_SERIAL.print(gpsManager.hasValidFix() ? "YES" : "NO");
      if (gpsManager.hasValidFix()) {
        DEBUG_SERIAL.print(", Lat: ");
        DEBUG_SERIAL.print(gpsManager.getLatitude(), 6);
        DEBUG_SERIAL.print(", Lon: ");
        DEBUG_SERIAL.print(gpsManager.getLongitude(), 6);
      }
      DEBUG_SERIAL.println();
    }
    
    // Timeout after 30 seconds for lab testing
    if (millis() - gpsWaitStart > GPS_TIMEOUT_MS) {
      DEBUG_SERIAL.println("WARNING: GPS timeout - continuing without fix (lab mode)");
      break;
    }
    delay(100);
  }
  
  if (gpsManager.hasValidFix()) {
    // GPS fix obtained
    digitalWrite(LED_GPS, LOW); // LED on steady
    DEBUG_SERIAL.println("GPS Fix obtained! System ready.");
  }
#else
  DEBUG_SERIAL.println("GPS wait skipped (SKIP_GPS_WAIT enabled)");
#endif
  
  // Set initial target position
  control.updateTargetPosition(gpsManager.getLatitude(), gpsManager.getLongitude());
  
  // Small delay before starting main loop
  delay(1000);
}

/* ===================== MAIN LOOP ===================== */
void loop() {
  // Update sensor data
  gpsManager.update();
  updateGPSLed();
  
  // Update gyroscope at high rate
  if (millis() - lastGyroUpdate >= GYRO_UPDATE_INTERVAL_MS) {
    gyroManager.update();
    lastGyroUpdate = millis();
  }
  
  // Read receiver commands
  processReceiver();
  
  // Update water current estimation
  if (control.getMode() == MODE_AUTONOMOUS) {
    estimateWaterCurrent();
  }
  
  // Execute control mode
  if (control.getMode() == MODE_MANUAL) {
    manualMode();
  } else {
    autonomousMode();
  }
  
  // Send telemetry periodically
#ifndef USE_HC12_DEBUG
  if (millis() - lastTelemetryTime > TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = millis();
    sendTelemetryData();
  }
#endif

  // Send GPS debug info every 5 seconds
  if (millis() - lastGPSDebugTime > 5000) {
    lastGPSDebugTime = millis();
    DEBUG_SERIAL.print("[GPS] Chars: ");
    DEBUG_SERIAL.print(gpsManager.getCharsProcessed());
    DEBUG_SERIAL.print(", Sentences: ");
    DEBUG_SERIAL.print(gpsManager.getValidSentences());
    DEBUG_SERIAL.print(", Failed: ");
    DEBUG_SERIAL.print(gpsManager.getGPS().failedChecksum());
    DEBUG_SERIAL.print(", Sats: ");
    DEBUG_SERIAL.print(gpsManager.getSatellites());
    DEBUG_SERIAL.print(" (valid:");
    DEBUG_SERIAL.print(gpsManager.getGPS().satellites.isValid() ? "Y" : "N");
    DEBUG_SERIAL.print("), HDOP: ");
    DEBUG_SERIAL.print(gpsManager.getHDOP(), 2);
    DEBUG_SERIAL.print(" (valid:");
    DEBUG_SERIAL.print(gpsManager.getGPS().hdop.isValid() ? "Y" : "N");
    DEBUG_SERIAL.print("), Fix: ");
    DEBUG_SERIAL.print(gpsManager.hasValidFix() ? "YES" : "NO");
    if (gpsManager.hasValidFix()) {
      DEBUG_SERIAL.print(", Lat: ");
      DEBUG_SERIAL.print(gpsManager.getLatitude(), 6);
      DEBUG_SERIAL.print(", Lon: ");
      DEBUG_SERIAL.print(gpsManager.getLongitude(), 6);
    }
    
    // Add RC status
    DEBUG_SERIAL.print(", Mode: ");
    DEBUG_SERIAL.print(control.getMode() == MODE_MANUAL ? "MANUAL" : "AUTO");
    DEBUG_SERIAL.print(", Cond: ");
    WaterCondition cond = control.getWaterCondition();
    if (cond == CALM_WATERS) {
      DEBUG_SERIAL.print("CALM");
    } else if (cond == LOW_CURRENT) {
      DEBUG_SERIAL.print("CURRENT");
    } else {
      DEBUG_SERIAL.print("STRONG");
    }
    
    // Add calibration mode
    DEBUG_SERIAL.print(", Cal: ");
    if (lastCalibrationMode == 0) {
      DEBUG_SERIAL.print("COMPASS");
    } else if (lastCalibrationMode == 2) {
      DEBUG_SERIAL.print("GYRO");
    } else {
      DEBUG_SERIAL.print("OFF");
    }
    
    // Add target position
    DEBUG_SERIAL.print(", Target: ");
    DEBUG_SERIAL.print(control.getTargetLat(), 6);
    DEBUG_SERIAL.print(", ");
    DEBUG_SERIAL.print(control.getTargetLon(), 6);
    DEBUG_SERIAL.println();
  }
  
  delay(50); // 20 Hz main loop
}

/* ===================== INITIALIZATION ===================== */
void setupSensors() {
  // Initialize GPS
  DEBUG_SERIAL.println(">> Initializing GPS...");
  gpsManager.begin(SerialGPS, GPS_BAUD);
  DEBUG_SERIAL.println("   GPS initialized");
  
  // Initialize compass
#ifndef SKIP_COMPASS_INIT
  DEBUG_SERIAL.println(">> Initializing I2C bus...");
  DEBUG_SERIAL.flush(); // Force output before I2C init
  
  // Configure I2C pins for STM32
  Wire.setSDA(MAG_SDA);  // PB9
  Wire.setSCL(MAG_SCL);  // PB8
  Wire.begin();
  Wire.setClock(100000); // 100kHz I2C for compatibility
  delay(100); // Allow I2C bus to stabilize
  
  DEBUG_SERIAL.println(">> Scanning I2C bus for devices...");
  DEBUG_SERIAL.flush();
  
  // I2C Scanner
  int deviceCount = 0;
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();
    
    if (error == 0) {
      DEBUG_SERIAL.print("   I2C device found at address 0x");
      if (addr < 16) DEBUG_SERIAL.print("0");
      DEBUG_SERIAL.println(addr, HEX);
      deviceCount++;
    }
  }
  
  if (deviceCount == 0) {
    DEBUG_SERIAL.println("   No I2C devices found! Check wiring.");
  } else {
    DEBUG_SERIAL.print("   Found ");
    DEBUG_SERIAL.print(deviceCount);
    DEBUG_SERIAL.println(" device(s)");
  }
  DEBUG_SERIAL.flush();
  
  DEBUG_SERIAL.println(">> Attempting compass detection...");
  DEBUG_SERIAL.flush();
  
  if (!compassManager.begin()) {
    DEBUG_SERIAL.println("WARNING: Compass initialization failed - check wiring!");
    DEBUG_SERIAL.println("  Expected I2C address: 0x2C (IST8310)");
    DEBUG_SERIAL.println("  I2C pins: SDA=PB9, SCL=PB8");
    DEBUG_SERIAL.println("System will continue without compass (limited autonomous capability)");
  } else {
    compassManager.setCalibration(MAG_OFFSET_X, MAG_OFFSET_Y, MAG_SCALE_X, MAG_SCALE_Y);
    compassManager.setDeclination(MAG_DECLINATION);
    DEBUG_SERIAL.println("Compass initialized successfully!");
  }
#else
  DEBUG_SERIAL.println(">> Compass initialization SKIPPED (SKIP_COMPASS_INIT defined)");
#endif
  
  // Initialize gyroscope (optional - system works without it)
  DEBUG_SERIAL.println(">> Initializing SPI bus...");
  DEBUG_SERIAL.flush();
  
  SPI.begin();
  DEBUG_SERIAL.println("   SPI bus initialized");
  
  if (!gyroManager.begin(GYRO_CS)) {
    DEBUG_SERIAL.println("WARNING: Gyroscope initialization failed - continuing without yaw damping");
  } else {
    DEBUG_SERIAL.println("Gyroscope initialized - yaw rate damping enabled");
  }
  
  // Initialize receiver PPM
  DEBUG_SERIAL.println(">> Initializing receiver...");
  receiver.begin();
  DEBUG_SERIAL.println("   Receiver initialized");
  
  // Initialize motors
  DEBUG_SERIAL.println(">> Initializing motors...");
  motors.begin();
  motors.stop();
  DEBUG_SERIAL.println("   Motors initialized");
  
  // Initialize telemetry
#ifndef USE_HC12_DEBUG
  DEBUG_SERIAL.println(">> Initializing telemetry...");
  telemetry.begin(SerialTelemetry, TELEMETRY_BAUD);
  DEBUG_SERIAL.println("   Telemetry initialized");
#else
  DEBUG_SERIAL.println(">> Telemetry DISABLED (HC12 used for debug)");
#endif
  
  DEBUG_SERIAL.println("All sensors initialized successfully");
}

/* ===================== RECEIVER INPUT PROCESSING ===================== */
void processReceiver() {
  // Update receiver (updates lastPulseTime from ISR)
  receiver.update();
  
  // Check signal validity
  if (!receiver.isSignalValid()) {
    // Loss of signal - stop motors for safety
    motors.stop();
    // Print warning only every 10 seconds
    if (millis() - lastRCWarningTime > 10000) {
      lastRCWarningTime = millis();
      DEBUG_SERIAL.println("WARNING: RC signal lost!");
    }
    return;
  }
  
  // Get mode switch (CH5)
  uint16_t modeSwitch = receiver.getMode();
  OperationMode newMode = (modeSwitch > RC_MODE_THRESHOLD) ? MODE_MANUAL : MODE_AUTONOMOUS;
  
  // Update target position when switching from MANUAL to AUTO
  if (newMode == MODE_AUTONOMOUS && lastOperationMode == MODE_MANUAL) {
    if (gpsManager.hasValidFix()) {
      control.updateTargetPosition(gpsManager.getLatitude(), gpsManager.getLongitude());
      DEBUG_SERIAL.println("=== Switched to AUTO mode - target position updated ===");
    }
  }
  
  control.setMode(newMode);

  if (newMode != lastOperationMode) {
    lastManualYawCmd = 0.0f;
    lastAutoYawCmd = 0.0f;
    lastManualControlTime = 0;
    lastAutoControlTime = 0;
  }

  lastOperationMode = newMode;
  
  // Get current type selector (CH6)
  uint16_t currentTypeSwitch = receiver.getCurrentType();
  WaterCondition condition;
  if (currentTypeSwitch < RC_CALM_THRESHOLD_LOW) {
    condition = CALM_WATERS;
  } else if (currentTypeSwitch > RC_CALM_THRESHOLD_HIGH) {
    condition = STRONG_CURRENT;
  } else {
    condition = LOW_CURRENT;
  }
  control.setWaterCondition(condition);
  
  // Get calibration switch (CH7)
  uint16_t calSwitch = receiver.getChannel(CH_CALIBRATION);
  uint8_t calMode;
  
  if (calSwitch < RC_CAL_COMPASS_MAX) {
    calMode = 0; // Compass calibration
  } else if (calSwitch > RC_CAL_GYRO_MIN) {
    calMode = 2; // Gyro calibration
  } else {
    calMode = 1; // Off
  }
  
  // Trigger calibration on mode change
  if (calMode != lastCalibrationMode) {
    if (calMode == 0) {
      DEBUG_SERIAL.println("=== COMPASS CALIBRATION MODE ===");
      DEBUG_SERIAL.println("Rotate the buoy slowly in all directions for 30 seconds...");
      // Compass calibration is continuous while switch is in this position
    } else if (calMode == 2) {
      DEBUG_SERIAL.println("=== GYRO CALIBRATION ===");
      DEBUG_SERIAL.println("Keep buoy stationary...");
      motors.stop();
      gyroManager.calibrate();
      DEBUG_SERIAL.println("Gyro calibration complete!");
    } else {
      DEBUG_SERIAL.println("Calibration mode: OFF");
    }
    lastCalibrationMode = calMode;
  }
}

/* ===================== AUTONOMOUS MODE ===================== */
void autonomousMode() {
  if (!gpsManager.hasValidFix()) {
    lastAutoYawCmd = 0.0f;
    lastAutoControlTime = 0;
    motors.stop();
    return;
  }
  
  double currentLat = gpsManager.getLatitude();
  double currentLon = gpsManager.getLongitude();
  double targetLat = control.getTargetLat();
  double targetLon = control.getTargetLon();
  
  // Calculate distance to target
  float distance = gpsManager.getDistanceTo(targetLat, targetLon);
  float holdRadius = control.getParameters().holdRadius;
  
  // Check if at target
  if (distance < holdRadius) {
    lastAutoYawCmd = 0.0f;
    lastAutoControlTime = 0;
    motors.stop();
    driftingDetected = false;
    return;
  }
  
  // Get bearing to target
  float bearing = gpsManager.getBearingTo(targetLat, targetLon);
  
  // Read compass heading (use GPS bearing as fallback if compass unavailable)
  float heading = compassManager.readHeading();
  if (heading == 0.0 && bearing != 0.0) {
    // Compass might not be initialized, use GPS bearing instead
    heading = bearing;
  }
  
  // Calculate desired bearing accounting for estimated current
  float desiredBearing = bearing;
  if (control.isCurrentValid() && control.getCurrentStrength() > CURRENT_LOCK_THRESHOLD) {
    // Strong current detected - head upstream to compensate
    float upstreamBearing = fmod(control.getCurrentDirection() + 180.0, 360.0);
    float weight = constrain(control.getCurrentStrength() / 3.0, 0.0, 1.0);
    float blendError = control.headingError(upstreamBearing, bearing);
    desiredBearing = bearing + weight * blendError;
    if (desiredBearing < 0.0f) desiredBearing += 360.0f;
    if (desiredBearing >= 360.0f) desiredBearing -= 360.0f;
  }
  
  // Calculate heading error
  float error = control.headingError(desiredBearing, heading);
  
  // Apply deadband
  if (abs(error) < AUTO_HOLD_HEADING_DEADBAND_DEG) {
    error = 0.0;
  }
  
  // Calculate base yaw command from heading error
  float yawCmd = error * control.getParameters().yawGain;
  
  // Add yaw rate damping to prevent spinning (gyro feedback)
  if (gyroManager.isReady()) {
    float yawRate = gyroManager.getYawRate();
    
    // Apply damping only if yaw rate exceeds deadband
    if (abs(yawRate) > YAW_RATE_DEADBAND) {
      float dampingCorrection = -yawRate * control.getParameters().yawRateGain;
      yawCmd += dampingCorrection;
    }
  }
  
  // Constrain yaw command
  yawCmd = constrain(yawCmd, -AUTO_MAX_YAW_COMMAND, AUTO_MAX_YAW_COMMAND);
  
  // Calculate thrust based on distance
  float thrust = 0.0f;
  float softBandLimit = holdRadius + AUTO_HOLD_SOFT_BAND_M;
  if (distance < softBandLimit) {
    float proximityScale = constrain((distance - holdRadius) / AUTO_HOLD_SOFT_BAND_M, 0.0f, 1.0f);
    thrust = distance * control.getParameters().distanceGain * 80.0f * proximityScale;
    yawCmd = constrain(yawCmd, -AUTO_HOLD_NEAR_YAW_LIMIT, AUTO_HOLD_NEAR_YAW_LIMIT);
    yawCmd *= (0.35f + 0.65f * proximityScale);
  } else {
    thrust = constrain(distance * control.getParameters().distanceGain * 80.0f,
                       control.getParameters().minThrust,
                       control.getParameters().maxThrust);
  }

  thrust = constrain(thrust, 0.0f, control.getParameters().maxThrust);

  float absHeadingError = fabsf(error);
  float headingScale = 1.0f;
  if (absHeadingError > AUTO_HEADING_BRAKE_START_DEG) {
    if (absHeadingError >= AUTO_HEADING_BRAKE_FULL_DEG) {
      headingScale = AUTO_MIN_THRUST_SCALE;
    } else {
      float blend = (absHeadingError - AUTO_HEADING_BRAKE_START_DEG) /
                    (AUTO_HEADING_BRAKE_FULL_DEG - AUTO_HEADING_BRAKE_START_DEG);
      headingScale = 1.0f - blend * (1.0f - AUTO_MIN_THRUST_SCALE);
    }
  }
  thrust *= headingScale;

  yawCmd = applySlewLimit(yawCmd, lastAutoYawCmd, AUTO_YAW_SLEW_PER_SEC, lastAutoControlTime);
  lastAutoYawCmd = yawCmd;
  
  // Apply motor commands
  motors.drive((int)thrust, (int)yawCmd);
}

/* ===================== MANUAL MODE ===================== */
void manualMode() {
  // Read throttle and steering from receiver
  uint16_t throttleRC = receiver.getThrottle();
  uint16_t steerRC = receiver.getSteer();
  
  // Apply dead band to throttle
  int throttle = 0;
  if (abs(throttleRC - RC_NEUTRAL) > RC_DEADBAND) {
    throttle = map(throttleRC, RC_MIN, RC_MAX, -MANUAL_MAX_RANGE, MANUAL_MAX_RANGE);
  }
  
  // Apply dead band to steering
  float yaw = 0.0f;
  int steerDelta = (int)steerRC - RC_NEUTRAL - MANUAL_STEER_TRIM_US;
  if (abs(steerDelta) > RC_DEADBAND) {
    float steerSign = (steerDelta >= 0) ? 1.0f : -1.0f;
    float steerMagnitude = (float)(abs(steerDelta) - RC_DEADBAND) / (500.0f - RC_DEADBAND);
    steerMagnitude = constrain(steerMagnitude, 0.0f, 1.0f);
    float shapedMagnitude = powf(steerMagnitude, MANUAL_STEER_EXPO);
    float yawMagnitude = MANUAL_MIN_YAW_BITE +
                         (MANUAL_MAX_YAW_RANGE - MANUAL_MIN_YAW_BITE) * shapedMagnitude;
    yaw = steerSign * yawMagnitude;
  }

  // Straight-line assist: when steering stick is centered, use gyro damping to resist unwanted yaw drift
  if (abs(steerDelta) <= RC_DEADBAND &&
      abs(throttle) >= MANUAL_STRAIGHT_MIN_THROTTLE &&
      gyroManager.isReady()) {
    float yawRate = gyroManager.getYawRate();
    float yawCorrection = constrain(-yawRate * MANUAL_GYRO_STRAIGHT_GAIN,
                                    -MANUAL_STRAIGHT_MAX_YAW_CORR,
                                    MANUAL_STRAIGHT_MAX_YAW_CORR);
    yaw += yawCorrection;
  }

  yaw = constrain(yaw, -MANUAL_MAX_YAW_RANGE, MANUAL_MAX_YAW_RANGE);
  yaw = applySlewLimit(yaw, lastManualYawCmd, MANUAL_YAW_SLEW_PER_SEC, lastManualControlTime);
  lastManualYawCmd = yaw;
  
  // Apply motor commands
  motors.drive(throttle, (int)yaw);
}

/* ===================== WATER CURRENT ESTIMATION ===================== */
void estimateWaterCurrent() {
  if (!gpsManager.hasValidFix()) return;
  
  double currentLat = gpsManager.getLatitude();
  double currentLon = gpsManager.getLongitude();
  double targetLat = control.getTargetLat();
  double targetLon = control.getTargetLon();
  
  float distance = gpsManager.getDistanceTo(targetLat, targetLon);
  float holdRadius = control.getParameters().holdRadius;
  
  // Check if motors are stopped (within holding radius)
  bool motorsInactive = (distance < holdRadius);
  
  if (motorsInactive) {
    // Start/continue drift measurement
    if (!driftingDetected) {
      driftStartLat = currentLat;
      driftStartLon = currentLon;
      driftStartTime = millis();
      driftingDetected = true;
    }
    
    // After sufficient time has passed, calculate drift
    if (millis() - driftStartTime >= CURRENT_SAMPLE_TIME_MS) {
      float elapsedSec = (millis() - driftStartTime) / 1000.0f;
      float driftDistance = TinyGPSPlus::distanceBetween(driftStartLat, driftStartLon, 
                                                         currentLat, currentLon);
      float driftDirection = TinyGPSPlus::courseTo(driftStartLat, driftStartLon, 
                                                   currentLat, currentLon);
      
      // Current strength in m/s
      float currentStrength = (elapsedSec > 0.0f) ? (driftDistance / elapsedSec) : 0.0f;
      
      control.updateCurrentEstimate(currentStrength, driftDirection, true);
      
      // Reset for next measurement
      driftingDetected = false;
    }
  } else {
    // Motors active - stop current estimation
    driftingDetected = false;
  }
}

/* ===================== GPS LED STATUS ===================== */
void updateGPSLed() {
  if (gpsManager.hasValidFix() && gpsManager.hasGoodHDOP(GPS_HDOP_MAX)) {
    // Steady LED (good fix)
    digitalWrite(LED_GPS, LOW);
  } else if (gpsManager.hasValidFix()) {
    // Fast blinking LED (fix but poor HDOP) - 250ms interval
    if (millis() - gpsLedTimer > 250) {
      gpsLedTimer = millis();
      gpsLedState = !gpsLedState;
      digitalWrite(LED_GPS, gpsLedState ? LOW : HIGH);
    }
  } else {
    // Slow blinking LED (no fix, searching) - 1000ms interval
    if (millis() - gpsLedTimer > 1000) {
      gpsLedTimer = millis();
      gpsLedState = !gpsLedState;
      digitalWrite(LED_GPS, gpsLedState ? LOW : HIGH);
    }
  }
}

/* ===================== TELEMETRY ===================== */
void sendTelemetryData() {
  TelemetryData data;
  
  data.currentLat = gpsManager.getLatitude();
  data.currentLon = gpsManager.getLongitude();
  data.targetLat = control.getTargetLat();
  data.targetLon = control.getTargetLon();
  data.heading = compassManager.readHeading();
  data.hdop = gpsManager.getHDOP();
  data.distance = gpsManager.getDistanceTo(control.getTargetLat(), control.getTargetLon());
  data.estimatedCurrent = control.getCurrentStrength();
  data.currentDirection = control.getCurrentDirection();
  data.yawRate = gyroManager.getYawRate();
  data.rollRate = gyroManager.getRollRate();
  data.pitchRate = gyroManager.getPitchRate();
  data.gpsFixed = gpsManager.hasValidFix() ? 1 : 0;
  data.mode = control.getMode();
  data.currentType = control.getWaterCondition();
  data.gyroReady = gyroManager.isReady() ? 1 : 0;
  
  telemetry.sendTelemetry(data);
}
