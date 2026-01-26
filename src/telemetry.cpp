#include "telemetry.h"

Telemetry::Telemetry() : _serial(nullptr), _lastSendTime(0) {
}

void Telemetry::begin(HardwareSerial& serial, uint32_t baudRate) {
  _serial = &serial;
  _serial->begin(baudRate);
}

void Telemetry::sendTelemetry(const TelemetryData& data) {
  if (!_serial) return;
  
  unsigned long now = millis();
  // Send at regular intervals
  // if (now - _lastSendTime < 1000) return;
  // _lastSendTime = now;
  
  // Simple text-based telemetry format for SIK radio
  _serial->print("$TELEM,");
  _serial->print(data.currentLat, 8); _serial->print(",");
  _serial->print(data.currentLon, 8); _serial->print(",");
  _serial->print(data.targetLat, 8); _serial->print(",");
  _serial->print(data.targetLon, 8); _serial->print(",");
  _serial->print(data.heading, 1); _serial->print(",");
  _serial->print(data.hdop, 2); _serial->print(",");
  _serial->print(data.distance, 1); _serial->print(",");
  _serial->print(data.estimatedCurrent, 2); _serial->print(",");
  _serial->print(data.currentDirection, 1); _serial->print(",");
  _serial->print(data.gpsFixed); _serial->print(",");
  _serial->print(data.mode); _serial->print(",");
  _serial->println(data.currentType);
}

void Telemetry::sendDebug(const String& message) {
  if (!_serial) return;
  _serial->println(message);
}
