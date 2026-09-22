# Guía del firmware Globo Radiante

El firmware se abre mediante `main/main.ino`. Conserva la máquina de estados, WiFi, token Bearer, botón y portal OTA del firmware TARS.

## Dependencias

- ESP32 by Espressif Systems 3.x
- Adafruit SSD1306, Adafruit GFX y Adafruit BusIO
- ArduinoJson 7.x
- OneWire y DallasTemperature

El HDC1080 se maneja directamente mediante `Wire`.

## Pines

- I2C: SDA GPIO21 y SCL GPIO22.
- DS18B20: GPIO27 con pull-up de 4,7 kΩ a 3,3 V.
- Botón: GPIO0 a GND con `INPUT_PULLUP`.

## Configuración

El hostname inicial es `globo-mrt-01`. El espacio NVS `globomrt` separa esta configuración de TARS. En el primer inicio se abre modo desarrollador para configurar WiFi, URL de Orion y autenticación.

Si no existe una red disponible, usar el AP `TARS-globo-mrt-01` y abrir `http://192.168.4.1/`. Después de cambiar el hostname, reiniciar.

La URL debe terminar en `/v2/entities/ID/attrs`. La entidad se crea previamente. El payload contiene `tempAire`, `tempGlobo`, `mrt` y `humedad`, todos `Float`.

El dispositivo solo envía muestras válidas y recientes. Un HTTP 2xx limpia los acumuladores. Ante un fallo programa un reintento y no envía `NaN`.

## Pruebas pendientes

- Compilar con la placa ESP32 y bibliotecas instaladas.
- Verificar OLED, HDC1080 y DS18B20.
- Confirmar tensiones de 5 V y 3,3 V, polaridad del capacitor y carga USB-C.
- Medir autonomía real y respuesta térmica.
- Probar NGSIv2, reconexión, token y OTA.
- Confirmar que el AP se apague al salir del portal.

El portal OTA heredado no incluye autenticación adicional y TLS conserva `setInsecure()`. Usar en una red controlada.
