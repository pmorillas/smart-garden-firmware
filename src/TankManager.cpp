#include "TankManager.h"
#include <ArduinoJson.h>
#include <Preferences.h>

TankManager::TankManager() : _count(0) {}

void TankManager::loadFromNVS() {
  Preferences prefs;
  prefs.begin(TANK_NVS_NAMESPACE, true);
  String stored = prefs.getString(TANK_NVS_KEY, "");
  prefs.end();

  if (stored.isEmpty()) {
    _count = 0;
    Serial.println("[TankMgr] Sense config al NVS");
    return;
  }

  JsonDocument doc;
  if (deserializeJson(doc, stored) != DeserializationError::Ok) {
    Serial.println("[TankMgr] Error parsejant config NVS");
    _count = 0;
    return;
  }

  JsonArray arr = doc["tanks"].as<JsonArray>();
  _count = 0;
  for (JsonObject t : arr) {
    if (_count >= MAX_TANKS) break;
    TankConfig& tc = _tanks[_count];
    tc.id           = t["id"]           | 0;
    tc.peripheralId = t["peripheral_id"] | 0;
    tc.lowPct       = t["low_pct"]      | 20;
    tc.emptyPct     = t["empty_pct"]    | 5;
    if (tc.id > 0) _count++;
  }
  Serial.printf("[TankMgr] %d dipòsits carregats des de NVS\n", _count);
}

bool TankManager::saveToNVS(const char* jsonStr) {
  JsonDocument doc;
  if (deserializeJson(doc, jsonStr) != DeserializationError::Ok) {
    Serial.println("[TankMgr] JSON invàlid, no es desa");
    return false;
  }
  Preferences prefs;
  prefs.begin(TANK_NVS_NAMESPACE, false);
  prefs.putString(TANK_NVS_KEY, jsonStr);
  prefs.end();
  Serial.println("[TankMgr] Config desada al NVS");
  return true;
}
