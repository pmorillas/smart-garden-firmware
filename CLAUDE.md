# Smart Garden — Firmware ESP32

## Idioma

- **Comunicació amb l'usuari:** català
- **Tot artefacte tècnic (commits, comentaris, noms de variables):** anglès

---

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
board_build.partitions = min_spiffs.csv   ; dues particions app OTA de 1.9MB
build_flags = -DMQTT_MAX_PACKET_SIZE=512
lib_deps =
    knolleary/PubSubClient @ ^2.8
    bblanchon/ArduinoJson @ ^7.0
    claws/BH1750 @ ^1.3.0
    adafruit/Adafruit HTU21DF Library @ ^1.0.5
```

---

## Pins per defecte (`config.h`)

| Funció | GPIO |
|---|---|
| Sensor terra zona 1 — A | 32 |
| Sensor terra zona 1 — B | 33 |
| Sensor terra zona 2 — A | 34 |
| Sensor terra zona 2 — B | 35 |
| I2C SDA (HTU21 + BH1750) | 21 |
| I2C SCL (HTU21 + BH1750) | 22 |
| Relé zona 1 (actiu LOW) | 14 |
| Relé zona 2 (actiu LOW) | 27 |

Tots configurables a `src/config.h` sense tocar el codi.

---

## Comportament MQTT

- Publica lectures cada **5 minuts** (configurable a `config.h`)
- Topics de publicació:
  - `smartgarden/sensors/soil/{zone_id}` — lectures d'humitat de terra
  - `smartgarden/sensors/ambient` — temperatura, humitat i lluminositat
  - `smartgarden/devices/register` — registre del dispositiu (retained)
  - `smartgarden/devices/ota_status` — estat de les actualitzacions OTA
- Topics subscrits:
  - `smartgarden/control/{zone_id}` — ordres de reg (on/off + durada)
  - `smartgarden/config/push` — actualització de programes offline
  - `smartgarden/ota/{mac_address}` — ordres d'actualització OTA
- Reconnexió automàtica amb backoff exponencial (màx 60s)

---

## OTA (Over-The-Air) — Actualitzacions sense cable

### Flux complet

1. **Pujar el .bin al backend** (via API o curl):
   ```bash
   curl -X POST http://192.168.1.162:8000/api/firmware/ \
     -H "Authorization: Bearer <token>" \
     -F "version=1.2.0" \
     -F "notes=Millores de rendiment" \
     -F "file=@.pio/build/esp32dev/firmware.bin"
   ```

2. **Disparar l'actualització** per a un dispositiu o tots:
   ```bash
   # Tots els dispositius actius
   curl -X POST http://192.168.1.162:8000/api/firmware/1/deploy \
     -H "Authorization: Bearer <token>" \
     -H "Content-Type: application/json" \
     -d '{}'

   # Dispositiu específic (id=1)
   curl -X POST http://192.168.1.162:8000/api/firmware/1/deploy \
     -H "Authorization: Bearer <token>" \
     -H "Content-Type: application/json" \
     -d '{"device_id": 1}'
   ```

3. **L'ESP32 rep el missatge MQTT** a `smartgarden/ota/{mac}`, descarrega el binari i fa el flash automàticament. Reinicia sol.

4. **Consultar estat**:
   ```bash
   curl http://192.168.1.162:8000/api/firmware/devices/1/status \
     -H "Authorization: Bearer <token>"
   ```

### Protocol MQTT OTA

**Backend → ESP32** (`smartgarden/ota/{mac}`):
```json
{"version": "1.2.0", "url": "http://192.168.1.162:8000/api/firmware/1/download", "checksum": "sha256hex..."}
```

**ESP32 → Backend** (`smartgarden/devices/ota_status`):
```json
{"mac": "AA:BB:CC:DD:EE:FF", "status": "downloading|success|failed", "version": "1.2.0", "error": "..."}
```

### Comportament de seguretat
- Si la versió rebuda és la mateixa que la instal·lada (`FIRMWARE_VERSION` a `config.h`), l'actualització s'ignora.
- Si la descàrrega o el flash fallen, l'ESP32 continua operant normalment (el firmware antic segueix actiu gràcies a les particions dual OTA).
- El binari s'emmagatzema a `uploads/firmware/` al backend.

### Compilar el .bin per a OTA
```bash
pio run
# El binari és a: .pio/build/esp32dev/firmware.bin
```

### Primera instal·lació (cable USB obligatori)
```bash
pio run --target upload --upload-port /dev/ttyUSB0
```

---

## Mode offline

Si perd la connexió WiFi o MQTT:
1. Continua llegint sensors localment
2. Executa l'últim programa de reg rebut (guardat a NVS — Non-Volatile Storage)
3. Quan es reconnecta, publica les lectures acumulades i sincronitza config

---

## Format JSON dels missatges

**Publica (soil) — des de v1.4.0:**
```json
{"zone_id": 1, "raw_values": [2100, 1950], "mac": "AA:BB:...", "timestamp": 1714123456}
```
Publica valors ADC crus (0–4095). El backend aplica el calibratge.
Calibration parameters (`cal_empty`/`cal_full`) ja no s'envien al firmware.

**Publica (tank — FLOAT_BINARY N-pins, des de v1.5.0):**
```json
{"peripheral_id": 3, "pin_states": [0, 1, 0], "mac": "AA:BB:...", "timestamp": 1714123456}
```
`pin_states` alineat amb `extra_config.pins`. Backend interpreta mode pullup/pulldown i calcula level_pct.

**Publica (tank — HC_SR04 / llegat):**
```json
{"raw_value": 25.3, "level_pct": 70.0, "state": "ok", "mac": "AA:BB:...", "timestamp": 1714123456}
```

**Publica (ambient):**
```json
{"temp": 22.5, "humidity": 60.2, "light_lux": 1200, "unit_temp": "celsius", "mac": "AA:BB:...", "timestamp": 1714123456}
```

**Rep (control):**
```json
{"action": "on", "duration_seconds": 120}
```

**Rep (config/push):**
```json
{"schedules": [{"zone_id": 1, "time_start": "07:00", "days": [1,3,5], "duration_minutes": 10}]}
```

---

## Compilar i pujar (primera vegada)

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
