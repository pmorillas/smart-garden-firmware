#include "AmbientSensor.h"

AmbientSensor::AmbientSensor(uint8_t pin) : _dht(pin, DHT22) {}

void AmbientSensor::begin() {
  _dht.begin();
}

float AmbientSensor::readTemperature() {
  return _dht.readTemperature();
}

float AmbientSensor::readHumidity() {
  return _dht.readHumidity();
}
