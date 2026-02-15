#include "gyro.h"
#include "config.h"

GyroManager::GyroManager()
  : _csPin(0),
    _rawGyroX(0), _rawGyroY(0), _rawGyroZ(0),
    _rawAccelX(0), _rawAccelY(0), _rawAccelZ(0), _rawTemp(0),
    _gyroOffsetX(0), _gyroOffsetY(0), _gyroOffsetZ(0),
    _initialized(false), _lastUpdate(0), _filteredYawRate(0) {
}

uint8_t GyroManager::readRegister(uint8_t reg) const {
  digitalWrite(_csPin, LOW);
  SPI.transfer(reg | 0x80); // Read operation (bit 7 = 1)
  uint8_t value = SPI.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  return value;
}

void GyroManager::writeRegister(uint8_t reg, uint8_t value) const {
  digitalWrite(_csPin, LOW);
  SPI.transfer(reg & 0x7F); // Write operation (bit 7 = 0)
  SPI.transfer(value);
  digitalWrite(_csPin, HIGH);
}

int16_t GyroManager::readRegister16(uint8_t reg) const {
  digitalWrite(_csPin, LOW);
  SPI.transfer(reg | 0x80);
  int16_t high = SPI.transfer(0x00);
  int16_t low = SPI.transfer(0x00);
  digitalWrite(_csPin, HIGH);
  return (high << 8) | low;
}

bool GyroManager::begin(uint8_t cs_pin) {
  _csPin = cs_pin;
  
  // Setup CS pin
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, HIGH);
  
  // Initialize SPI
  SPI.begin();
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3)); // 1 MHz, Mode 3
  
  delay(100); // Power-up delay
  
  // Check WHO_AM_I register
  uint8_t whoami = readRegister(REG_WHOAMI);
  DEBUG_SERIAL.print("Gyro WHO_AM_I: 0x");
  DEBUG_SERIAL.println(whoami, HEX);
  
  // Common WHO_AM_I values: 0x71 (MPU9250), 0xEA (MPU6500), 0x47 (ICM-42688-P)
  if (whoami != 0x71 && whoami != 0xEA && whoami != 0x47 && whoami != 0x68 && whoami != 0x70) {
    DEBUG_SERIAL.println("ERROR: Gyro not detected via SPI!");
    _initialized = false;
    return false;
  }
  
  DEBUG_SERIAL.println("ICM/MPU Gyroscope Found (SPI)!");
  
  // Reset device
  writeRegister(REG_PWR_MGMT_1, 0x80);
  delay(100);
  
  // Wake up device
  writeRegister(REG_PWR_MGMT_1, 0x01); // Auto select clock
  delay(10);
  
  // Configure gyroscope: ±2000 dps
  writeRegister(REG_GYRO_CONFIG, 0x18); // 0x18 = ±2000 deg/s
  
  // Configure accelerometer: ±16g
  writeRegister(REG_ACCEL_CONFIG, 0x18); // 0x18 = ±16g
  
  DEBUG_SERIAL.println("Gyroscope configured (SPI mode):");
  DEBUG_SERIAL.println("  Range: ±2000 deg/s");
  DEBUG_SERIAL.println("  Accel: ±16g");
  
  _initialized = true;
  
  // Perform initial calibration
  delay(100);
  calibrate();
  
  return true;
}

void GyroManager::update() {
  if (!_initialized) return;
  
  // Read gyroscope data (6 bytes starting at GYRO_XOUT_H)
  _rawGyroX = readRegister16(REG_GYRO_XOUT_H);
  _rawGyroY = readRegister16(REG_GYRO_XOUT_H + 2);
  _rawGyroZ = readRegister16(REG_GYRO_XOUT_H + 4);
  
  _lastUpdate = millis();
  
  // Convert to deg/s and apply calibration
  float rawYawRate = (_rawGyroZ / GYRO_SENSITIVITY) - _gyroOffsetZ;
  
  // Apply low-pass filter
  _filteredYawRate = FILTER_ALPHA * rawYawRate + (1.0 - FILTER_ALPHA) * _filteredYawRate;
}

float GyroManager::getYawRate() const {
  if (!_initialized) return 0.0;
  return _filteredYawRate;
}

float GyroManager::getPitchRate() const {
  if (!_initialized) return 0.0;
  return (_rawGyroY / GYRO_SENSITIVITY) - _gyroOffsetY;
}

float GyroManager::getRollRate() const {
  if (!_initialized) return 0.0;
  return (_rawGyroX / GYRO_SENSITIVITY) - _gyroOffsetX;
}

float GyroManager::getAccelX() const {
  if (!_initialized) return 0.0;
  _rawAccelX = readRegister16(REG_ACCEL_XOUT_H);
  return _rawAccelX / ACCEL_SENSITIVITY * 9.81; // Convert to m/s²
}

float GyroManager::getAccelY() const {
  if (!_initialized) return 0.0;
  _rawAccelY = readRegister16(REG_ACCEL_XOUT_H + 2);
  return _rawAccelY / ACCEL_SENSITIVITY * 9.81;
}

float GyroManager::getAccelZ() const {
  if (!_initialized) return 0.0;
  _rawAccelZ = readRegister16(REG_ACCEL_XOUT_H + 4);
  return _rawAccelZ / ACCEL_SENSITIVITY * 9.81;
}

bool GyroManager::isReady() const {
  return _initialized;
}

void GyroManager::calibrate() {
  if (!_initialized) return;
  
  DEBUG_SERIAL.println("Calibrating gyroscope - keep sensor stationary...");
  
  const int numSamples = 100;
  float sumX = 0, sumY = 0, sumZ = 0;
  
  for (int i = 0; i < numSamples; i++) {
    _rawGyroX = readRegister16(REG_GYRO_XOUT_H);
    _rawGyroY = readRegister16(REG_GYRO_XOUT_H + 2);
    _rawGyroZ = readRegister16(REG_GYRO_XOUT_H + 4);
    
    sumX += _rawGyroX / GYRO_SENSITIVITY;
    sumY += _rawGyroY / GYRO_SENSITIVITY;
    sumZ += _rawGyroZ / GYRO_SENSITIVITY;
    
    delay(10);
  }
  
  _gyroOffsetX = sumX / numSamples;
  _gyroOffsetY = sumY / numSamples;
  _gyroOffsetZ = sumZ / numSamples;
  
  DEBUG_SERIAL.print("Gyro calibration complete (");
  DEBUG_SERIAL.print(numSamples);
  DEBUG_SERIAL.println(" samples)");
  DEBUG_SERIAL.print("Offsets (deg/s): X=");
  DEBUG_SERIAL.print(_gyroOffsetX, 2);
  DEBUG_SERIAL.print(" Y=");
  DEBUG_SERIAL.print(_gyroOffsetY, 2);
  DEBUG_SERIAL.print(" Z=");
  DEBUG_SERIAL.println(_gyroOffsetZ, 2);
  
  // Reset filtered value
  _filteredYawRate = 0.0;
}

float GyroManager::getTemperature() const {
  if (!_initialized) return 0.0;
  int16_t rawTemp = readRegister16(REG_TEMP_OUT_H);
  // Temperature formula: temp_degC = (rawTemp / 340.0) + 36.53
  return (rawTemp / 340.0) + 36.53;
}
