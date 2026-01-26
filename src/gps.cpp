#include "gps.h"

GPSManager::GPSManager() : _serial(nullptr) {
}

void GPSManager::begin(HardwareSerial& serial, uint32_t baudRate) {
  _serial = &serial;
  _serial->begin(baudRate);
}

void GPSManager::update() {
  if (_serial) {
    while (_serial->available()) {
      _gps.encode(_serial->read());
    }
  }
}

bool GPSManager::hasValidFix() const {
  return _gps.location.isValid();
}

bool GPSManager::hasGoodHDOP(float maxHDOP) const {
  return _gps.hdop.isValid() && _gps.hdop.hdop() <= maxHDOP;
}

double GPSManager::getLatitude() const {
  return _gps.location.lat();
}

double GPSManager::getLongitude() const {
  return _gps.location.lng();
}

float GPSManager::getHDOP() const {
  return _gps.hdop.isValid() ? _gps.hdop.hdop() : 999.0;
}

float GPSManager::getSpeed() const {
  return _gps.speed.isValid() ? _gps.speed.kmph() : 0.0;
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
