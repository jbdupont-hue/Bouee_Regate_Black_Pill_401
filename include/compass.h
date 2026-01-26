#ifndef COMPASS_H
#define COMPASS_H

#include <Adafruit_HMC5883_U.h>
#include <Arduino.h>

class CompassManager {
public:
  CompassManager();
  
  bool begin();
  float readHeading();
  
  void setCalibration(float offsetX, float offsetY, float scaleX, float scaleY);
  void setDeclination(float declination);
  
private:
  Adafruit_HMC5883_Unified _compass;
  float _offsetX, _offsetY;
  float _scaleX, _scaleY;
  float _declination;
  
  float normalizeHeading(float heading);
};

#endif // COMPASS_H
