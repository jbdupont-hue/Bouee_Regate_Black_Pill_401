#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <Arduino.h>

// Telemetry structure for SIK radio protocol
struct TelemetryData {
  double currentLat;      // Current GPS latitude
  double currentLon;      // Current GPS longitude
  double targetLat;       // Target GPS latitude
  double targetLon;       // Target GPS longitude
  float heading;          // Compass heading (degrees)
  float hdop;             // GPS HDOP
  float distance;         // Distance to target (meters)
  float estimatedCurrent; // Estimated current strength (m/s)
  float currentDirection; // Estimated current direction (degrees)
  uint8_t gpsFixed;       // GPS fix status (0=no fix, 1=fix)
  uint8_t mode;           // Current mode (0=autonomous, 1=manual)
  uint8_t currentType;    // Water current type (0=calm, 1=low, 2=strong)
};

class Telemetry {
public:
  Telemetry();
  
  void begin(HardwareSerial& serial, uint32_t baudRate);
  void sendTelemetry(const TelemetryData& data);
  void sendDebug(const String& message);
  
private:
  HardwareSerial* _serial;
  unsigned long _lastSendTime;
};

#endif // TELEMETRY_H
