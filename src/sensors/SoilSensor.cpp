#include "SoilSensor.h"

SoilSensor::SoilSensor(const PeripheralConfig* sensors[], int count, AggregationMode mode)
  : _count(count), _mode(mode)
{
  int n = count < MAX_SOIL_PER_ZONE ? count : MAX_SOIL_PER_ZONE;
  for (int i = 0; i < n; i++) _sensors[i] = sensors[i];
}

void SoilSensor::begin() {
  // ADC1 pins don't require pinMode on ESP32
}

int SoilSensor::readAllRaw(int* out, int maxCount) {
  int written = 0;
  int n = _count < maxCount ? _count : maxCount;
  for (int i = 0; i < n; i++) {
    if (!_sensors[i]) continue;
    const PeripheralConfig& p = *_sensors[i];
    if (p.pin1 == PIN_UNSET) continue;
    out[written++] = analogRead(p.pin1);
  }
  return written;
}
