#pragma once

// WiFi
#define WIFI_SSID       "YOUR_SSID"
#define WIFI_PASSWORD   "YOUR_PASSWORD"

// MQTT
#define MQTT_BROKER     "192.168.1.X"   // IP del servidor al Tailscale o xarxa local
#define MQTT_PORT       1883
#define MQTT_CLIENT_ID  "esp32-smartgarden"

// Pins sensors de terra
#define SOIL_ZONE1_PIN_A  34
#define SOIL_ZONE1_PIN_B  35
#define SOIL_ZONE2_PIN_A  32
#define SOIL_ZONE2_PIN_B  33

// Pin DHT22 (temperatura + humitat ambient)
#define DHT22_PIN         27

// Pins relés (actius LOW)
#define RELAY_ZONE1_PIN   26
#define RELAY_ZONE2_PIN   25

// Intervals
#define PUBLISH_INTERVAL_MS  (5 * 60 * 1000UL)   // 5 minuts
#define MQTT_RECONNECT_MAX_MS  60000              // backoff màxim 60s
