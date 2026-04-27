#pragma once

// Firmware
#define FIRMWARE_VERSION "1.2.0"

// AP de configuració (primera arrencada o WiFi no configurat)
#define PROVISION_AP_SSID "SmartGarden-Setup"

// Pins sensors de terra (analògics ADC1 — compatibles amb WiFi)
#define SOIL_ZONE1_PIN_A  32
#define SOIL_ZONE1_PIN_B  33
#define SOIL_ZONE2_PIN_A  34
#define SOIL_ZONE2_PIN_B  35

// I2C (SHT21/HTU21 + BH1750)
#define I2C_SDA_PIN  21
#define I2C_SCL_PIN  22

// Pins relés (actius LOW)
#define RELAY_ZONE1_PIN  14
#define RELAY_ZONE2_PIN  27

// Intervals
#define SENSOR_FAILSAFE_MS     (15 * 60 * 1000UL)  // publica si no arriba cap petició en 15 min
#define MQTT_RECONNECT_MAX_MS  60000                // backoff màxim 60s
