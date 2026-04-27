#include "ZoneManager.h"
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
    Serial.println("[ZoneMgr] Error parsejant config NVS, usant defaults");
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
    ZoneConfig& zc = _zones[_count];

    zc.id                 = z["id"]                  | 0;
    zc.relayPeripheralId  = z["relay_peripheral_id"]  | 0;
    zc.aggregationMode    = aggregationModeFromStr(z["soil_aggregation_mode"] | "AVG");
    zc.soilCount          = 0;

    if (z["soil_peripheral_ids"].is<JsonArray>()) {
      for (uint8_t pid : z["soil_peripheral_ids"].as<JsonArray>()) {
        if (zc.soilCount >= MAX_SOIL_PER_ZONE) break;
        zc.soilPeripheralIds[zc.soilCount++] = pid;
      }
    }

    if (zc.id > 0) _count++;
  }
  Serial.printf("[ZoneMgr] %d zones carregades des de NVS\n", _count);
}

bool ZoneManager::saveToNVS(const char* jsonStr) {
  Preferences prefs;
  prefs.begin(ZONE_NVS_NAMESPACE, false);
  bool ok = prefs.putString(ZONE_NVS_KEY, jsonStr);
  prefs.end();
  if (ok) Serial.println("[ZoneMgr] Config desada a NVS");
  else    Serial.println("[ZoneMgr] Error desant config a NVS");
  return ok;
}

int ZoneManager::indexById(int id) const {
  for (int i = 0; i < _count; i++) {
    if (_zones[i].id == id) return i;
  }
  return -1;
}

void ZoneManager::_loadDefaults() {
  // No hardcoded pins — ESP operates without zone config until backend pushes it
  _count = 0;
  Serial.println("[ZoneMgr] Sense config NVS, esperant config del backend");
}
