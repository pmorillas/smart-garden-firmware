#pragma once
#include <Arduino.h>

class OtaUpdater {
public:
  // Retorna true si s'ha flashat correctament (l'ESP32 reiniciarà sol)
  bool update(const char* url);
};
