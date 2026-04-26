#pragma once
#include <Arduino.h>

#define MAX_TANKS 8
#define TANK_NVS_NAMESPACE "tanks"
#define TANK_NVS_KEY "cfg"

struct TankConfig {
  int  id;
  char sensorType[20];
  int  pin1;
  int  pin2;
  int  calEmpty;
  int  calFull;
  int  lowPct;
  int  emptyPct;
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
