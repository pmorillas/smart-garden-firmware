#include "AmbientSensor.h"
#include "../config.h"

void AmbientSensor::begin() {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setTimeOut(200);   // evita que un bus I2C penjat trigui el watchdog

  _htuOk = _htu.begin();
  if (!_htuOk) Serial.println("[AmbientSensor] HTU21D no trobat");

  _bh1750Ok = _bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
  if (!_bh1750Ok) Serial.println("[AmbientSensor] BH1750 no trobat");
}

float AmbientSensor::readTemperature() {
  if (!_htuOk) return NAN;
  return _htu.readTemperature();
}

float AmbientSensor::readHumidity() {
  if (!_htuOk) return NAN;
  return _htu.readHumidity();
}

float AmbientSensor::readLightLux() {
  if (!_bh1750Ok) return NAN;
  return _bh1750.readLightLevel();
}
