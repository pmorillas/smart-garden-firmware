#include "AmbientSensor.h"

void AmbientSensor::begin(const PeripheralRegistry& registry) {
  // Only initialise sensors that are registered as peripherals
  const PeripheralConfig* htuList[1];
  bool hasHtu = registry.byType(PeripheralType::HTU21D, htuList, 1) > 0;

  const PeripheralConfig* bh1750List[1];
  bool hasBh1750 = registry.byType(PeripheralType::BH1750, bh1750List, 1) > 0;

  if (hasHtu) {
    _htuOk = _htu.begin();
    if (!_htuOk) Serial.println("[Ambient] HTU21D no trobat al bus I2C");
  } else {
    Serial.println("[Ambient] HTU21D no configurat en els perifèrics");
  }

  if (hasBh1750) {
    _bh1750Ok = _bh1750.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);
    if (!_bh1750Ok) Serial.println("[Ambient] BH1750 no trobat al bus I2C");
  } else {
    Serial.println("[Ambient] BH1750 no configurat en els perifèrics");
  }
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
