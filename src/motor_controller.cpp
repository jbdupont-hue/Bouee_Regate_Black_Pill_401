#include "motor_controller.h"

MotorController::MotorController(uint8_t portPin, uint8_t starboardPin)
  : _portPin(portPin), 
    _starboardPin(starboardPin),
    _portMotorValue(NEUTRAL_PWM),
    _starboardMotorValue(NEUTRAL_PWM) {
}

void MotorController::begin() {
  _escPort.attach(_portPin);
  _escStarboard.attach(_starboardPin);
  stop();
}

void MotorController::stop() {
  _escPort.writeMicroseconds(NEUTRAL_PWM);
  _escStarboard.writeMicroseconds(NEUTRAL_PWM);
  _portMotorValue = NEUTRAL_PWM;
  _starboardMotorValue = NEUTRAL_PWM;
}

void MotorController::drive(int throttle, int yaw) {
  int portValue = constrain(NEUTRAL_PWM + throttle + yaw, MIN_PWM, MAX_PWM);
  int starboardValue = constrain(NEUTRAL_PWM + throttle - yaw, MIN_PWM, MAX_PWM);
  setMotorValues(portValue, starboardValue);
}

void MotorController::setMotorValues(int portValue, int starboardValue) {
  _portMotorValue = constrain(portValue, MIN_PWM, MAX_PWM);
  _starboardMotorValue = constrain(starboardValue, MIN_PWM, MAX_PWM);
  
  _escPort.writeMicroseconds(_portMotorValue);
  _escStarboard.writeMicroseconds(_starboardMotorValue);
}

int MotorController::getPortMotorValue() const {
  return _portMotorValue;
}

int MotorController::getStarboardMotorValue() const {
  return _starboardMotorValue;
}
