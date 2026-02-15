#ifndef COMPASS_H
#define COMPASS_H

#include <Arduino.h>
#include <Wire.h>

class CompassManager {
public:
  CompassManager();
  
  bool begin();
  float readHeading();
  
  void setCalibration(float offsetX, float offsetY, float scaleX, float scaleY);
  void setDeclination(float declination);
  
private:
  static const uint8_t IST8310_I2C_ADDRESS = 0x2C;  // IST8310 I2C address
  
  float _offsetX, _offsetY;
  float _scaleX, _scaleY;
  float _declination;
  float _lastHeading;
  bool _hasReading;
  
  float normalizeHeading(float heading);
  bool readRawMagnetometer(int16_t& x, int16_t& y, int16_t& z);
  bool writeRegister(uint8_t reg, uint8_t value);
  bool readRegisters(uint8_t reg, uint8_t* buffer, size_t length);
  bool triggerSingleMeasurement();
};

#endif // COMPASS_H