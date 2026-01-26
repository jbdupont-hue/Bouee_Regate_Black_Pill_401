#ifndef RECEIVER_H
#define RECEIVER_H

#include <Arduino.h>

class ReceiverPPM {
public:
  ReceiverPPM(uint8_t ppmPin);
  
  void begin();
  
  uint16_t getChannel(uint8_t channel) const;
  float getChannelNormalized(uint8_t channel) const;
  
  uint16_t getThrottle() const;
  uint16_t getSteer() const;
  uint16_t getMode() const;
  uint16_t getCurrentType() const;
  
  bool isSignalValid() const;
  
  static const uint8_t MAX_CHANNELS = 8;
  static const uint16_t RC_MIN = 1000;
  static const uint16_t RC_MAX = 2000;
  static const uint16_t RC_NEUTRAL = 1500;
  
private:
  uint8_t _ppmPin;
  volatile uint16_t _ppm[MAX_CHANNELS];
  volatile uint8_t _ppmIndex;
  unsigned long _lastPulseTime;
  
  static ReceiverPPM* _instance;
  static void ppmISR();
  void handlePPM();
};

#endif // RECEIVER_H
