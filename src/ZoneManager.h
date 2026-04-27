#pragma once
#include <Arduino.h>
#include "sensors/PeripheralConfig.h"

#define MAX_ZONES          8
#define ZONE_NVS_NAMESPACE "zones"
#define ZONE_NVS_KEY       "cfg"

struct ZoneConfig {
  int             id                                  = 0;
  uint8_t         relayPeripheralId                   = 0;    // 0 = not assigned
  AggregationMode aggregationMode                     = AggregationMode::AVG;
  uint8_t         soilPeripheralIds[MAX_SOIL_PER_ZONE] = {};
  uint8_t         soilCount                           = 0;
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
