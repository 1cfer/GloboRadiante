#include "AppConfig.h"

AppConfig::AppConfig() { defaults(); }

void AppConfig::defaults() {
  hostname = "globo-mrt-01";
  serverUrl = "";  // Configurar URL completa /v2/entities/ID/attrs.
  tokenUrl = "";   // HTTPS: TokenManager original utiliza cliente TLS.
  clientId = "";
  clientSecret = "";
  keyrockUser = "";
  keyrockPass = "";
  agentUrl = "";
  intervaloEnvio = 15000;
  intervaloLectura = 2000;
  intervaloReintento = 20000;
  tiempoInactividad = 180000;
  skipToken = false;
  useAgent = false;
  isConfigured = false;
}

void AppConfig::begin() {
  // Namespace propio: no heredar la identidad ni credenciales TARS.
  prefs.begin("globomrt", false);
  isConfigured = prefs.getBool("isConfigured", isConfigured);
  hostname = prefs.getString("hostname", hostname);
  serverUrl = prefs.getString("serverUrl", serverUrl);
  tokenUrl = prefs.getString("tokenUrl", tokenUrl);
  clientId = prefs.getString("clientId", clientId);
  clientSecret = prefs.getString("clientSecret", clientSecret);
  keyrockUser = prefs.getString("keyrockUser", keyrockUser);
  keyrockPass = prefs.getString("keyrockPass", keyrockPass);
  agentUrl = prefs.getString("agentUrl", agentUrl);
  intervaloEnvio = prefs.getULong("intervaloEnvio", intervaloEnvio);
  intervaloLectura = prefs.getULong("intervaloLectura", intervaloLectura);
  intervaloReintento = prefs.getULong("intervaloReintento", intervaloReintento);
  tiempoInactividad = prefs.getULong("tiempoInactividad", tiempoInactividad);
  skipToken = prefs.getBool("skipToken", skipToken);
  useAgent = prefs.getBool("useAgent", useAgent);
  prefs.end();
  if (intervaloLectura < 1000 || intervaloLectura > 3600000UL) intervaloLectura = 2000;
  if (intervaloEnvio < 1000 || intervaloEnvio > 86400000UL) intervaloEnvio = 15000;
  if (intervaloReintento < 1000 || intervaloReintento > 86400000UL) intervaloReintento = 20000;
  Serial.printf("[AppConfig] %s, configurado: %s\n", hostname.c_str(), isConfigured ? "si" : "no");
}

void AppConfig::save() {
  prefs.begin("globomrt", false);
  prefs.putBool("isConfigured", true);
  prefs.putString("hostname", hostname);
  prefs.putString("serverUrl", serverUrl);
  prefs.putString("tokenUrl", tokenUrl);
  prefs.putString("clientId", clientId);
  prefs.putString("clientSecret", clientSecret);
  prefs.putString("keyrockUser", keyrockUser);
  prefs.putString("keyrockPass", keyrockPass);
  prefs.putString("agentUrl", agentUrl);
  prefs.putULong("intervaloEnvio", intervaloEnvio);
  prefs.putULong("intervaloLectura", intervaloLectura);
  prefs.putULong("intervaloReintento", intervaloReintento);
  prefs.putULong("tiempoInactividad", tiempoInactividad);
  prefs.putBool("skipToken", skipToken);
  prefs.putBool("useAgent", useAgent);
  prefs.end();
  isConfigured = true;
}

void AppConfig::reset() {
  prefs.begin("globomrt", false);
  prefs.clear();
  prefs.end();
  defaults();
}
