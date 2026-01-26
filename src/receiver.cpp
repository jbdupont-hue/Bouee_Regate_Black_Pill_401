#include "receiver.h"

// Static instance pointer for ISR
ReceiverPPM* ReceiverPPM::_instance = nullptr;

ReceiverPPM::ReceiverPPM(uint8_t ppmPin) 
  : _ppmPin(ppmPin), 
    _ppmIndex(0),
    _lastPulseTime(0) {
  for (int i = 0; i < MAX_CHANNELS; i++) {
    _ppm[i] = RC_NEUTRAL;
  }
}

void ReceiverPPM::begin() {
  _instance = this;
  pinMode(_ppmPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(_ppmPin), ppmISR, RISING);
}

uint16_t ReceiverPPM::getChannel(uint8_t channel) const {
  if (channel >= MAX_CHANNELS) return RC_NEUTRAL;
  return _ppm[channel];
}

float ReceiverPPM::getChannelNormalized(uint8_t channel) const {
  uint16_t raw = getChannel(channel);
  return (float)(raw - RC_NEUTRAL) / (RC_MAX - RC_NEUTRAL);
}

uint16_t ReceiverPPM::getThrottle() const {
  return getChannel(2); // Channel 3
}

uint16_t ReceiverPPM::getSteer() const {
  return getChannel(0); // Channel 1
}

uint16_t ReceiverPPM::getMode() const {
  return getChannel(4); // Channel 5
}

uint16_t ReceiverPPM::getCurrentType() const {
  return getChannel(5); // Channel 6
}

bool ReceiverPPM::isSignalValid() const {
  unsigned long now = millis();
  return (now - _lastPulseTime) < 100; // Signal valid if updated within 100ms
}

void ReceiverPPM::ppmISR() {
  if (_instance) {
    _instance->handlePPM();
  }
}

void ReceiverPPM::handlePPM() {
  static uint32_t lastPulse = 0;
  uint32_t now = micros();
  uint32_t pulseWidth = now - lastPulse;
  lastPulse = now;
  
  const uint32_t SYNC_THRESHOLD = 3000;
  const uint32_t MIN_PULSE = 600;
  const uint32_t MAX_PULSE = 2500;
  
  if (pulseWidth > 0x80000000UL) return; // Overflow check
  
  if (pulseWidth > SYNC_THRESHOLD) {
    // Sync pulse detected
    _ppmIndex = 0;
  } else if (_ppmIndex < MAX_CHANNELS) {
    // Valid channel pulse
    if (pulseWidth >= MIN_PULSE && pulseWidth <= MAX_PULSE) {
      _ppm[_ppmIndex++] = pulseWidth;
      _lastPulseTime = millis();
    }
  }
}
