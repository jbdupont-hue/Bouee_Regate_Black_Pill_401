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
  
  double getLatitude() const;
  double getLongitude() const;
  float getHDOP() const;
  float getSpeed() const;
  
  float getDistanceTo(double targetLat, double targetLon) const;
  float getBearingTo(double targetLat, double targetLon) const;
  
  TinyGPSPlus& getGPS();
  
private:
  TinyGPSPlus _gps;
  HardwareSerial* _serial;
};

#endif // GPS_H
