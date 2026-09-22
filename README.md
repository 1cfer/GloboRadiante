# Globo Radiante

Firmware y documentación de un dispositivo portátil basado en ESP32 para estimar la **temperatura radiante media (MRT)**. El equipo mide temperatura del aire, temperatura de globo y humedad relativa; presenta los datos en una pantalla OLED y transmite promedios a FIWARE mediante NGSIv2.

## Documentación

- [Documentación técnica en PDF](docs/GloboRadiante_Documentacion_Tecnica.pdf)
- [Guía del firmware](docs/GUIA_FIRMWARE.md)
- [Código completo de los archivos adaptados](docs/CODIGO_COMPLETO.md)
- [Sketch principal](main/main.ino)

## Hardware

| Elemento | Configuración |
|---|---|
| Microcontrolador | ESP32 |
| Pantalla | OLED SSD1306 128 × 64, I2C |
| Sensores | HDC1080 y DS18B20 |
| Batería | Litio, 1000 mAh |
| Carga | USB-C |
| Energía | UPS con salida de 5 V y regulación a 3,3 V |
| Filtrado | Capacitor de 1000 µF / 12 V en la salida de 5 V de la UPS |
| Carcasa | PLA impreso en 3D, con tapa atornillada |
| Controles | Switch de encendido y botón multifunción |

Los componentes electrónicos trabajan a **3,3 V**. Los 12 V del capacitor corresponden a su tensión nominal; el bus donde se conecta es de 5 V. El capacitor apoya el filtrado y la reserva local, pero no regula tensión.

## Conexiones

| Componente | ESP32 |
|---|---|
| OLED SDA / SCL | GPIO21 / GPIO22 |
| HDC1080 SDA / SCL | GPIO21 / GPIO22, dirección 0x40 |
| DS18B20 DATA | GPIO27, pull-up de 4,7 kΩ a 3,3 V |
| Botón | GPIO0 a GND, `INPUT_PULLUP` |

El DS18B20 se alimenta a 3,3 V con tres hilos. El HDC1080 debe ubicarse fuera del globo y alejado del calor del ESP32 y de la alimentación. No mantener GPIO0 pulsado durante encendido o reinicio.

## Cálculo de MRT

Se implementa la forma simplificada de ISO 7726:1998 para convección natural, globo negro estándar de 150 mm y emisividad 0,95:

```text
MRT = [(Tg + 273.15)^4 + 4×10^7 × |Tg-Ta|^(1/4) × (Tg-Ta)]^(1/4) - 273.15
```

Ta, Tg y MRT se expresan en °C. La humedad se registra, pero no forma parte de la ecuación. Con corrientes de aire se necesita medir velocidad del aire y evaluar convección forzada. La carcasa de PLA de la electrónica no se considera automáticamente un globo normalizado.

## Pantalla y botón

La OLED alterna cada 5 s entre Ta/Tg/HR, MRT grande, tiempo para el próximo envío e IP de red. Un clic corto cambia de vista. Si la pantalla se apagó por inactividad, el primer clic la despierta sin avanzar. Una pulsación de 4 s entra o sale del modo desarrollador. El switch físico enciende o apaga todo el dispositivo.

## Firmware

El proyecto conserva la arquitectura base de TARS: `StateMachine`, `WiFiManager`, `TokenManager`, `ButtonHandler` y `DevWebOTA`. Los estados son `INICIO`, `LECTURA`, `ENVIO` y `DESARROLLADOR`. Al salir del último se detienen DNS y servidor, se ejecuta `WiFi.enableAP(false)` y se vuelve a modo estación.

El payload NGSIv2 transmite `tempAire`, `tempGlobo`, `mrt` y `humedad`, todos como `Float`. La entidad de Orion debe existir previamente.

## Arduino IDE

1. Abrir [main/main.ino](main/main.ino) sin separar los archivos de `main`.
2. Instalar **esp32 by Espressif Systems 3.x**.
3. Instalar Adafruit SSD1306, Adafruit GFX, Adafruit BusIO, ArduinoJson 7.x, OneWire y DallasTemperature.
4. Seleccionar particiones compatibles con OTA, compilar y cargar.
5. Monitor serie: 115200 baudios.

En el primer arranque se abre modo desarrollador. Sin una red guardada, el AP se denomina `TARS-globo-mrt-01` y el portal está en `http://192.168.4.1/`. El hostname inicial es `globo-mrt-01`.

## FIWARE

URL de ejemplo: `http://IP_RASPBERRY:1026/v2/entities/globo-mrt-01/attrs`.

Para Orion local sin autenticación puede activarse `Skip Token`. Con autenticación se deben configurar credenciales propias y un `tokenUrl` HTTPS. No se deben subir credenciales reales.

## Verificación

```bash
python3 verificar.py
```

El script ejecuta 298 casos numéricos de MRT y verifica la integridad de diez archivos base. Esto no sustituye la compilación completa para ESP32 ni las pruebas físicas de sensores, batería, carga, red y OTA.

Adaptado de [1cfer/Tars](https://github.com/1cfer/Tars), commit `2439dd5a2378ba45309707a6d51de8fe334ec83c`.

Referencias: [ISO 7726:1998](https://standards.iteh.ai/catalog/standards/iso/99f92eea-d1b3-48b4-8a3c-8e5a5112718a/iso-7726-1998), [HDC1080](https://www.ti.com/lit/ds/symlink/hdc1080.pdf) y [DS18B20](https://www.analog.com/media/en/technical-documentation/data-sheets/DS18B20.pdf).
