#pragma once
#include <Arduino.h>
#include "../TankManager.h"

struct TankLevel {
  float rawValue;
  float levelPct;   // -1.0 if not applicable
  char  state[12];  // "full", "ok", "low", "empty", "unknown"
};

class TankSensor {
public:
  explicit TankSensor(const TankConfig& cfg);
  void begin();
  TankLevel read();

private:
  const TankConfig& _cfg;

  TankLevel _readBinarySingle();
  TankLevel _readBinaryDual();
  TankLevel _readUltrasonic();
  TankLevel _readAdc();

  void _stateFromPct(float pct, char* out) const;
};
