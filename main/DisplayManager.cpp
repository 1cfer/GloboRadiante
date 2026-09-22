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
