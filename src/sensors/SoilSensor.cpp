#include "SoilSensor.h"
#include <string.h>

SoilSensor::SoilSensor(const PeripheralConfig* sensors[], int count,
                       AggregationMode mode, int dryVal, int wetVal)
  : _count(count), _mode(mode), _defaultDry(dryVal), _defaultWet(wetVal)
{
  int n = count < MAX_SOIL_PER_ZONE ? count : MAX_SOIL_PER_ZONE;
  for (int i = 0; i < n; i++) _sensors[i] = sensors[i];
}

void SoilSensor::begin() {
  // ADC1 pins don't require pinMode on ESP32
}

float SoilSensor::_rawToPercent(int raw, int dryVal, int wetVal) {
  if (dryVal == wetVal) return NAN;
  float pct = 100.0f * (dryVal - raw) / (float)(dryVal - wetVal);
  return constrain(pct, 0.0f, 100.0f);
}

float SoilSensor::_readOneSensor(int idx) {
  if (!_sensors[idx]) return NAN;
  const PeripheralConfig& p = *_sensors[idx];
  if (p.pin1 == PIN_UNSET) return NAN;

  int raw = analogRead(p.pin1);
  int dry = (p.calEmpty > 0) ? p.calEmpty : _defaultDry;
  int wet = (p.calFull  > 0) ? p.calFull  : _defaultWet;
  return _rawToPercent(raw, dry, wet);
}

float SoilSensor::readHumidityPct() {
  if (_count == 0) return NAN;

  switch (_mode) {
    case AggregationMode::AVG: {
      float sum = 0.0f;
      int   valid = 0;
      for (int i = 0; i < _count; i++) {
        float v = _readOneSensor(i);
        if (!isnan(v)) { sum += v; valid++; }
      }
      return (valid > 0) ? (sum / valid) : NAN;
    }

    case AggregationMode::ANY_BELOW:
    case AggregationMode::ALL_BELOW: {
      // Return the minimum — the caller uses threshold comparison
      // ANY_BELOW: if min < threshold → irrigate
      // ALL_BELOW: min is the most-humid sensor; if all are below, min < threshold too
      float minVal = 200.0f;
      int   valid  = 0;
      for (int i = 0; i < _count; i++) {
        float v = _readOneSensor(i);
        if (!isnan(v)) {
          if (v < minVal) minVal = v;
          valid++;
        }
      }
      if (valid == 0) return NAN;
      if (_mode == AggregationMode::ALL_BELOW) {
        // Return max so that ALL must be below threshold
        float maxVal = -1.0f;
        for (int i = 0; i < _count; i++) {
          float v = _readOneSensor(i);
          if (!isnan(v) && v > maxVal) maxVal = v;
        }
        return maxVal;
      }
      return minVal;
    }
  }
  return NAN;
}
