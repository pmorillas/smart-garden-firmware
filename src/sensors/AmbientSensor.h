#pragma once
#include <Wire.h>
#include <Adafruit_HTU21DF.h>
#include <BH1750.h>

class AmbientSensor {
public:
  void begin();
  float readTemperature();     // Celsius, NAN si sensor absent
  float readHumidity();        // %, NAN si sensor absent
  float readLightLux();        // lux, NAN si sensor absent

private:
  Adafruit_HTU21DF _htu;
  BH1750 _bh1750;
  bool _htuOk    = false;
  bool _bh1750Ok = false;
};
