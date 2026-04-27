#pragma once
#include <Arduino.h>

#define MAX_TANKS          8
#define TANK_NVS_NAMESPACE "tanks"
#define TANK_NVS_KEY       "cfg"

struct TankConfig {
  int     id           = 0;
  uint8_t peripheralId = 0;  // 0 = not assigned
  int     lowPct       = 20;
  int     emptyPct     = 5;
};

class TankManager {
public:
  TankManager();
  void loadFromNVS();
  bool saveToNVS(const char* jsonStr);
  int tankCount() const { return _count; }
  const TankConfig& tank(int index) const { return _tanks[index]; }

private:
  TankConfig _tanks[MAX_TANKS];
  int _count = 0;
};
