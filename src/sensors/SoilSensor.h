#pragma once
#include <Arduino.h>

class SoilSensor {
public:
  SoilSensor(uint8_t pinA, uint8_t pinB);
  void begin();
  float readHumidityPct();   // retorna mitjana dels dos sensors [0.0, 100.0]

private:
  uint8_t _pinA, _pinB;
  float _rawToPercent(int raw);
};
