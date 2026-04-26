#include "LocalSchedule.h"
#include <Preferences.h>
#include <ArduinoJson.h>

void LocalSchedule::loadFromNVS() {
  // TODO: llegir _entries des de Preferences (NVS)
}

void LocalSchedule::saveToNVS() {
  // TODO: desar _entries a Preferences (NVS)
}

void LocalSchedule::updateFromJson(const char* json) {
  // TODO: parseja JSON rebut via MQTT config/push i actualitza _entries
}

void LocalSchedule::tick(uint8_t currentHour, uint8_t currentMinute, uint8_t currentDow) {
  // TODO: si hora/dia coincideix amb una entrada, activar la zona corresponent
}
