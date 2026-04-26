#include "SoilSensor.h"

// Calibració: ajusta aquests valors per al teu sensor específic
static constexpr int DRY_VALUE = 4095;
static constexpr int WET_VALUE = 1500;

SoilSensor::SoilSensor(uint8_t pinA, uint8_t pinB)
  : _pinA(pinA), _pinB(pinB) {}

void SoilSensor::begin() {
  // ADC pins d'entrada, no cal pinMode per a ADC en ESP32
}

float SoilSensor::readHumidityPct() {
  float a = _rawToPercent(analogRead(_pinA));
  float b = _rawToPercent(analogRead(_pinB));
  return (a + b) / 2.0f;
}

float SoilSensor::_rawToPercent(int raw) {
  float pct = 100.0f * (DRY_VALUE - raw) / (float)(DRY_VALUE - WET_VALUE);
  return constrain(pct, 0.0f, 100.0f);
}
