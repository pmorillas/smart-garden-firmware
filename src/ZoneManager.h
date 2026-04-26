#pragma once
#include <Arduino.h>

#define MAX_ZONES 8
#define ZONE_NVS_NAMESPACE "zones"
#define ZONE_NVS_KEY "cfg"

struct ZoneConfig {
  int id;
  int relayPin;
  int soilPinA;
  int soilPinB;
};

class ZoneManager {
public:
  ZoneManager();
  void loadFromNVS();
  bool saveToNVS(const char* jsonStr);
  int zoneCount() const { return _count; }
  const ZoneConfig& zone(int index) const { return _zones[index]; }
  int indexById(int id) const;

private:
  ZoneConfig _zones[MAX_ZONES];
  int _count = 0;
  void _loadDefaults();
};
