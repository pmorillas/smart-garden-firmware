# smart-garden-firmware

Firmware ESP32 per al sistema de reg automàtic.

## Requisits

- [PlatformIO](https://platformio.org/) (CLI o extensió VS Code)

## Configuració

Edita `src/config.h` amb les teves credencials WiFi i la IP del broker MQTT.

## Compilar i pujar

```bash
# Compilar
pio run

# Pujar (ajusta el port)
pio run --target upload --upload-port /dev/ttyUSB0

# Monitor sèrie
pio device monitor --baud 115200
```

## Estructura

```
src/
├── main.cpp                     — loop principal
├── config.h                     — WiFi, MQTT, pins, intervals
├── sensors/
│   ├── SoilSensor.h/.cpp        — sensors d'humitat de terra
│   └── AmbientSensor.h/.cpp     — DHT22 temperatura/humitat
├── mqtt/
│   └── MqttClient.h/.cpp        — publicació i subscripció MQTT
├── irrigation/
│   └── IrrigationController.h/.cpp — control relés
└── schedule/
    └── LocalSchedule.h/.cpp     — mode offline (NVS)
```

Veure `CLAUDE.md` per a la documentació completa.
