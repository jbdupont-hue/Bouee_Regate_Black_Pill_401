#ifndef MOTOR_CONTROLLER_H
#define MOTOR_CONTROLLER_H

#include <Servo.h>
#include <Arduino.h>

class MotorController {
public:
  MotorController(uint8_t portPin, uint8_t starboardPin);
  
  void begin();
  void stop();
  
  void drive(int throttle, int yaw);
  void setMotorValues(int portValue, int starboardValue);
  
  int getPortMotorValue() const;
  int getStarboardMotorValue() const;
  
  static const uint16_t MIN_PWM = 1000;
  static const uint16_t MAX_PWM = 2000;
  static const uint16_t NEUTRAL_PWM = 1500;
  
private:
  Servo _escPort;
  Servo _escStarboard;
  uint8_t _portPin;
  uint8_t _starboardPin;
  
  int _portMotorValue;
  int _starboardMotorValue;
};

#endif // MOTOR_CONTROLLER_H
