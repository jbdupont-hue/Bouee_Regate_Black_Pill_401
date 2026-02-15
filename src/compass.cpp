#include "compass.h"
#include "config.h"

// IST8310 register map (based on common IST83xx layout)
static const uint8_t IST8310_REG_WHO_AM_I = 0x00;
static const uint8_t IST8310_REG_DATA_START = 0x03; // X_L, X_H, Y_L, Y_H, Z_L, Z_H
static const uint8_t IST8310_REG_CTRL1 = 0x0A;
static const uint8_t IST8310_MODE_SINGLE = 0x01;

CompassManager::CompassManager()
  : _offsetX(0), _offsetY(0),
    _scaleX(1.0), _scaleY(1.0),
    _declination(0),
    _lastHeading(0.0),
    _hasReading(false) {
}

bool CompassManager::begin() {
  // Set I2C timeout to prevent hanging (STM32 specific)
  Wire.setTimeout(100); // 100ms timeout
  
  // Test I2C communication
  Wire.beginTransmission(IST8310_I2C_ADDRESS);
  uint8_t error = Wire.endTransmission();
  
  if (error != 0) {
    // I2C communication failed
    return false;
  }

  // Attempt a basic WHO_AM_I read (optional, ignore if read fails)
  uint8_t whoAmI = 0;
  readRegisters(IST8310_REG_WHO_AM_I, &whoAmI, 1);

  // Set single measurement mode (safe for most IST83xx-style parts)
  writeRegister(IST8310_REG_CTRL1, IST8310_MODE_SINGLE);

  return true;
}

float CompassManager::readHeading() {
  int16_t x = 0, y = 0, z = 0;
  if (!readRawMagnetometer(x, y, z)) {
    return _hasReading ? _lastHeading : 0.0f;
  }

  float mx = static_cast<float>(x);
  float my = static_cast<float>(y);

  // Apply hard-iron offset (calibration)
  mx -= _offsetX;
  my -= _offsetY;

  // Apply soft-iron scale (calibration)
  mx *= _scaleX;
  my *= _scaleY;

  // Calculate heading from magnetometer values
  float heading = atan2(my, mx) * 180.0f / PI;

  // Apply magnetic declination
  heading += _declination;

  _lastHeading = normalizeHeading(heading);
  _hasReading = true;
  return _lastHeading;
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
  if (heading < 0) heading += 360.0f;
  if (heading >= 360.0f) heading -= 360.0f;
  return heading;
}

bool CompassManager::readRawMagnetometer(int16_t& x, int16_t& y, int16_t& z) {
  if (!triggerSingleMeasurement()) {
    return false;
  }

  delay(10);

  uint8_t data[6] = {0};
  if (!readRegisters(IST8310_REG_DATA_START, data, sizeof(data))) {
    return false;
  }

  x = static_cast<int16_t>((data[1] << 8) | data[0]);
  y = static_cast<int16_t>((data[3] << 8) | data[2]);
  z = static_cast<int16_t>((data[5] << 8) | data[4]);

  return true;
}

bool CompassManager::writeRegister(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(IST8310_I2C_ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool CompassManager::readRegisters(uint8_t reg, uint8_t* buffer, size_t length) {
  Wire.beginTransmission(IST8310_I2C_ADDRESS);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) {
    return false;
  }

  size_t readCount = Wire.requestFrom(IST8310_I2C_ADDRESS, static_cast<uint8_t>(length));
  if (readCount != length) {
    return false;
  }

  for (size_t i = 0; i < length; i++) {
    buffer[i] = Wire.read();
  }

  return true;
}

bool CompassManager::triggerSingleMeasurement() {
  return writeRegister(IST8310_REG_CTRL1, IST8310_MODE_SINGLE);
}
