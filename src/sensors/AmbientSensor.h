#pragma once
#include <Arduino.h>
#include <DHT.h>

class AmbientSensor {
public:
  explicit AmbientSensor(uint8_t pin);
  void begin();
  float readTemperature();    // Celsius
  float readHumidity();       // %

private:
  DHT _dht;
};
