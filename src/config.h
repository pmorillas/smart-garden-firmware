#pragma once

// WiFi
#define WIFI_SSID       "Pararols4-1er2na"
#define WIFI_PASSWORD   "hML3vpH5ejKS"

// Firmware
#define FIRMWARE_VERSION "1.1.0"

// MQTT
#define MQTT_BROKER  "192.168.1.162"
#define MQTT_PORT    1883
// MQTT_CLIENT_ID és dinàmic (basat en MAC) — generat a MqttClient::_reconnect()

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
#define PUBLISH_INTERVAL_MS    (10 * 1000UL)                     // (5 * 60 * 1000UL)   // 5 minuts
#define MQTT_RECONNECT_MAX_MS  60000                // backoff màxim 60s
