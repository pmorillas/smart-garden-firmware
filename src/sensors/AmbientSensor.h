#pragma once
#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <BH1750.h>
#include "PeripheralRegistry.h"

class AmbientSensor {
public:
  // Initialise based on what the registry has registered.
  // Wire.begin() must have been called before this.
  void begin(const PeripheralRegistry& registry);

  float readTemperature();  // Celsius, NAN if not available
  float readHumidity();     // %, NAN if not available
  float readLightLux();     // lux, NAN if not available

private:
  Adafruit_HTU21DF _htu;
  BH1750 _bh1750;
  bool _htuOk    = false;
  bool _bh1750Ok = false;
};
