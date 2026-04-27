#pragma once

// Firmware
#define FIRMWARE_VERSION "1.2.0"

// AP de configuració (primera arrencada o WiFi no configurat)
#define PROVISION_AP_SSID "SmartGarden-Setup"

// I2C default pins (HTU21D + BH1750)
#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22

// Intervals
#define SENSOR_FAILSAFE_MS     (15 * 60 * 1000UL)  // publica si no arriba cap petició en 15 min
#define MQTT_RECONNECT_MAX_MS  60000                // backoff màxim 60s
