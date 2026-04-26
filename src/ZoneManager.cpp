#include "ZoneManager.h"
#include "config.h"
#include <ArduinoJson.h>
#include <Preferences.h>

ZoneManager::ZoneManager() : _count(0) {}

void ZoneManager::loadFromNVS() {
  Preferences prefs;
  prefs.begin(ZONE_NVS_NAMESPACE, true);
  String json = prefs.getString(ZONE_NVS_KEY, "");
  prefs.end();

  if (json.isEmpty()) {
    _loadDefaults();
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, json) != DeserializationError::Ok) {
    Serial.println("[ZoneManager] Error parsejant config NVS, usant defaults");
    _loadDefaults();
    return;
  }

  JsonArray arr = doc["zones"].as<JsonArray>();
  if (arr.isNull() || arr.size() == 0) {
    _loadDefaults();
    return;
  }

  _count = 0;
  for (JsonObject z : arr) {
    if (_count >= MAX_ZONES) break;
    _zones[_count++] = {
      z["id"].as<int>(),
      z["relay_pin"].as<int>(),
      z["soil_pin_a"].as<int>(),
      z["soil_pin_b"].as<int>()
    };
  }
  Serial.printf("[ZoneManager] %d zones carregades des de NVS\n", _count);
}

bool ZoneManager::saveToNVS(const char* jsonStr) {
  Preferences prefs;
  prefs.begin(ZONE_NVS_NAMESPACE, false);
  bool ok = prefs.putString(ZONE_NVS_KEY, jsonStr);
  prefs.end();
  if (ok) {
    Serial.println("[ZoneManager] Config desada a NVS");
  } else {
    Serial.println("[ZoneManager] Error desant config a NVS");
  }
  return ok;
}

int ZoneManager::indexById(int id) const {
  for (int i = 0; i < _count; i++) {
    if (_zones[i].id == id) return i;
  }
  return -1;
}

void ZoneManager::_loadDefaults() {
  _count = 2;
  _zones[0] = { 1, RELAY_ZONE1_PIN, SOIL_ZONE1_PIN_A, SOIL_ZONE1_PIN_B };
  _zones[1] = { 2, RELAY_ZONE2_PIN, SOIL_ZONE2_PIN_A, SOIL_ZONE2_PIN_B };
  Serial.println("[ZoneManager] Usant pins per defecte (config.h)");
}
