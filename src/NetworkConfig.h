#pragma once
#include <Preferences.h>
#include <stdint.h>

struct NetworkConfig {
    char     wifiSsid[64];
    char     wifiPass[64];
    char     mqttBroker[64];
    uint16_t mqttPort;
};

inline bool loadNetworkConfig(NetworkConfig& cfg) {
    Preferences prefs;
    prefs.begin("network", true);
    bool found = prefs.isKey("ssid");
    if (found) {
        prefs.getString("ssid",   cfg.wifiSsid,   sizeof(cfg.wifiSsid));
        prefs.getString("pass",   cfg.wifiPass,   sizeof(cfg.wifiPass));
        prefs.getString("broker", cfg.mqttBroker, sizeof(cfg.mqttBroker));
        cfg.mqttPort = prefs.getUShort("port", 1883);
    }
    prefs.end();
    return found;
}

inline void saveNetworkConfig(const NetworkConfig& cfg) {
    Preferences prefs;
    prefs.begin("network", false);
    prefs.putString("ssid",   cfg.wifiSsid);
    prefs.putString("pass",   cfg.wifiPass);
    prefs.putString("broker", cfg.mqttBroker);
    prefs.putUShort("port",   cfg.mqttPort);
    prefs.end();
}
