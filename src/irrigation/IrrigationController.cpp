#include "IrrigationController.h"

void IrrigationController::begin(const ZoneManager& zm) {
  _count = zm.zoneCount();
  for (int i = 0; i < _count; i++) {
    const ZoneConfig& z = zm.zone(i);
    _relayPins[i] = z.relayPin;
    _zoneIds[i]   = z.id;
    _endMs[i]     = 0;
    pinMode(z.relayPin, OUTPUT);
    digitalWrite(z.relayPin, HIGH);  // relé actiu LOW — apagat per defecte
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
  if (idx < 0) {
    Serial.printf("[IrrigCtrl] Zona %d no trobada\n", zoneId);
    return;
  }
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
    if (_endMs[i] > 0) {
      digitalWrite(_relayPins[i], HIGH);
      _endMs[i] = 0;
    }
  }
}

bool IrrigationController::isAnyActive() const {
  for (int i = 0; i < _count; i++) {
    if (_endMs[i] > 0) return true;
  }
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
