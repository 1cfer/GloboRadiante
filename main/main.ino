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
