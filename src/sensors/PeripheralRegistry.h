#pragma once
#include "PeripheralConfig.h"

#define MAX_PERIPHERALS    16
#define HW_CFG_NVS_NS      "hw_cfg"
#define HW_CFG_NVS_KEY     "perif"

class PeripheralRegistry {
public:
  void loadFromNVS();

  // Save the "peripherals" JSON array received from backend
  bool saveToNVS(const char* jsonArrayStr);

  const PeripheralConfig* byId(uint8_t id) const;

  // Fill out[] with up to maxOut peripherals of the given type.
  // Returns number found.
  int byType(PeripheralType t, const PeripheralConfig* out[], int maxOut) const;

  int count() const { return _count; }

private:
  PeripheralConfig _p[MAX_PERIPHERALS];
  int _count = 0;

  void _parse(const char* jsonArrayStr);
};
