#include "gps.h"
#include "config.h"

GPSManager::GPSManager() 
  : _serial(nullptr),
    _gnssSatellites(_gps, "GNGGA", 7),  // GNGGA field 7 = satellites used
    _gnssHDOP(_gps, "GNGSA", 16),       // HDOP from GNGSA sentence, field 16
    _satelliteCount(0),
    _hdopValue(999.0),
    _customDataValid(false) {
}

void GPSManager::begin(HardwareSerial& serial, uint32_t baudRate) {
  _serial = &serial;
  _serial->begin(baudRate);
}

void GPSManager::update() {
  if (_serial) {
    while (_serial->available()) {
      char c = _serial->read();
      _gps.encode(c);
    }
    
    // Update custom GNSS data if available
    if (_gnssSatellites.isUpdated()) {
      _satelliteCount = atoi(_gnssSatellites.value());
      _customDataValid = true;
    }
    if (_gnssHDOP.isUpdated()) {
      _hdopValue = atof(_gnssHDOP.value());
      _customDataValid = true;
    }
  }
}

bool GPSManager::hasValidFix() const {
  return _gps.location.isValid();
}

bool GPSManager::hasGoodHDOP(float maxHDOP) const {
  float hdop = getHDOP();
  return hdop <= maxHDOP;
}

double GPSManager::getLatitude() const {
  return _gps.location.lat();
}

double GPSManager::getLongitude() const {
  return _gps.location.lng();
}

float GPSManager::getHDOP() const {
  // Try custom GNSS HDOP first, then fall back to standard GPS
  if (_customDataValid && _hdopValue < 999.0) {
    return _hdopValue;
  }
  return _gps.hdop.isValid() ? _gps.hdop.hdop() : 999.0;
}

float GPSManager::getSpeed() const {
  return _gps.speed.isValid() ? _gps.speed.kmph() : 0.0;
}

uint32_t GPSManager::getSatellites() const {
  // Try custom GNSS satellite count first, then fall back to standard GPS
  if (_customDataValid && _satelliteCount > 0) {
    return _satelliteCount;
  }
  return _gps.satellites.isValid() ? _gps.satellites.value() : 0;
}

uint32_t GPSManager::getCharsProcessed() const {
  return _gps.charsProcessed();
}

uint32_t GPSManager::getValidSentences() const {
  return _gps.passedChecksum();
}

float GPSManager::getDistanceTo(double targetLat, double targetLon) const {
  if (!hasValidFix()) return 0.0;
  return TinyGPSPlus::distanceBetween(
    getLatitude(), getLongitude(),
    targetLat, targetLon
  );
}

float GPSManager::getBearingTo(double targetLat, double targetLon) const {
  if (!hasValidFix()) return 0.0;
  return TinyGPSPlus::courseTo(
    getLatitude(), getLongitude(),
    targetLat, targetLon
  );
}

TinyGPSPlus& GPSManager::getGPS() {
  return _gps;
}