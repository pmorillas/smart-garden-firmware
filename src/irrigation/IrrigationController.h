#pragma once
#include <Arduino.h>

class IrrigationController {
public:
  void begin();
  void startZone(int zoneId, unsigned long durationMs);
  void stopZone(int zoneId);
  void loop();   // comprova timeouts de reg actius

private:
  unsigned long _zone1EndMs = 0;
  unsigned long _zone2EndMs = 0;
};
