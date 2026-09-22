#ifndef SENSORMANAGER_H
#define SENSORMANAGER_H
#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>
#include "AppConfig.h"

struct GloboData {
  float tempAire = NAN;
  float tempGlobo = NAN;
  float mrt = NAN;
  float humedad = NAN;
  bool valid() const {
    return isfinite(tempAire) && isfinite(tempGlobo) &&
           isfinite(mrt) && isfinite(humedad);
  }
};

class SensorManager {
 private:
  OneWire oneWire;
  DallasTemperature globe;
  DeviceAddress address = {};
  bool present = false, pending = false;
  unsigned long conversionStart = 0, lastValid = 0;
  GloboData latest;
  double accAir = 0, accGlobe = 0, accMrt = 0, accHumidity = 0;
  uint32_t sampleCount = 0;
  bool readHdc(float &temperature, float &humidity);
  void invalidate();
 public:
  SensorManager();
  void begin();
  void read();
  void update();
  void cancel();
  static float calculateMRT(float tempGlobo, float tempAire);
  const GloboData &getLatest() const { return latest; }
  GloboData getAverages() const;
  bool readyToSend() const;
  void resetAccumulator();
  uint32_t getSampleCount() const { return sampleCount; }
};
extern SensorManager sensorManager;
#endif
