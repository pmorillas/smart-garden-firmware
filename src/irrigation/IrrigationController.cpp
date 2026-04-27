#include "IrrigationController.h"

void IrrigationController::begin(const ZoneManager& zm, const PeripheralRegistry& registry) {
  _count = 0;
  int total = zm.zoneCount();

  for (int i = 0; i < total && _count < MAX_ZONES; i++) {
    const ZoneConfig& z = zm.zone(i);
    const PeripheralConfig* relayPerif = registry.byId(z.relayPeripheralId);

    if (!relayPerif || relayPerif->pin1 == PIN_UNSET) {
      Serial.printf("[IrrigCtrl] Zona %d: sense relé configurat\n", z.id);
      continue;
    }

    _relayPins[_count] = relayPerif->pin1;
    _zoneIds[_count]   = z.id;
    _endMs[_count]     = 0;
    pinMode(relayPerif->pin1, OUTPUT);
    digitalWrite(relayPerif->pin1, HIGH);  // active LOW — off by default
    _count++;
    Serial.printf("[IrrigCtrl] Zona %d → relé GPIO%d\n", z.id, relayPerif->pin1);
  }

  Serial.printf("[IrrigCtrl] %d zones inicialitzades\n", _count);
}

int IrrigationController::_indexOf(int zoneId) const {
  for (int i = 0; i < _count; i++) {
    if (_zoneIds[i] == zoneId) return i;
  }
  return -1;
}

void IrrigationController::startZone(int zoneId, unsigned long durationMs) {
  int idx = _indexOf(zoneId);
  if (idx < 0) { Serial.printf("[IrrigCtrl] Zona %d no trobada\n", zoneId); return; }
  digitalWrite(_relayPins[idx], LOW);
  _endMs[idx] = millis() + durationMs;
  Serial.printf("[IrrigCtrl] Zona %d ON durant %lus\n", zoneId, durationMs / 1000);
}

void IrrigationController::stopZone(int zoneId) {
  int idx = _indexOf(zoneId);
  if (idx < 0) return;
  digitalWrite(_relayPins[idx], HIGH);
  _endMs[idx] = 0;
  Serial.printf("[IrrigCtrl] Zona %d OFF\n", zoneId);
}

void IrrigationController::stopAll() {
  for (int i = 0; i < _count; i++) {
    if (_endMs[i] > 0) { digitalWrite(_relayPins[i], HIGH); _endMs[i] = 0; }
  }
}

bool IrrigationController::isAnyActive() const {
  for (int i = 0; i < _count; i++) if (_endMs[i] > 0) return true;
  return false;
}

void IrrigationController::loop() {
  unsigned long now = millis();
  for (int i = 0; i < _count; i++) {
    if (_endMs[i] > 0 && now >= _endMs[i]) {
      digitalWrite(_relayPins[i], HIGH);
      _endMs[i] = 0;
      Serial.printf("[IrrigCtrl] Zona %d timeout, apagada\n", _zoneIds[i]);
    }
  }
}
