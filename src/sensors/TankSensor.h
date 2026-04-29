#pragma once
#include <Arduino.h>
#include "../TankManager.h"
#include "PeripheralRegistry.h"

struct TankLevel {
  float rawValue;
  float levelPct;   // -1.0 if not applicable
  char  state[12];  // "full", "ok", "low", "empty", "unknown"
};

class TankSensor {
public:
  TankSensor(const TankConfig& cfg, const PeripheralRegistry& registry);
  void begin();
  TankLevel read();

  // For FLOAT_BINARY N-pin sensors: fills out[] with raw digital states (0/1)
  // aligned with extra_config.pins order. Returns pin count, or -1 if not applicable.
  int readPinStates(int* out, int maxCount) const;

private:
  const TankConfig&    _cfg;
  const PeripheralConfig* _perif;  // nullptr if not found in registry

  TankLevel _readBinaryNPin();
  TankLevel _readBinarySingle();
  TankLevel _readBinaryDual();
  TankLevel _readUltrasonic();
  TankLevel _readAdc();

  void _stateFromPct(float pct, char* out) const;
};
