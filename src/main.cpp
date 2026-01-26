#include <Arduino.h>
#include <Wire.h>

#include "config.h"
#include "gps.h"
#include "compass.h"
#include "receiver.h"
#include "motor_controller.h"
#include "control_system.h"
#include "telemetry.h"

/* ===================== SYSTEM OBJECTS ===================== */
HardwareSerial SerialGPS(USART1);
HardwareSerial SerialTelemetry(USART2);

GPSManager gpsManager;
CompassManager compassManager;
ReceiverPPM receiver(PPM_PIN);
MotorController motors(ESC_PORT_PIN, ESC_STARBOARD_PIN);
ControlSystem control;
Telemetry telemetry;

/* ===================== STATE VARIABLES ===================== */
unsigned long gpsLedTimer = 0;
bool gpsLedState = false;

// Current estimation tracking
double driftStartLat = 0;
double driftStartLon = 0;
unsigned long driftStartTime = 0;
bool driftingDetected = false;

// Telemetry
unsigned long lastTelemetryTime = 0;

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
  
  // Initialize serial ports
  Serial.begin(DEBUG_BAUD);          // USB debug
  SerialGPS.begin(GPS_BAUD);         // GPS UART
  SerialTelemetry.begin(TELEMETRY_BAUD); // Telemetry UART
  
  delay(500);
  Serial.println("\n=== Autonomous Buoy Starting ===");
  
  // Initialize subsystems
  setupSensors();
  
  // Wait for initial GPS fix
  Serial.println("Waiting for GPS fix...");
  while (!gpsManager.hasValidFix() || !gpsManager.hasGoodHDOP(GPS_HDOP_MAX)) {
    gpsManager.update();
    updateGPSLed();
    delay(100);
  }
  
  // GPS fix obtained
  digitalWrite(LED_GPS, LOW); // LED on steady
  Serial.println("GPS Fix obtained! System ready.");
  
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
  if (millis() - lastTelemetryTime > TELEMETRY_INTERVAL_MS) {
    lastTelemetryTime = millis();
    sendTelemetryData();
  }
  
  delay(50); // 20 Hz main loop
}

/* ===================== INITIALIZATION ===================== */
void setupSensors() {
  // Initialize GPS
  gpsManager.begin(SerialGPS, GPS_BAUD);
  
  // Initialize compass
  Wire.begin();
  if (!compassManager.begin()) {
    Serial.println("ERROR: Compass initialization failed!");
    while (1);
  }
  compassManager.setCalibration(MAG_OFFSET_X, MAG_OFFSET_Y, MAG_SCALE_X, MAG_SCALE_Y);
  compassManager.setDeclination(MAG_DECLINATION);
  
  // Initialize receiver PPM
  receiver.begin();
  
  // Initialize motors
  motors.begin();
  motors.stop();
  
  // Initialize telemetry
  telemetry.begin(SerialTelemetry, TELEMETRY_BAUD);
  
  Serial.println("All sensors initialized successfully");
}

/* ===================== RECEIVER INPUT PROCESSING ===================== */
void processReceiver() {
  // Check signal validity
  if (!receiver.isSignalValid()) {
    // Loss of signal - stop motors for safety
    motors.stop();
    Serial.println("WARNING: RC signal lost!");
    return;
  }
  
  // Get mode switch (CH5)
  uint16_t modeSwitch = receiver.getMode();
  OperationMode newMode = (modeSwitch > RC_MODE_THRESHOLD) ? MODE_MANUAL : MODE_AUTONOMOUS;
  control.setMode(newMode);
  
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
}

/* ===================== AUTONOMOUS MODE ===================== */
void autonomousMode() {
  if (!gpsManager.hasValidFix()) {
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
    motors.stop();
    driftingDetected = false;
    return;
  }
  
  // Get bearing to target
  float bearing = gpsManager.getBearingTo(targetLat, targetLon);
  
  // Read compass heading
  float heading = compassManager.readHeading();
  
  // Calculate desired bearing accounting for estimated current
  float desiredBearing = bearing;
  if (control.isCurrentValid() && control.getCurrentStrength() > CURRENT_LOCK_THRESHOLD) {
    // Strong current detected - head upstream to compensate
    float upstreamBearing = fmod(control.getCurrentDirection() + 180.0, 360.0);
    float weight = constrain(control.getCurrentStrength() / 3.0, 0.0, 1.0);
    desiredBearing = (1.0 - weight) * bearing + weight * upstreamBearing;
  }
  
  // Calculate heading error
  float error = control.headingError(desiredBearing, heading);
  
  // Apply deadband
  if (abs(error) < 5.0) {
    error = 0.0;
  }
  
  // Calculate control outputs
  float yawCmd = constrain(error * control.getParameters().yawGain, 
                           -MAX_YAW, MAX_YAW);
  float thrust = constrain(distance * control.getParameters().distanceGain * 80, 
                           control.getParameters().minThrust, 
                           control.getParameters().maxThrust);
  
  // Apply motor commands
  motors.drive((int)thrust, (int)yawCmd);
}

/* ===================== MANUAL MODE ===================== */
void manualMode() {
  // Read throttle and steering from receiver
  uint16_t throttleRC = receiver.getThrottle();
  uint16_t steerRC = receiver.getSteer();
  
  // Convert RC values to control commands
  int throttle = map(throttleRC, RC_MIN, RC_MAX, -500, 500);
  int yaw = map(steerRC, RC_MIN, RC_MAX, -500, 500);
  
  // Apply motor commands
  motors.drive(throttle, yaw);
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
      float driftDistance = TinyGPSPlus::distanceBetween(driftStartLat, driftStartLon, 
                                                         currentLat, currentLon);
      float driftDirection = TinyGPSPlus::courseTo(driftStartLat, driftStartLon, 
                                                   currentLat, currentLon);
      
      // Current strength in m/s
      float currentStrength = driftDistance / ((CURRENT_SAMPLE_TIME_MS / 1000.0) / 1000.0);
      
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
    // Blinking LED (fix but poor HDOP)
    if (millis() - gpsLedTimer > 500) {
      gpsLedTimer = millis();
      gpsLedState = !gpsLedState;
      digitalWrite(LED_GPS, gpsLedState ? LOW : HIGH);
    }
  } else {
    // LED off (no fix)
    digitalWrite(LED_GPS, HIGH);
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
  data.gpsFixed = gpsManager.hasValidFix() ? 1 : 0;
  data.mode = control.getMode();
  data.currentType = control.getWaterCondition();
  
  telemetry.sendTelemetry(data);
}