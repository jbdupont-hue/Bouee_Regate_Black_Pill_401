#include "control_system.h"
#include "config.h"

ControlSystem::ControlSystem()
  : _mode(MODE_AUTONOMOUS),
    _waterCondition(CALM_WATERS),
    _targetLat(0), _targetLon(0),
    _currentStrength(0), _currentDirection(0),
    _currentValid(false) {
  updateParameters();
}

void ControlSystem::setMode(OperationMode mode) {
  _mode = mode;
}

void ControlSystem::setWaterCondition(WaterCondition condition) {
  if (_waterCondition != condition) {
    _waterCondition = condition;
    updateParameters();
  }
}

OperationMode ControlSystem::getMode() const {
  return _mode;
}

WaterCondition ControlSystem::getWaterCondition() const {
  return _waterCondition;
}

const ControlParameters& ControlSystem::getParameters() const {
  return _parameters;
}

void ControlSystem::updateTargetPosition(double lat, double lon) {
  _targetLat = lat;
  _targetLon = lon;
}

double ControlSystem::getTargetLat() const {
  return _targetLat;
}

double ControlSystem::getTargetLon() const {
  return _targetLon;
}

void ControlSystem::updateCurrentEstimate(float strength, float direction, bool isValid) {
  _currentStrength = strength;
  _currentDirection = direction;
  _currentValid = isValid;
}

float ControlSystem::getCurrentStrength() const {
  return _currentStrength;
}

float ControlSystem::getCurrentDirection() const {
  return _currentDirection;
}

bool ControlSystem::isCurrentValid() const {
  return _currentValid;
}

float ControlSystem::headingError(float desired, float current) const {
  float error = desired - current;
  while (error > 180.0) error -= 360.0;
  while (error < -180.0) error += 360.0;
  return error;
}

void ControlSystem::updateParameters() {
  switch (_waterCondition) {
    case CALM_WATERS:
      _parameters.holdRadius = HOLD_RADIUS_CALM;
      _parameters.distanceGain = DIST_GAIN_CALM;
      _parameters.yawGain = YAW_GAIN_CALM;
      _parameters.maxThrust = MAX_THRUST_CALM;
      break;
      
    case LOW_CURRENT:
      _parameters.holdRadius = HOLD_RADIUS_LOW_CURRENT;
      _parameters.distanceGain = DIST_GAIN_LOW_CURRENT;
      _parameters.yawGain = YAW_GAIN_LOW_CURRENT;
      _parameters.maxThrust = MAX_THRUST_LOW_CURRENT;
      break;
      
    case STRONG_CURRENT:
      _parameters.holdRadius = HOLD_RADIUS_STRONG_CURRENT;
      _parameters.distanceGain = DIST_GAIN_STRONG_CURRENT;
      _parameters.yawGain = YAW_GAIN_STRONG_CURRENT;
      _parameters.maxThrust = MAX_THRUST_STRONG_CURRENT;
      break;
  }
  
  _parameters.minThrust = MIN_THRUST;
}
