#pragma once
#include <Arduino.h>
#include "../ZoneManager.h"

class IrrigationController {
public:
  void begin(const ZoneManager& zm);
  void startZone(int zoneId, unsigned long durationMs);
  void stopZone(int zoneId);
  void stopAll();
  bool isAnyActive() const;
  void loop();

private:
  int _relayPins[MAX_ZONES];
  int _zoneIds[MAX_ZONES];
  unsigned long _endMs[MAX_ZONES];
  int _count = 0;

  int _indexOf(int zoneId) const;
};
