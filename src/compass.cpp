#include "compass.h"

CompassManager::CompassManager() 
  : _compass(12345),
    _offsetX(0), _offsetY(0),
    _scaleX(1.0), _scaleY(1.0),
    _declination(0) {
}

bool CompassManager::begin() {
  return _compass.begin();
}

float CompassManager::readHeading() {
  sensors_event_t event;
  _compass.getEvent(&event);
  
  float mx = event.magnetic.x;
  float my = event.magnetic.y;
  
  // Apply hard-iron offset (calibration)
  mx -= _offsetX;
  my -= _offsetY;
  
  // Apply soft-iron scale (calibration)
  mx *= _scaleX;
  my *= _scaleY;
  
  // Calculate heading from magnetometer values
  float heading = atan2(my, mx) * 180.0 / PI;
  
  // Apply magnetic declination
  heading += _declination;
  
  // Normalize to 0-360 degrees
  return normalizeHeading(heading);
}

void CompassManager::setCalibration(float offsetX, float offsetY, float scaleX, float scaleY) {
  _offsetX = offsetX;
  _offsetY = offsetY;
  _scaleX = scaleX;
  _scaleY = scaleY;
}

void CompassManager::setDeclination(float declination) {
  _declination = declination;
}

float CompassManager::normalizeHeading(float heading) {
  if (heading < 0) heading += 360.0;
  if (heading >= 360.0) heading -= 360.0;
  return heading;
}
