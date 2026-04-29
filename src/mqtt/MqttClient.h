#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include "../irrigation/IrrigationController.h"
#include "../ota/OtaUpdater.h"
#include "../ZoneManager.h"
#include "../TankManager.h"
#include "../sensors/PeripheralRegistry.h"

class MqttClient {
public:
  void connect(const char* broker, uint16_t port);
  void loop();
  void setIrrigationController(IrrigationController* ctrl);
  void setOtaUpdater(OtaUpdater* ota);
  void setZoneManager(ZoneManager* zm);
  void setTankManager(TankManager* tm);
  void setPeripheralRegistry(PeripheralRegistry* pr);
  void setSensorReadCallback(void (*fn)());
  void publishSoil(int zoneId, const int* rawValues, int count);
  void publishAmbient(float tempC, float humidityPct, float lightLux);
  void publishTank(int tankId, float rawValue, float levelPct, const char* state);
  void publishRegister();

private:
  WiFiClient   _wifiClient;
  PubSubClient _mqtt{_wifiClient};

  static IrrigationController* _irrigCtrl;
  static OtaUpdater*            _otaUpdater;
  static ZoneManager*           _zoneManager;
  static TankManager*           _tankManager;
  static PeripheralRegistry*    _peripheralRegistry;
  static void (*_sensorReadCallback)();

  // Pending actions (processed in loop to avoid MQTT callback reentrancy)
  static bool _otaPending;
  static char _otaUrl[256];
  static char _otaVersion[24];

  static bool _hwCfgPending;
  static char _hwCfgBuf[3072];  // full hardware config JSON from backend

  static bool _sensorReadPending;
  static bool _pingPending;

  char     _broker[64] = {};
  uint16_t _port       = 1883;

  void _reconnect();
  void _performOtaUpdate();
  void _performHardwareConfigUpdate();
  void _publishPong();
  void _publishOtaStatus(const char* status, const char* error = nullptr);
  void _publishHardwareConfigAck(const char* status);
  static void _onMessage(char* topic, byte* payload, unsigned int length);
};
