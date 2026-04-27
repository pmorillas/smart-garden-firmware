#pragma once
#include <Arduino.h>
#include "PeripheralConfig.h"

class SoilSensor {
public:
  // sensors: array of pointers to PeripheralConfig (SOIL_ADC type)
  // count:   number of sensors (0 = zone with no soil sensors → returns NAN)
  // mode:    how to aggregate multiple sensor readings
  // dryVal / wetVal: ADC calibration defaults (overridden per-sensor if calEmpty/calFull set)
  SoilSensor(const PeripheralConfig* sensors[], int count, AggregationMode mode,
             int dryVal = 4095, int wetVal = 1500);

  void begin();
  float readHumidityPct();              // aggregated value; NAN if no sensors
  int   readAllPct(float* out, int maxCount);  // individual readings; returns count

private:
  const PeripheralConfig* _sensors[MAX_SOIL_PER_ZONE];
  int           _count;
  AggregationMode _mode;
  int           _defaultDry;
  int           _defaultWet;

  float _rawToPercent(int raw, int dryVal, int wetVal);
  float _readOneSensor(int idx);
};
