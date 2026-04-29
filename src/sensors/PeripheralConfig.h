#pragma once
#include <Arduino.h>

// Add new types at the END only — type strings are persisted in NVS
enum class PeripheralType : uint8_t {
  UNKNOWN = 0,
  SOIL_ADC,       // Capacitive/resistive soil moisture sensor (ADC1 pin)
  HTU21D,         // I2C temperature + humidity (fixed address 0x40)
  BH1750,         // I2C ambient light in lux (address 0x23 or 0x5C)
  RELAY,          // Digital output relay (active LOW)
  HC_SR04,        // Ultrasonic distance sensor (tank level)
  FLOAT_BINARY,   // Float switch (binary tank level)
  // Future (append here, never reorder):
  // BME280,      // I2C temp + hum + pressure
  // SHT31,       // I2C temp + hum (high precision)
  // AHT20,       // I2C temp + hum (low cost)
  // VEML7700,    // I2C ambient light (high range)
  // RAIN_GAUGE,  // Pulse counter rain gauge
  // SOIL_I2C,    // I2C capacitive soil sensor
};

enum class AggregationMode : uint8_t {
  AVG = 0,    // Average of all sensor readings
  ANY_BELOW,  // Irrigate if any sensor is below threshold
  ALL_BELOW,  // Irrigate only if all sensors are below threshold
};

// 255 is used as "pin not assigned" since valid ESP32 GPIO pins are 0–39
static constexpr uint8_t PIN_UNSET = 255;

static constexpr int MAX_SOIL_PER_ZONE = 8;
static constexpr int MAX_FLOAT_PINS    = 4;

struct FloatPin {
  uint8_t gpio     = PIN_UNSET;
  uint8_t levelPct = 0;    // water level when this pin is active (0–100)
  uint8_t mode     = 0;    // 0=pullup (active LOW), 1=pulldown (active HIGH)
};

struct PeripheralConfig {
  uint8_t        id          = 0;
  PeripheralType type        = PeripheralType::UNKNOWN;
  uint8_t        pin1        = PIN_UNSET;
  uint8_t        pin2        = PIN_UNSET;
  uint8_t        i2cAddress  = 0;
  uint8_t        i2cBus      = 0;
  int            calEmpty    = 0;  // dry/empty calibration (ADC raw or distance cm)
  int            calFull     = 0;  // wet/full  calibration (ADC raw or distance cm)
  char           name[24]    = {};
  // FLOAT_BINARY N-pin configuration (from extra_config.pins)
  FloatPin       floatPins[MAX_FLOAT_PINS] = {};
  uint8_t        floatPinCount = 0;
};

PeripheralType  peripheralTypeFromStr(const char* s);
AggregationMode aggregationModeFromStr(const char* s);
