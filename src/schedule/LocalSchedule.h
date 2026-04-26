#pragma once
#include <Arduino.h>

struct ScheduleEntry {
  int zoneId;
  uint8_t hour;
  uint8_t minute;
  uint8_t daysOfWeek;   // bitmask: bit0=dilluns ... bit6=diumenge
  uint16_t durationMinutes;
};

class LocalSchedule {
public:
  void loadFromNVS();
  void saveToNVS();
  void updateFromJson(const char* json);
  void tick(uint8_t currentHour, uint8_t currentMinute, uint8_t currentDow);

private:
  static constexpr int MAX_ENTRIES = 8;
  ScheduleEntry _entries[MAX_ENTRIES];
  int _count = 0;
};
