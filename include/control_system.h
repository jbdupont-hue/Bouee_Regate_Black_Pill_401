#ifndef CONTROL_SYSTEM_H
#define CONTROL_SYSTEM_H

#include <Arduino.h>

enum OperationMode {
  MODE_AUTONOMOUS = 0,
  MODE_MANUAL = 1
};

enum WaterCondition {
  CALM_WATERS = 0,
  LOW_CURRENT = 1,
  STRONG_CURRENT = 2
};

struct ControlParameters {
  float holdRadius;
  float distanceGain;
  float yawGain;
  float maxThrust;
  float minThrust;
  float yawRateGain;  // Gyro yaw rate damping gain
};

class ControlSystem {
public:
  ControlSystem();
  
  void setMode(OperationMode mode);
  void setWaterCondition(WaterCondition condition);
  
  OperationMode getMode() const;
  WaterCondition getWaterCondition() const;
  
  const ControlParameters& getParameters() const;
  
  void updateTargetPosition(double lat, double lon);
  double getTargetLat() const;
  double getTargetLon() const;
  
  void updateCurrentEstimate(float strength, float direction, bool isValid);
  float getCurrentStrength() const;
  float getCurrentDirection() const;
  bool isCurrentValid() const;
  
  float headingError(float desired, float current) const;
  
private:
  OperationMode _mode;
  WaterCondition _waterCondition;
  ControlParameters _parameters;
  
  double _targetLat;
  double _targetLon;
  
  float _currentStrength;
  float _currentDirection;
  bool _currentValid;
  
  void updateParameters();
};

#endif // CONTROL_SYSTEM_H