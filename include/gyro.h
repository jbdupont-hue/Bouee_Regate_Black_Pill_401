#ifndef GYRO_H
#define GYRO_H

#include <Arduino.h>
#include <SPI.h>

/**
 * GyroManager - Manages ICM-42688-P / MPU9250 6-axis IMU
 * Provides gyroscope data for yaw rate damping and improved control
 */
class GyroManager {
public:
  GyroManager();
  
  /**
   * Initialize the gyroscope with SPI interface
   * @param cs_pin Chip Select pin for SPI
   * @return true if initialization successful
   */
  bool begin(uint8_t cs_pin);
  
  /**
   * Update sensor readings
   * Must be called regularly in main loop
   */
  void update();
  
  /**
   * Get current yaw rate (rotation around Z-axis)
   * @return Yaw rate in degrees per second
   */
  float getYawRate() const;
  
  /**
   * Get current pitch rate (rotation around Y-axis)
   * @return Pitch rate in degrees per second
   */
  float getPitchRate() const;
  
  /**
   * Get current roll rate (rotation around X-axis)  
   * @return Roll rate in degrees per second
   */
  float getRollRate() const;
  
  /**
   * Get accelerometer X-axis reading
   * @return Acceleration in m/s²
   */
  float getAccelX() const;
  
  /**
   * Get accelerometer Y-axis reading
   * @return Acceleration in m/s²
   */
  float getAccelY() const;
  
  /**
   * Get accelerometer Z-axis reading
   * @return Acceleration in m/s²
   */
  float getAccelZ() const;
  
  /**
   * Check if sensor is ready and providing valid data
   * @return true if sensor is operational
   */
  bool isReady() const;
  
  /**
   * Calibrate gyroscope bias (should be done when stationary)
   * Takes multiple samples and calculates average offset
   */
  void calibrate();
  
  /**
   * Get sensor temperature
   * @return Temperature in degrees Celsius
   */
  float getTemperature() const;

private:
  uint8_t _csPin;
  
  // ICM-42688-P register addresses
  static constexpr uint8_t REG_WHOAMI = 0x75;
  static constexpr uint8_t REG_PWR_MGMT_1 = 0x6B;
  static constexpr uint8_t REG_GYRO_CONFIG = 0x1B;
  static constexpr uint8_t REG_ACCEL_CONFIG = 0x1C;
  static constexpr uint8_t REG_GYRO_XOUT_H = 0x43;
  static constexpr uint8_t REG_ACCEL_XOUT_H = 0x3B;
  static constexpr uint8_t REG_TEMP_OUT_H = 0x41;
  
  // Configuration values
  static constexpr float GYRO_SENSITIVITY = 16.4; // LSB/(deg/s) for ±2000 dps
  static constexpr float ACCEL_SENSITIVITY = 2048.0; // LSB/g for ±16g
  
  // Raw sensor data
  mutable int16_t _rawGyroX, _rawGyroY, _rawGyroZ;
  mutable int16_t _rawAccelX, _rawAccelY, _rawAccelZ;
  mutable int16_t _rawTemp;
  
  float _gyroOffsetX;
  float _gyroOffsetY;
  float _gyroOffsetZ;
  
  bool _initialized;
  unsigned long _lastUpdate;
  
  // Low-pass filter for smoothing
  float _filteredYawRate;
  static constexpr float FILTER_ALPHA = 0.2; // Low-pass filter coefficient
  
  // SPI helper functions
  uint8_t readRegister(uint8_t reg) const;
  void writeRegister(uint8_t reg, uint8_t value) const;
  int16_t readRegister16(uint8_t reg) const;
};

#endif // GYRO_H
