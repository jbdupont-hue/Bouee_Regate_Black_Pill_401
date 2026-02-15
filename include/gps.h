#ifndef GPS_H
#define GPS_H

#include <TinyGPS++.h>
#include <Arduino.h>

class GPSManager {
public:
  GPSManager();
  
  void begin(HardwareSerial& serial, uint32_t baudRate);
  void update();
  
  bool hasValidFix() const;
  bool hasGoodHDOP(float maxHDOP) const;
  
  uint32_t getCharsProcessed() const;
  uint32_t getValidSentences() const;
  
  double getLatitude() const;
  double getLongitude() const;
  float getHDOP() const;
  float getSpeed() const;
  uint32_t getSatellites() const;
  
  float getDistanceTo(double targetLat, double targetLon) const;
  float getBearingTo(double targetLat, double targetLon) const;
  
  TinyGPSPlus& getGPS();
  
private:
  mutable TinyGPSPlus _gps;
  HardwareSerial* _serial;
  
  // Custom GNSS sentence parsers for multi-band GPS
  TinyGPSCustom _gnssSatellites;
  TinyGPSCustom _gnssHDOP;
  
  uint32_t _satelliteCount;
  float _hdopValue;
  bool _customDataValid;

};

#endif // GPS_H