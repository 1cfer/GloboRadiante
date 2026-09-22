#ifndef APPCONFIG_H
#define APPCONFIG_H
#include <Arduino.h>
#include <Preferences.h>

class AppConfig {
 private:
  Preferences prefs;
  void defaults();
 public:
  String serverUrl, hostname, tokenUrl, clientId, clientSecret;
  String keyrockUser, keyrockPass, agentUrl;
  unsigned long intervaloEnvio, intervaloLectura, intervaloReintento;
  unsigned long tiempoInactividad;
  bool skipToken, useAgent, isConfigured;
  static constexpr uint8_t SDA_PIN = 21;
  static constexpr uint8_t SCL_PIN = 22;
  static constexpr uint8_t GLOBE_PIN = 27;
  static constexpr uint8_t BUTTON_PIN = 32;
  static constexpr unsigned long VIEW_INTERVAL_MS = 5000;
  AppConfig();
  void begin();
  void save();
  void reset();
};
extern AppConfig appConfig;
#endif
