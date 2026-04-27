#include "PeripheralRegistry.h"
#include <ArduinoJson.h>
#include <Preferences.h>
#include <string.h>

PeripheralType peripheralTypeFromStr(const char* s) {
  if (!s)                            return PeripheralType::UNKNOWN;
  if (strcmp(s, "SOIL_ADC") == 0)    return PeripheralType::SOIL_ADC;
  if (strcmp(s, "HTU21D") == 0)      return PeripheralType::HTU21D;
  if (strcmp(s, "BH1750") == 0)      return PeripheralType::BH1750;
  if (strcmp(s, "RELAY") == 0)       return PeripheralType::RELAY;
  if (strcmp(s, "HC_SR04") == 0)     return PeripheralType::HC_SR04;
  if (strcmp(s, "FLOAT_BINARY") == 0) return PeripheralType::FLOAT_BINARY;
  return PeripheralType::UNKNOWN;
}

AggregationMode aggregationModeFromStr(const char* s) {
  if (s && strcmp(s, "ANY_BELOW") == 0) return AggregationMode::ANY_BELOW;
  if (s && strcmp(s, "ALL_BELOW") == 0) return AggregationMode::ALL_BELOW;
  return AggregationMode::AVG;
}

void PeripheralRegistry::_parse(const char* jsonArrayStr) {
  _count = 0;
  if (!jsonArrayStr || jsonArrayStr[0] == '\0') return;

  JsonDocument doc;
  if (deserializeJson(doc, jsonArrayStr) != DeserializationError::Ok) {
    Serial.println("[PerifReg] Error parsejant JSON");
    return;
  }

  JsonArray arr = doc.as<JsonArray>();
  if (arr.isNull()) return;

  for (JsonObject obj : arr) {
    if (_count >= MAX_PERIPHERALS) break;
    PeripheralConfig& p = _p[_count];

    p.id          = obj["id"]           | 0;
    p.pin1        = obj["pin1"].isNull() ? PIN_UNSET : (uint8_t)(obj["pin1"] | PIN_UNSET);
    p.pin2        = obj["pin2"].isNull() ? PIN_UNSET : (uint8_t)(obj["pin2"] | PIN_UNSET);
    p.i2cAddress  = obj["i2c_address"].isNull() ? 0 : (uint8_t)(obj["i2c_address"] | 0);
    p.i2cBus      = obj["i2c_bus"]      | 0;
    p.type        = peripheralTypeFromStr(obj["type"] | "");

    if (obj["extra_config"].is<JsonObject>()) {
      JsonObject ec = obj["extra_config"].as<JsonObject>();
      p.calEmpty = ec["cal_empty"] | 0;
      p.calFull  = ec["cal_full"]  | 0;
    } else {
      p.calEmpty = 0;
      p.calFull  = 0;
    }

    const char* nm = obj["name"] | "";
    strncpy(p.name, nm, sizeof(p.name) - 1);
    p.name[sizeof(p.name) - 1] = '\0';

    if (p.id > 0 && p.type != PeripheralType::UNKNOWN) {
      _count++;
    }
  }
  Serial.printf("[PerifReg] %d perifèrics carregats\n", _count);
}

void PeripheralRegistry::loadFromNVS() {
  Preferences prefs;
  prefs.begin(HW_CFG_NVS_NS, true);
  String json = prefs.getString(HW_CFG_NVS_KEY, "");
  prefs.end();

  if (json.isEmpty()) {
    Serial.println("[PerifReg] Sense config al NVS");
    return;
  }
  _parse(json.c_str());
}

bool PeripheralRegistry::saveToNVS(const char* jsonArrayStr) {
  Preferences prefs;
  prefs.begin(HW_CFG_NVS_NS, false);
  bool ok = prefs.putString(HW_CFG_NVS_KEY, jsonArrayStr);
  prefs.end();
  if (ok) Serial.println("[PerifReg] Desat al NVS");
  else    Serial.println("[PerifReg] Error desant al NVS");
  return ok;
}

const PeripheralConfig* PeripheralRegistry::byId(uint8_t id) const {
  for (int i = 0; i < _count; i++) {
    if (_p[i].id == id) return &_p[i];
  }
  return nullptr;
}

int PeripheralRegistry::byType(PeripheralType t, const PeripheralConfig* out[], int maxOut) const {
  int found = 0;
  for (int i = 0; i < _count && found < maxOut; i++) {
    if (_p[i].type == t) out[found++] = &_p[i];
  }
  return found;
}
