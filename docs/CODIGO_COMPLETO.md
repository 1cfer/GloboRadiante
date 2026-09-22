# Globo Radiante — código completo de archivos adaptados

Base: https://github.com/1cfer/Tars, commit `2439dd5a2378ba45309707a6d51de8fe334ec83c`.

Abrir `GloboRadiante/main/main.ino` del ZIP. Los diez archivos base no modificados también están incluidos en el ZIP. Este documento muestra íntegros los nueve archivos solicitados, sin recortes. Compilación ESP32 y validación en hardware pendientes; consultar README.

## main.ino

```cpp
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 
#include <ArduinoJson.h> 
#include <EEPROM.h> 
#include <ESPmDNS.h> 
#include <HTTPClient.h> 
#include <WebServer.h> 
#include <WiFi.h> 
#include <Wire.h> 

#include "AppConfig.h" 
#include "ButtonHandler.h" 
#include "DisplayManager.h" 
#include "Estados.h" 
#include "PayloadBuilder.h" 
#include "SensorManager.h" 
#include "State.h" 
#include "StateMachine.h" 
#include "TokenManager.h" 
#include "WiFiManager.h"

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define BUTTON_PIN AppConfig::BUTTON_PIN 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1); 
WebServer server(80); 
AppConfig appConfig; 
WiFiManager wifiManager; 
TokenManager tokenManager; 
ButtonHandler buttonHandler(BUTTON_PIN); 
StateMachine stateMachine; 
SensorManager sensorManager; 

SET_LOOP_TASK_STACK_SIZE(16384); 

// Función para mostrar la portada institucional y limpiar píxeles parásitos de arranque
void displaySplash() {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  
  // Encabezado institucional
  display.setTextSize(2);
  display.setCursor(18, 8);
  display.println("Globo");
  
  // Nombre configurado del dispositivo (ej: tars1, moreha1, etc.)
  display.setTextSize(1);
  display.setCursor(15, 30);
  if (appConfig.hostname.length() > 0) {
    display.print("Nodo: ");
    display.println(appConfig.hostname);
  } else {
    display.println("GLOBO RADIANTE");
  }
  
  display.setCursor(15, 48);
  display.println("Iniciando nodo...");
  
  display.display();
  delay(1500); // Pausa para visualizar la portada en el arranque
}

void setup() { 
  Serial.begin(115200); 

  Wire.begin(AppConfig::SDA_PIN, AppConfig::SCL_PIN);
  Wire.setTimeOut(50); 
  Wire.setClock(100000); 

  // 1. Cargar configuración desde memoria EEPROM/NVS
  appConfig.begin();
  WiFi.setHostname(appConfig.hostname.c_str());

  // 2. Inicializar pantalla OLED y mostrar portada de inmediato
  if (display.begin(SSD1306_SWITCHCAPVCC, 0x3C, true, false)) {
    displaySplash();
  } else {
    Serial.println("[DISPLAY] Error al iniciar SSD1306");
    while (true) delay(1000);
  }

  // 3. Inicializar sensores periféricos
  // Sensores inicializados por SensorManager.

  // 4. Inicializar subsistemas y máquina de estados
  buttonHandler.begin();
  sensorManager.begin();
  stateMachine.begin(new EstadoINICIO()); 
}

void loop() { 
  ButtonHandler::Event event = buttonHandler.update();

  if (event != ButtonHandler::NONE) { 
    // Actualizamos el reloj de interacción siempre que se presione el botón
    stateMachine.clocks.ultima_interaccion = millis();
    
    // Si la pantalla estaba apagada, la despertamos
    if (!stateMachine.isDisplayOn) {
      stateMachine.isDisplayOn = true;
      stateMachine.needsUpdate = true;
      
      // Si el evento fue un clic corto para despertar la pantalla, 
      // lo anulamos para que NO avance a la siguiente vista de sensores
      if (event == ButtonHandler::SHORT_PRESS) {
        event = ButtonHandler::NONE; 
      }
    }
  }

  // Pasamos el evento a los estados
  stateMachine.currentEvent = event;
  stateMachine.update(); 
  stateMachine.currentEvent = ButtonHandler::NONE; // Limpiamos el evento

  yield(); // Alimentar el Watchdog de FreeRTOS
}
```

## AppConfig.h

```cpp
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
  static constexpr uint8_t BUTTON_PIN = 0;
  static constexpr unsigned long VIEW_INTERVAL_MS = 5000;
  AppConfig();
  void begin();
  void save();
  void reset();
};
extern AppConfig appConfig;
#endif
```

## AppConfig.cpp

```cpp
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
```

## SensorManager.h

```cpp
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
```

## SensorManager.cpp

```cpp
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
```

## DisplayManager.h

```cpp
#ifndef DISPLAYMANAGER_H
#define DISPLAYMANAGER_H
#include <Arduino.h>

void drawAllSensors();
void drawMrtScreen();
void drawCountdownScreen();
void drawIpScreen();
void updateDisplay();
void displayStateInfo(const char* estado);
void displayDeveloperInfo();

// Nuevas funciones para el menú de WiFi interactivo
void displayWiFiList(int selected, int offset, int total);
void displayAPAlert();

#endif
```

## DisplayManager.cpp

```cpp
#include "DisplayManager.h"
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include "AppConfig.h"
#include "StateMachine.h"
#include "WiFiManager.h"
#include "SensorManager.h"

extern Adafruit_SSD1306 display;
extern StateMachine stateMachine;
extern WiFiManager wifiManager;

// =========================================================
// HEADER CON PAGINACION INTEGRADA
// =========================================================
void drawScreenHeader(const char* title, int page = -1, int total = -1) {
    display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setTextSize(1);
    display.setCursor(2, 2);
    display.print(title);
    
    if (page >= 0 && total >= 0) {
        display.setCursor(95, 2);
        display.print("["); display.print(page + 1); display.print("/"); display.print(total); display.print("]");
    } else {
        display.setCursor(105, 2);
        if (WiFi.status() == WL_CONNECTED) display.print("WiFi");
        else display.print("--");
    }
    display.setTextColor(SSD1306_WHITE);
}

// =========================================================
// PANTALLAS DE MODO LECTURA
// =========================================================
static void printValue(float value) {
    if (isfinite(value)) display.print(value, 1);
    else display.print("--");
}

void drawAllSensors() {
    drawScreenHeader("GLOBO - Datos", 0, 4);
    const GloboData &v = sensorManager.getLatest();
    display.setTextSize(1);
    display.setCursor(0, 17);
    display.print("Ta: "); printValue(v.tempAire); display.print(" C");
    display.setCursor(0, 32);
    display.print("Tg: "); printValue(v.tempGlobo); display.print(" C");
    display.setCursor(0, 47);
    display.print("HR: "); printValue(v.humedad); display.print(" %");
}

void drawMrtScreen() {
    drawScreenHeader("MRT", 1, 4);
    display.setCursor(0, 23);
    display.setTextSize(3);
    printValue(sensorManager.getLatest().mrt);
    display.setTextSize(1);
    display.setCursor(0, 54);
    display.print("C / conveccion natural");
}

void drawCountdownScreen() {
    drawScreenHeader("PROXIMO ENVIO", 2, 4);
    const int32_t remaining = static_cast<int32_t>(stateMachine.clocks.proximo_envio - millis());
    display.setTextSize(2);
    display.setCursor(0, 24);
    if (!stateMachine.flags.envio_programado) display.print("--");
    else display.print(remaining > 0 ? (remaining + 999) / 1000 : 0);
    display.print(" s");
    display.setTextSize(1);
    display.setCursor(0, 52);
    display.print(sensorManager.getLatest().valid() ? "Lecturas validas" : "Esperando sensores");
}

void drawIpScreen() {
    drawScreenHeader("RED WIFI", 3, 4);
    display.setTextSize(1);
    display.setCursor(0, 20);
    display.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "Sin conexion");
    display.setCursor(0, 36);
    display.print(WiFi.SSID().substring(0, 21));
    display.setCursor(0, 52);
    display.print(appConfig.hostname.substring(0, 21));
}

void updateDisplay() {
    if (!stateMachine.isDisplayOn) return;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);

    switch (stateMachine.screenMode) {
        case 0: drawAllSensors(); break;
        case 1: drawMrtScreen(); break;
        case 2: drawCountdownScreen(); break;
        case 3: drawIpScreen(); break;
    }
    display.display();
}

// =========================================================
// PANTALLAS DEL SISTEMA Y MENÚS
// =========================================================
void displayStateInfo(const char* estado) {
    display.clearDisplay();
    drawScreenHeader("ESTADO SISTEMA");
    
    display.setCursor(0, 25);
    display.print("MODO: "); display.println(estado);
    
    display.setCursor(0, 45);
    display.print("IP: ");
    if (WiFi.status() == WL_CONNECTED) display.println(wifiManager.getIP());
    else display.println("Desconectado");
    
    display.display();
}

void displayDeveloperInfo() {
    display.clearDisplay();
    drawScreenHeader("MODO DEVELOPER");

    display.setTextSize(1);
    display.setCursor(0, 14); display.print("IP: "); display.println(wifiManager.getIP());

    display.setCursor(0, 24);
    if (WiFi.status() == WL_CONNECTED) {
        display.print("WIFI: ");
        String ssid = WiFi.SSID();
        if (ssid.length() > 15) ssid = ssid.substring(0, 15);
        display.println(ssid);
    } else if (WiFi.getMode() == WIFI_AP || WiFi.getMode() == WIFI_AP_STA) {
        display.print("AP: TARS-");
        String h = appConfig.hostname;
        if (h.length() > 10) h = h.substring(0, 10);
        display.println(h);
    } else {
        display.println("Red: Desconectado");
    }

    display.setCursor(0, 34);
    display.print("Ag:"); display.print(appConfig.useAgent ? "ON " : "OFF");
    display.print(" Tk:"); display.print(appConfig.skipToken ? "OFF" : "ON ");
    display.print(" T:");
    if (appConfig.intervaloEnvio >= 60000) { display.print(appConfig.intervaloEnvio / 60000); display.println("m"); } 
    else { display.print(appConfig.intervaloEnvio / 1000); display.println("s"); }

    display.setCursor(0, 44);
    String hostLine = appConfig.hostname;
    if (hostLine.length() > 12) hostLine = hostLine.substring(0, 12);
    display.print("Host: "); display.print(hostLine); display.println(".local");

    display.setCursor(0, 54);
    display.print("S:"); display.print(WiFi.RSSI());
    display.print("dBm | Mem:"); display.print(ESP.getFreeHeap() / 1024); display.println("K");

    display.display();
}

void displayWiFiList(int selected, int offset, int total) {
    display.clearDisplay();
    display.setTextSize(1);
    
    display.fillRect(0, 0, 128, 11, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(2, 2);
    display.print("REDES WIFI: "); display.println(total);
    display.setTextColor(SSD1306_WHITE);

    int maxLines = 5; 
    for (int i = 0; i < maxLines && (i + offset) < total; i++) {
        int netIdx = i + offset;
        int yPos = 14 + (i * 10);
        
        display.setCursor(0, yPos);
        if (netIdx == selected) display.print(">");
        else display.print(" ");
        
        String ssid = WiFi.SSID(netIdx);
        if (wifiManager.hasCredentialsFor(ssid)) {
            display.print("*"); 
        } else { 
            display.print(" "); 
        }
        
        if (ssid.length() > 17) ssid = ssid.substring(0, 17);
        display.print(ssid);
    }
    
    // Barra de scroll a la derecha
    if (total > maxLines) {
        int barHeight = (maxLines * 48) / total;
        if (barHeight < 5) barHeight = 5;
        int barY = 13 + (offset * (48 - barHeight)) / (total - maxLines);
        display.drawRect(125, 13, 3, 50, SSD1306_WHITE);
        display.fillRect(125, barY, 3, barHeight, SSD1306_WHITE);
    }
    display.display();
}

void displayAPAlert() {
    display.clearDisplay();
    display.setTextSize(1);
    
    display.fillRect(0, 0, 128, 13, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(12, 3); 
    display.println("! SIN CONTRASENA !"); 
    
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 17); display.println("Red no almacenada");
    display.setCursor(0, 30); display.print("Configurar desde:");
    
    display.setCursor(0, 42); 
    display.print("Wi-Fi: TARS-"); display.println(appConfig.hostname);
    
    display.setCursor(0, 53); 
    display.print("http://"); display.print(appConfig.hostname); display.println(".local");
    
    display.display();
}
```

## Estados.h

```cpp
#ifndef ESTADOS_H
#define ESTADOS_H

#include <Arduino.h>
#include <WebServer.h>
#include <Adafruit_SSD1306.h>

#include "AppConfig.h"
#include "DisplayManager.h"
#include "SensorManager.h"
#include "State.h"
#include "StateMachine.h"
#include "TokenManager.h"
#include "WiFiManager.h"

// Objetos definidos en main.ino
extern Adafruit_SSD1306 display;
extern WebServer server;

// ========================================
// ESTADO INICIO
// ========================================
class EstadoINICIO : public State {
private:
    bool firstRun = true;
public:
    void onEnter() override;
    void execute() override;
    void onExit() override;
    const char* getName() override;
};

// ========================================
// ESTADO LECTURA
// ========================================
class EstadoLECTURA : public State {
private:
    unsigned long lastViewChange = 0;
    unsigned long lastRefresh = 0;
public:
    void onEnter() override;
    void execute() override;
    void onExit() override;
    const char* getName() override;
};

// ========================================
// ESTADO ENVIO
// ========================================
class EstadoENVIO : public State {
public:
    void onEnter() override;
    void execute() override;
    void onExit() override;
    const char* getName() override;
};

// ========================================
// ESTADO DESARROLLADOR
// ========================================
class EstadoDESARROLLADOR : public State {
private:
    bool primera_vez = true;
    unsigned long lastDisplayRefresh = 0;
    int subMode = 0; // 0=Info, 1=Scan, 2=Lista, 3=Alerta AP
    int selectedNetwork = 0;
    int scrollOffset = 0;
    int totalNetworks = 0;
public:
    void onEnter() override;
    void execute() override;
    void onExit() override;
    const char* getName() override;
};

#endif
```

## Estados.cpp

```cpp
#include "Estados.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <ArduinoJson.h>

#include "AppConfig.h" 
#include "DevWebOTA.h" 
#include "TokenManager.h" 
#include "WiFiManager.h"
#include "DisplayManager.h"

DevWebOTA *devWeb = nullptr;
static DNSServer captiveDns;
extern Adafruit_SSD1306 display;
extern WebServer server;
extern SensorManager sensorManager;

// ======================================== 
// IMPLEMENTACIÓN ESTADO INICIO 
// ========================================
void EstadoINICIO::onEnter() { 
    Serial.println("===Entrando en Estado INICIO==="); 
    statemachine->flags.inicio = true; 
    firstRun = true; 
}

void EstadoINICIO::execute() { 
    if (!firstRun) return;

    if (!appConfig.isConfigured) { 
        Serial.println("[INICIO] Primer boot detectado — forzando modo DESARROLLADOR para setup"); 
        statemachine->flags.dev = true; 
        statemachine->ChangeState(new EstadoDESARROLLADOR()); 
        return; 
    }

    if (statemachine->flags.dev) { 
        Serial.println("Cambiando a Estado DESARROLLADOR desde INICIO"); 
        statemachine->ChangeState(new EstadoDESARROLLADOR()); 
        return; 
    }

    unsigned long now = millis(); 
    Serial.println("Estado: INICIO");

    if (wifiManager.connect(16)) { 
        Serial.println("[INICIO] WiFi listo para envío de datos"); 
        MDNS.begin(appConfig.hostname.c_str()); 
        Serial.printf("[mDNS] Activo en http://%s.local\n", appConfig.hostname.c_str()); 
    } else { 
        Serial.println("[INICIO] Sin WiFi, autoReconnect activo"); 
    }

    statemachine->flags.inicio = false; 
    statemachine->clocks.proximo_envio = now + appConfig.intervaloEnvio; 
    statemachine->flags.envio_programado = true;

    firstRun = false;
    statemachine->ChangeState(new EstadoLECTURA());
    return; 
}

void EstadoINICIO::onExit() { 
    Serial.println("===Saliendo de Estado INICIO==="); 
    statemachine->flags.inicio = false; 
    firstRun = false; 
}

const char *EstadoINICIO::getName() { return "INICIO"; }

// ======================================== 
// IMPLEMENTACIÓN ESTADO LECTURA 
// ========================================
void EstadoLECTURA::onEnter() {
    lastViewChange = lastRefresh = millis();
    statemachine->needsUpdate = true; 
    Serial.println("===Entrando en Estado LECTURA==="); 
    statemachine->flags.lectura = true; 
    statemachine->flags.inicio = false; 
    statemachine->flags.envio = false; 
}

void EstadoLECTURA::execute() { 
    unsigned long now = millis();
    ButtonHandler::Event btn = statemachine->currentEvent;

    sensorManager.update();
    if (statemachine->isDisplayOn && now - lastViewChange >= AppConfig::VIEW_INTERVAL_MS) {
        statemachine->screenMode = (statemachine->screenMode + 1) % 4;
        statemachine->needsUpdate = true;
        lastViewChange = now;
    }
    if (now - lastRefresh >= 250UL) {
        statemachine->needsUpdate = true;
        lastRefresh = now;
    }

    // Clic corto: siguiente vista.
    if (btn == ButtonHandler::SHORT_PRESS) {
        lastViewChange = now;
        statemachine->screenMode = (statemachine->screenMode + 1) % 4; 
        statemachine->needsUpdate = true;
    } else if (btn == ButtonHandler::VERY_LONG_PRESS_4S) {
        statemachine->ChangeState(new EstadoDESARROLLADOR());
        return;
    }

    if (statemachine->isDisplayOn && (now - statemachine->clocks.ultima_interaccion > appConfig.tiempoInactividad)) { 
        display.clearDisplay(); 
        display.display(); 
        statemachine->isDisplayOn = false; 
    }

    if (statemachine->needsUpdate && statemachine->isDisplayOn) { 
        updateDisplay(); 
        statemachine->needsUpdate = false; 
    }

    if (now - statemachine->clocks.tiempo_lectura >= appConfig.intervaloLectura) { 
        sensorManager.read(); 
        if (statemachine->isDisplayOn) { updateDisplay(); } 
        statemachine->clocks.tiempo_lectura = now; 
    }

    if (statemachine->flags.envio_programado && (long)(now - statemachine->clocks.proximo_envio) >= 0) { 
        statemachine->ChangeState(new EstadoENVIO()); 
        return; 
    }

    wifiManager.maintainConnection(); 
}

void EstadoLECTURA::onExit() { 
    Serial.println("===Saliendo de Estado LECTURA==="); 
    statemachine->flags.lectura = false; 
}

const char *EstadoLECTURA::getName() { return "LECTURA"; }

// ======================================== 
// IMPLEMENTACIÓN ESTADO ENVIO 
// ========================================
void EstadoENVIO::onEnter() { 
    Serial.println("=== ENTRANDO A ESTADO: ENVIO ==="); 
    statemachine->flags.envio = true; 
    statemachine->flags.inicio = false; 
    statemachine->flags.lectura = false;

    if (statemachine->isDisplayOn) { 
        display.clearDisplay(); 
        displayStateInfo("ENVIO"); 
        display.display(); 
    } 
}

void EstadoENVIO::execute() { 
    unsigned long now = millis();
    ButtonHandler::Event btn = statemachine->currentEvent;

    // Permitir ir al modo desarrollador si se requiere interrumpir
    if (btn == ButtonHandler::VERY_LONG_PRESS_4S) {
        statemachine->ChangeState(new EstadoDESARROLLADOR());
        return;
    }

    if (!wifiManager.isConnected()) { 
        Serial.println("[ENVIO] Sin conexion WiFi, posponiendo reintento"); 
        statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento; 
        statemachine->flags.envio_programado = true; 
        statemachine->ChangeState(new EstadoLECTURA()); 
        return; 
    }

    if (!sensorManager.readyToSend() || appConfig.serverUrl.length() == 0 ||
        (!appConfig.skipToken && !appConfig.tokenUrl.startsWith("https://"))) {
        Serial.println("[ENVIO] Revisar sensores, serverUrl y tokenUrl HTTPS.");
        statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento;
        statemachine->flags.envio_programado = true;
        statemachine->ChangeState(new EstadoLECTURA());
        return;
    }

    if (!appConfig.skipToken && !tokenManager.ensureValidToken()) { 
        Serial.println("[ENVIO] No se pudo obtener token valido, programando reintento rapido"); 
        statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento; 
        statemachine->flags.envio_programado = true; 
        statemachine->ChangeState(new EstadoLECTURA()); 
        return; 
    }

    GloboData avg = sensorManager.getAverages();
    if (!avg.valid()) {
        statemachine->clocks.proximo_envio = millis() + appConfig.intervaloReintento;
        statemachine->ChangeState(new EstadoLECTURA());
        return;
    }
    JsonDocument doc;
    doc["tempAire"]["type"] = "Float";
    doc["tempAire"]["value"] = avg.tempAire;
    doc["tempGlobo"]["type"] = "Float";
    doc["tempGlobo"]["value"] = avg.tempGlobo;
    doc["mrt"]["type"] = "Float";
    doc["mrt"]["value"] = avg.mrt;
    doc["humedad"]["type"] = "Float";
    doc["humedad"]["value"] = avg.humedad;
    String payload;
    serializeJson(doc, payload); 
    Serial.println("[ENVIO] Payload JSON:"); 
    Serial.println(payload);

    WiFiClient plainClient;
    WiFiClientSecure secureClient;
    secureClient.setInsecure(); // Comportamiento TLS heredado; ver README.
    WiFiClient &client = appConfig.serverUrl.startsWith("https://")
        ? static_cast<WiFiClient &>(secureClient) : plainClient;

    HTTPClient http; 
    http.setTimeout(10000); 
    
    if (!http.begin(client, appConfig.serverUrl)) {
        Serial.println("[ENVIO] Error al inicializar conexion HTTP con el servidor");
        client.stop();
        statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento; 
        statemachine->flags.envio_programado = true; 
        statemachine->ChangeState(new EstadoLECTURA()); 
        return;
    }

    http.addHeader("Content-Type", "application/json"); 
    if (!appConfig.skipToken) { 
        http.addHeader("Authorization", "Bearer " + tokenManager.getToken()); 
    }

    int httpResponseCode = http.PATCH(payload);
    now = millis(); // Reprogramar desde el final del intento HTTP.

    if (httpResponseCode >= 200 && httpResponseCode < 300) { 
        Serial.printf("[ENVIO] ✓ Exitoso, codigo: %d\n", httpResponseCode); 
        sensorManager.resetAccumulator();
        statemachine->clocks.proximo_envio = now + appConfig.intervaloEnvio; 
    } else if (httpResponseCode == 401 || httpResponseCode == 403 || httpResponseCode == 302) { 
        Serial.printf("[ENVIO] Wilma rechazo acceso (HTTP %d). Invalidando token...\n", httpResponseCode);
        tokenManager.clear(); 
        statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento; 
    } else {
        Serial.printf("[ENVIO] Error HTTP: %d. Programando reintento...\n", httpResponseCode);
        statemachine->clocks.proximo_envio = now + appConfig.intervaloReintento; 
    }

    http.end(); 
    client.stop(); 
    statemachine->flags.envio_programado = true; 
    statemachine->ChangeState(new EstadoLECTURA()); 
}

void EstadoENVIO::onExit() { 
    statemachine->flags.envio = false; 
}

const char *EstadoENVIO::getName() { return "ENVIO"; }

// ======================================== 
// IMPLEMENTACIÓN ESTADO DESARROLLADOR 
// ========================================
void EstadoDESARROLLADOR::onEnter() {
    sensorManager.cancel();
    statemachine->isDisplayOn = true; 
    Serial.println("=== ENTRANDO A ESTADO: DESARROLLADOR ==="); 
    statemachine->flags.dev = true; 
    statemachine->flags.inicio = false; 
    statemachine->flags.lectura = false; 
    primera_vez = true;
    
    // Reiniciamos las variables de navegación al entrar
    subMode = 0;
    selectedNetwork = 0;
    scrollOffset = 0;
    totalNetworks = 0;

    displayDeveloperInfo();
    if (wifiManager.isConnected()) MDNS.begin(appConfig.hostname.c_str()); 
}

void EstadoDESARROLLADOR::execute() { 
    ButtonHandler::Event btn = statemachine->currentEvent;
    unsigned long now = millis();

    if (devWeb && !primera_vez) devWeb->handle();
    captiveDns.processNextRequest();
    if (btn == ButtonHandler::VERY_LONG_PRESS_4S) {
        statemachine->ChangeState(new EstadoINICIO());
        return;
    }

    // ===== SUB-MODO 0: MENÚ PRINCIPAL =====
    if (subMode == 0) {
        if (primera_vez) {
            if (!devWeb) { devWeb = new DevWebOTA(&server); }
            devWeb->begin();
            server.onNotFound([]() {
                const bool ap = (WiFi.getMode() & WIFI_AP) != 0;
                const String ip = ap ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
                server.sendHeader("Location", "http://" + ip + "/");
                server.send(302, "text/plain", "Abrir portal");
            });
            if ((WiFi.getMode() & WIFI_AP) != 0) captiveDns.start(53, "*", WiFi.softAPIP());
            MDNS.begin(appConfig.hostname.c_str());
            primera_vez = false;
        }
        devWeb->handle();

        if (now - lastDisplayRefresh >= 2000) { 
            displayDeveloperInfo(); 
            lastDisplayRefresh = now; 
        }

        if (btn == ButtonHandler::VERY_LONG_PRESS_4S) {
            statemachine->ChangeState(new EstadoINICIO());
            return;
        } 
        else if (btn == ButtonHandler::SHORT_PRESS) {
            subMode = 1; 
            WiFi.scanNetworks(true); 
            display.clearDisplay(); 
            display.setCursor(10,30); 
            display.print("Buscando WiFi..."); 
            display.display();
        } 
    } 
    // ===== SUB-MODO 1: ESCANEANDO =====
    else if (subMode == 1) { 
        int n = WiFi.scanComplete();
        if (n >= 0) { 
            totalNetworks = n; 
            selectedNetwork = 0; 
            scrollOffset = 0; 
            subMode = 2; 
            displayWiFiList(selectedNetwork, scrollOffset, totalNetworks);
        } 
    } 
    // ===== SUB-MODO 2: LISTA PAGINADA =====
    else if (subMode == 2) { 
        if (btn == ButtonHandler::DOUBLE_CLICK) { 
            WiFi.scanDelete(); 
            subMode = 0; 
            displayDeveloperInfo(); 
        } 
        else if (btn == ButtonHandler::SHORT_PRESS) { 
            selectedNetwork++;
            if (selectedNetwork >= totalNetworks) selectedNetwork = 0; 
            if (selectedNetwork >= scrollOffset + 5) scrollOffset = selectedNetwork - 4; 
            if (selectedNetwork < scrollOffset) scrollOffset = selectedNetwork; 
            displayWiFiList(selectedNetwork, scrollOffset, totalNetworks); 
        } 
        else if (btn == ButtonHandler::LONG_PRESS_2S && totalNetworks > 0) { 
            String ssid = WiFi.SSID(selectedNetwork);
            if (wifiManager.hasCredentialsFor(ssid)) { 
                display.clearDisplay(); 
                display.setCursor(10,30); 
                display.print("Conectando..."); 
                display.display();
                wifiManager.connectTo(ssid); 
                subMode = 0; 
                displayDeveloperInfo(); 
            } else {
                subMode = 3; 
                wifiManager.createAP(appConfig.hostname);
                captiveDns.stop();
                captiveDns.start(53, "*", WiFi.softAPIP()); 
                MDNS.begin(appConfig.hostname.c_str()); 
                if (!devWeb) { devWeb = new DevWebOTA(&server); } 
                server.begin(); 
                displayAPAlert(); 
            } 
        } 
    } 
    // ===== SUB-MODO 3: ALERTA AP Y SERVIDOR LOCAL =====
    else if (subMode == 3) { 
        devWeb->handle();
        if (btn == ButtonHandler::DOUBLE_CLICK) { 
            captiveDns.stop();
            WiFi.enableAP(false);
            WiFi.mode(WIFI_STA);

            subMode = 0; 
            displayDeveloperInfo(); 
            
            if(WiFi.status() != WL_CONNECTED) {
                display.clearDisplay(); display.setCursor(10,30); 
                display.print("Aplicando Red..."); display.display();
                wifiManager.connect(5); 
                displayDeveloperInfo(); 
            } 
        } 
    } 
}

void EstadoDESARROLLADOR::onExit() {
    captiveDns.stop(); 
    statemachine->flags.dev = false; 
    primera_vez = true; 
    WiFi.enableAP(false);
    WiFi.mode(WIFI_STA);
    if (devWeb) { devWeb->end(); } 
}

const char *EstadoDESARROLLADOR::getName() { return "DESARROLLADOR"; }
```
