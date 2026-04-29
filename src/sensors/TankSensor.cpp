#include "TankSensor.h"

TankSensor::TankSensor(const TankConfig& cfg, const PeripheralRegistry& registry)
  : _cfg(cfg)
{
  _perif = registry.byId(cfg.peripheralId);
}

void TankSensor::begin() {
  if (!_perif) {
    Serial.printf("[TankSensor] Dipòsit %d: perifèric %d no trobat al registre\n",
                  _cfg.id, _cfg.peripheralId);
    return;
  }

  switch (_perif->type) {
    case PeripheralType::FLOAT_BINARY:
      if (_perif->floatPinCount > 0) {
        // N-pin mode: configure each pin with its own pullup/pulldown mode
        for (int i = 0; i < _perif->floatPinCount; i++) {
          const FloatPin& fp = _perif->floatPins[i];
          if (fp.gpio == PIN_UNSET) continue;
          pinMode(fp.gpio, fp.mode == 1 ? INPUT_PULLDOWN : INPUT_PULLUP);
        }
      } else {
        // Legacy 1-2 pin mode (INPUT_PULLUP)
        if (_perif->pin1 != PIN_UNSET) pinMode(_perif->pin1, INPUT_PULLUP);
        if (_perif->pin2 != PIN_UNSET) pinMode(_perif->pin2, INPUT_PULLUP);
      }
      break;

    case PeripheralType::HC_SR04:
      if (_perif->pin1 != PIN_UNSET) pinMode(_perif->pin1, OUTPUT);   // Trigger
      if (_perif->pin2 != PIN_UNSET) pinMode(_perif->pin2, INPUT);    // Echo
      break;

    default:
      break;
  }
}

TankLevel TankSensor::read() {
  if (!_perif) {
    TankLevel lvl = {0.0f, -1.0f, "unknown"};
    return lvl;
  }

  switch (_perif->type) {
    case PeripheralType::FLOAT_BINARY:
      if (_perif->floatPinCount > 0) return _readBinaryNPin();
      return (_perif->pin2 != PIN_UNSET) ? _readBinaryDual() : _readBinarySingle();
    case PeripheralType::HC_SR04:
      return _readUltrasonic();
    case PeripheralType::SOIL_ADC:
      return _readAdc();
    default:
      break;
  }

  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  return lvl;
}

int TankSensor::readPinStates(int* out, int maxCount) const {
  if (!_perif || _perif->type != PeripheralType::FLOAT_BINARY) return -1;
  if (_perif->floatPinCount == 0) return -1;

  int count = (_perif->floatPinCount < maxCount) ? _perif->floatPinCount : maxCount;
  for (int i = 0; i < count; i++) {
    if (_perif->floatPins[i].gpio == PIN_UNSET) {
      out[i] = 0;
    } else {
      out[i] = digitalRead(_perif->floatPins[i].gpio);
    }
  }
  return count;
}

TankLevel TankSensor::_readBinaryNPin() {
  TankLevel lvl = {0.0f, 0.0f, "empty"};
  int highestActive = -1;
  float rawBits = 0.0f;

  for (int i = 0; i < _perif->floatPinCount; i++) {
    const FloatPin& fp = _perif->floatPins[i];
    if (fp.gpio == PIN_UNSET) continue;
    int state = digitalRead(fp.gpio);
    if (state) rawBits += (1 << i);

    // pullup (mode=0): active when LOW (state=0)
    // pulldown (mode=1): active when HIGH (state=1)
    bool active = (fp.mode == 1) ? (state == HIGH) : (state == LOW);
    if (active && (int)fp.levelPct > highestActive) {
      highestActive = (int)fp.levelPct;
    }
  }

  lvl.rawValue = rawBits;
  if (highestActive >= 0) {
    lvl.levelPct = (float)highestActive;
    _stateFromPct(lvl.levelPct, lvl.state);
  }
  return lvl;
}

TankLevel TankSensor::_readBinarySingle() {
  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  if (_perif->pin1 == PIN_UNSET) return lvl;

  int val = digitalRead(_perif->pin1);
  lvl.rawValue = (float)val;
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
  int hiVal = (_perif->pin1 != PIN_UNSET) ? digitalRead(_perif->pin1) : LOW;
  int loVal = (_perif->pin2 != PIN_UNSET) ? digitalRead(_perif->pin2) : LOW;

  lvl.rawValue = (float)(hiVal * 2 + loVal);

  if      (hiVal == HIGH && loVal == HIGH) { strncpy(lvl.state, "full",    sizeof(lvl.state)); lvl.levelPct = 100.0f; }
  else if (hiVal == LOW  && loVal == HIGH) { strncpy(lvl.state, "ok",      sizeof(lvl.state)); lvl.levelPct = 50.0f;  }
  else if (hiVal == LOW  && loVal == LOW)  { strncpy(lvl.state, "empty",   sizeof(lvl.state)); lvl.levelPct = 0.0f;   }
  else                                     { strncpy(lvl.state, "unknown", sizeof(lvl.state)); lvl.levelPct = -1.0f;  }
  return lvl;
}

TankLevel TankSensor::_readUltrasonic() {
  TankLevel lvl = {0.0f, -1.0f, "unknown"};
  if (_perif->pin1 == PIN_UNSET || _perif->pin2 == PIN_UNSET) return lvl;

  // HC-SR04 trigger pulse
  digitalWrite(_perif->pin1, LOW);
  delayMicroseconds(2);
  digitalWrite(_perif->pin1, HIGH);
  delayMicroseconds(10);
  digitalWrite(_perif->pin1, LOW);

  long duration = pulseIn(_perif->pin2, HIGH, 30000UL);
  if (duration == 0) { strncpy(lvl.state, "unknown", sizeof(lvl.state)); return lvl; }

  float distCm = (float)duration * 0.034f / 2.0f;
  lvl.rawValue = distCm;

  int calEmpty = _perif->calEmpty;
  int calFull  = _perif->calFull;
  if (calEmpty > 0 && calFull >= 0 && calEmpty != calFull) {
    float pct = (calEmpty - distCm) / (float)(calEmpty - calFull) * 100.0f;
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
  if (_perif->pin1 == PIN_UNSET) return lvl;

  int raw = analogRead(_perif->pin1);
  lvl.rawValue = (float)raw;

  int calEmpty = _perif->calEmpty;
  int calFull  = _perif->calFull;
  if (calEmpty != calFull && calFull > 0) {
    float pct = ((float)raw - calEmpty) / (float)(calFull - calEmpty) * 100.0f;
    pct = constrain(pct, 0.0f, 100.0f);
    lvl.levelPct = pct;
    _stateFromPct(pct, lvl.state);
  } else {
    strncpy(lvl.state, "ok", sizeof(lvl.state));
  }
  return lvl;
}

void TankSensor::_stateFromPct(float pct, char* out) const {
  if      (pct <= (float)_cfg.emptyPct) strncpy(out, "empty", 12);
  else if (pct <= (float)_cfg.lowPct)   strncpy(out, "low",   12);
  else                                  strncpy(out, "ok",    12);
}
