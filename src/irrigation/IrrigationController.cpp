#include "IrrigationController.h"
#include "../config.h"

void IrrigationController::begin() {
  pinMode(RELAY_ZONE1_PIN, OUTPUT);
  pinMode(RELAY_ZONE2_PIN, OUTPUT);
  digitalWrite(RELAY_ZONE1_PIN, HIGH);   // relé actiu LOW — apagat per defecte
  digitalWrite(RELAY_ZONE2_PIN, HIGH);
}

void IrrigationController::startZone(int zoneId, unsigned long durationMs) {
  if (zoneId == 1) {
    digitalWrite(RELAY_ZONE1_PIN, LOW);
    _zone1EndMs = millis() + durationMs;
  } else if (zoneId == 2) {
    digitalWrite(RELAY_ZONE2_PIN, LOW);
    _zone2EndMs = millis() + durationMs;
  }
}

void IrrigationController::stopZone(int zoneId) {
  if (zoneId == 1) {
    digitalWrite(RELAY_ZONE1_PIN, HIGH);
    _zone1EndMs = 0;
  } else if (zoneId == 2) {
    digitalWrite(RELAY_ZONE2_PIN, HIGH);
    _zone2EndMs = 0;
  }
}

void IrrigationController::loop() {
  unsigned long now = millis();
  if (_zone1EndMs > 0 && now >= _zone1EndMs) stopZone(1);
  if (_zone2EndMs > 0 && now >= _zone2EndMs) stopZone(2);
}
