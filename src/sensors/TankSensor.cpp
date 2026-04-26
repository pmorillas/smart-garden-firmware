#include "TankSensor.h"

TankSensor::TankSensor(const TankConfig& cfg) : _cfg(cfg) {}

void TankSensor::begin() {
  if (strcmp(_cfg.sensorType, "binary_single") == 0) {
    if (_cfg.pin1 >= 0) pinMode(_cfg.pin1, INPUT_PULLUP);

  } else if (strcmp(_cfg.sensorType, "binary_dual") == 0) {
    if (_cfg.pin1 >= 0) pinMode(_cfg.pin1, INPUT_PULLUP);
    if (_cfg.pin2 >= 0) pinMode(_cfg.pin2, INPUT_PULLUP);

  } else if (strcmp(_cfg.sensorType, "ultrasonic") == 0) {
    // HC-SR04: pin1=Trigger, pin2=Echo
    if (_cfg.pin1 >= 0) pinMode(_cfg.pin1, OUTPUT);
    if (_cfg.pin2 >= 0) pinMode(_cfg.pin2, INPUT);

  }
  // ADC types (pressure_adc, capacitive_adc): pins are ADC inputs, no setup needed
}

TankLevel TankSensor::read() {
  if (strcmp(_cfg.sensorType, "binary_single") == 0) return _readBinarySingle();
  if (strcmp(_cfg.sensorType, "binary_dual")   == 0) return _readBinaryDual();
  if (strcmp(_cfg.sensorType, "ultrasonic")    == 0) return _readUltrasonic();
  if (strcmp(_cfg.sensorType, "pressure_adc")  == 0) return _readAdc();
  if (strcmp(_cfg.sensorType, "capacitive_adc") == 0) return _readAdc();

  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  return lvl;
}

TankLevel TankSensor::_readBinarySingle() {
  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  if (_cfg.pin1 < 0) return lvl;

  int val = digitalRead(_cfg.pin1);
  lvl.rawValue = (float)val;
  // HIGH = sensor submerged = full, LOW = exposed = empty
  if (val == HIGH) {
    strncpy(lvl.state, "full", sizeof(lvl.state));
    lvl.levelPct = 100.0f;
  } else {
    strncpy(lvl.state, "empty", sizeof(lvl.state));
    lvl.levelPct = 0.0f;
  }
  return lvl;
}

TankLevel TankSensor::_readBinaryDual() {
  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  // pin1 = high-level float (upper), pin2 = low-level float (lower)
  // HIGH = submerged
  int hiPin = (_cfg.pin1 >= 0) ? digitalRead(_cfg.pin1) : LOW;
  int loPin = (_cfg.pin2 >= 0) ? digitalRead(_cfg.pin2) : LOW;

  lvl.rawValue = (float)(hiPin * 2 + loPin);

  if (hiPin == HIGH && loPin == HIGH) {
    strncpy(lvl.state, "full", sizeof(lvl.state));
    lvl.levelPct = 100.0f;
  } else if (hiPin == LOW && loPin == HIGH) {
    strncpy(lvl.state, "ok", sizeof(lvl.state));
    lvl.levelPct = 50.0f;
  } else if (hiPin == LOW && loPin == LOW) {
    strncpy(lvl.state, "empty", sizeof(lvl.state));
    lvl.levelPct = 0.0f;
  } else {
    // hi submerged but lo exposed — sensor anomaly
    strncpy(lvl.state, "unknown", sizeof(lvl.state));
    lvl.levelPct = -1.0f;
  }
  return lvl;
}

TankLevel TankSensor::_readUltrasonic() {
  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  if (_cfg.pin1 < 0 || _cfg.pin2 < 0) return lvl;

  // HC-SR04 trigger pulse
  digitalWrite(_cfg.pin1, LOW);
  delayMicroseconds(2);
  digitalWrite(_cfg.pin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(_cfg.pin1, LOW);

  long duration = pulseIn(_cfg.pin2, HIGH, 30000UL);  // 30ms timeout
  if (duration == 0) {
    strncpy(lvl.state, "unknown", sizeof(lvl.state));
    return lvl;
  }

  // Distance in cm: sound speed ~0.034 cm/µs, round-trip ÷2
  float distCm = (float)duration * 0.034f / 2.0f;
  lvl.rawValue = distCm;

  if (_cfg.calEmpty > 0 && _cfg.calFull >= 0 && _cfg.calEmpty != _cfg.calFull) {
    // calEmpty = distance when tank empty (sensor near top, distance large)
    // calFull  = distance when tank full  (sensor near top, distance small)
    float pct = (_cfg.calEmpty - distCm) / (float)(_cfg.calEmpty - _cfg.calFull) * 100.0f;
    pct = constrain(pct, 0.0f, 100.0f);
    lvl.levelPct = pct;
    _stateFromPct(pct, lvl.state);
  } else {
    strncpy(lvl.state, "ok", sizeof(lvl.state));
  }
  return lvl;
}

TankLevel TankSensor::_readAdc() {
  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  if (_cfg.pin1 < 0) return lvl;

  int raw = analogRead(_cfg.pin1);
  lvl.rawValue = (float)raw;

  if (_cfg.calEmpty != _cfg.calFull && _cfg.calFull > 0) {
    float pct = ((float)raw - _cfg.calEmpty) / (float)(_cfg.calFull - _cfg.calEmpty) * 100.0f;
    pct = constrain(pct, 0.0f, 100.0f);
    lvl.levelPct = pct;
    _stateFromPct(pct, lvl.state);
  } else {
    strncpy(lvl.state, "ok", sizeof(lvl.state));
  }
  return lvl;
}

void TankSensor::_stateFromPct(float pct, char* out) const {
  if (pct <= (float)_cfg.emptyPct) {
    strncpy(out, "empty", 12);
  } else if (pct <= (float)_cfg.lowPct) {
    strncpy(out, "low", 12);
  } else {
    strncpy(out, "ok", 12);
  }
}
