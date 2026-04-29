#pragma once
#include <Arduino.h>
#include "PeripheralConfig.h"

class SoilSensor {
public:
  // sensors: array of pointers to PeripheralConfig (SOIL_ADC type)
  // count:   number of sensors (0 = zone with no soil sensors → returns NAN)
  // mode:    how to aggregate multiple sensor readings
  // Calibration is done server-side; firmware only reports raw ADC values.
  SoilSensor(const PeripheralConfig* sensors[], int count, AggregationMode mode);

  void begin();
  int readAllRaw(int* out, int maxCount);  // raw ADC values per sensor; returns count

private:
  const PeripheralConfig* _sensors[MAX_SOIL_PER_ZONE];
  int           _count;
  AggregationMode _mode;
};
