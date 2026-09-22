#include "SensorManager.h"
#include <Wire.h>

SensorManager::SensorManager() : oneWire(AppConfig::GLOBE_PIN), globe(&oneWire) {}

float SensorManager::calculateMRT(float tg, float ta) {
  if (!isfinite(tg) || !isfinite(ta) || tg <= -273.15f || ta <= -273.15f) return NAN;
  // ISO 7726:1998, anexo B, ec. (7): D=0.15 m, emisividad=0.95.
  // Coeficiente simplificado 0.4e8. Solo conveccion natural.
  // Conversion Celsius-Kelvin con 273.15.
  const double delta = static_cast<double>(tg) - ta;
  const double kelvin = static_cast<double>(tg) + 273.15;
  const double radicand = pow(kelvin, 4.0) + 4.0e7 * pow(fabs(delta), 0.25) * delta;
  if (!isfinite(radicand) || radicand <= 0) return NAN;
  return static_cast<float>(pow(radicand, 0.25) - 273.15);
}

void SensorManager::begin() {
  globe.begin();
  globe.setWaitForConversion(false);
  present = globe.getDeviceCount() == 1 && globe.getAddress(address, 0) && address[0] == 0x28;
  if (present) globe.setResolution(address, 12);
  invalidate();
}

void SensorManager::invalidate() {
  latest = GloboData();
  resetAccumulator(); // No enviar muestras antiguas despues de un fallo.
}

void SensorManager::read() {
  if (pending) return;
  if (!present) {
    globe.begin();
    globe.setWaitForConversion(false);
    present = globe.getDeviceCount() == 1 && globe.getAddress(address, 0) && address[0] == 0x28;
    if (present) globe.setResolution(address, 12);
  }
  if (!present || globe.isParasitePowerMode() || !globe.requestTemperaturesByAddress(address)) {
    present = false;
    invalidate();
    return;
  }
  conversionStart = millis();
  pending = true;
}

bool SensorManager::readHdc(float &temperature, float &humidity) {
  // HDC1080: ambas magnitudes a 14 bits, heater apagado.
  Wire.beginTransmission(0x40);
  Wire.write(0x02);
  Wire.write(0x10);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;
  Wire.beginTransmission(0x40);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) return false;
  delay(15); // Dos conversiones HDC; DS18B20 es asincrono.
  if (Wire.requestFrom(uint8_t(0x40), uint8_t(4)) != 4) return false;
  const uint8_t tHigh = Wire.read();
  const uint8_t tLow = Wire.read();
  const uint8_t hHigh = Wire.read();
  const uint8_t hLow = Wire.read();
  const uint16_t rawT = (uint16_t(tHigh) << 8) | tLow;
  const uint16_t rawH = (uint16_t(hHigh) << 8) | hLow;
  if ((rawT & 3) != 0 || (rawH & 3) != 0) return false;
  temperature = rawT * (165.0f / 65536.0f) - 40.0f;
  humidity = rawH * (100.0f / 65536.0f);
  return temperature >= -40 && temperature <= 125 && humidity >= 0 && humidity <= 100;
}

void SensorManager::update() {
  if (!pending || millis() - conversionStart < 750UL) return;
  pending = false;
  GloboData next;
  next.tempGlobo = globe.getTempC(address);
  // 85 C tambien es el valor de encendido: descarte conservador.
  if (!isfinite(next.tempGlobo) || next.tempGlobo < -55 || next.tempGlobo > 125 ||
      next.tempGlobo == 85.0f || !readHdc(next.tempAire, next.humedad)) {
    present = false;
    invalidate();
    Serial.println("[Sensores] Lectura invalida; envio suspendido.");
    return;
  }
  next.mrt = calculateMRT(next.tempGlobo, next.tempAire);
  if (!next.valid()) { invalidate(); return; }
  latest = next;
  lastValid = millis();
  if (sampleCount == UINT32_MAX) resetAccumulator();
  accAir += next.tempAire;
  accGlobe += next.tempGlobo;
  accMrt += next.mrt; // MRT por muestra, no MRT de promedios.
  accHumidity += next.humedad;
  ++sampleCount;
  Serial.printf("Ta %.2f | Tg %.2f | MRT %.2f C | HR %.2f %%\n",
                next.tempAire, next.tempGlobo, next.mrt, next.humedad);
}

bool SensorManager::readyToSend() const {
  return sampleCount > 0 && latest.valid() &&
         millis() - lastValid <= 3UL * appConfig.intervaloLectura + 1000UL;
}

GloboData SensorManager::getAverages() const {
  GloboData avg;
  if (!readyToSend()) return avg;
  avg.tempAire = accAir / sampleCount;
  avg.tempGlobo = accGlobe / sampleCount;
  avg.mrt = accMrt / sampleCount;
  avg.humedad = accHumidity / sampleCount;
  return avg;
}

void SensorManager::cancel() { pending = false; invalidate(); }

void SensorManager::resetAccumulator() {
  accAir = accGlobe = accMrt = accHumidity = 0;
  sampleCount = 0;
}
