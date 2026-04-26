# Smart Garden — Firmware ESP32

## Plataforma

- **Hardware:** ESP32 (qualsevol variant amb WiFi)
- **Framework:** Arduino via PlatformIO
- **IDE recomanat:** VS Code + extensió PlatformIO

## Dependències (platformio.ini)

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    knolleary/PubSubClient @ ^2.8
    adafruit/DHT sensor library @ ^1.4
    bblanchon/ArduinoJson @ ^7.0
```

---

## Pins per defecte (`config.h`)

| Funció | GPIO |
|---|---|
| Sensor terra zona 1 — A | 34 |
| Sensor terra zona 1 — B | 35 |
| Sensor terra zona 2 — A | 32 |
| Sensor terra zona 2 — B | 33 |
| DHT22 ambient | 27 |
| Relé zona 1 | 26 |
| Relé zona 2 | 25 |

Tots configurables a `src/config.h` sense tocar el codi.

---

## Comportament MQTT

- Publica lectures cada **5 minuts** (configurable a `config.h`)
- Topics de publicació:
  - `smartgarden/sensors/soil/{zone_id}` — lectures d'humitat de terra
  - `smartgarden/sensors/ambient` — temperatura i humitat DHT22
- Topics subscrits:
  - `smartgarden/control/{zone_id}` — ordres de reg (on/off + durada)
  - `smartgarden/config/push` — actualització de programes offline
- Reconnexió automàtica amb backoff exponencial (màx 60s)

---

## Mode offline

Si perd la connexió WiFi o MQTT:
1. Continua llegint sensors localment
2. Executa l'últim programa de reg rebut (guardat a NVS — Non-Volatile Storage)
3. Quan es reconnecta, publica les lectures acumulades i sincronitza config

---

## Format JSON dels missatges

**Publica (soil):**
```json
{
  "zone_id": 1,
  "values": [42, 45],
  "unit": "percent",
  "timestamp": 1714123456
}
```

**Publica (ambient):**
```json
{
  "temp": 22.5,
  "humidity": 60.2,
  "unit_temp": "celsius",
  "timestamp": 1714123456
}
```

**Rep (control):**
```json
{
  "action": "on",
  "duration_seconds": 120
}
```

**Rep (config/push):**
```json
{
  "schedules": [
    {"zone_id": 1, "time_start": "07:00", "days": [1,3,5], "duration_minutes": 10}
  ]
}
```

---

## Compilar i pujar

```bash
# Instal·lar PlatformIO CLI
pip install platformio

# Compilar
pio run

# Pujar a l'ESP32 (ajusta el port)
pio run --target upload --upload-port /dev/ttyUSB0

# Monitor sèrie
pio device monitor --baud 115200
```
