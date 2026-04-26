#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include "../irrigation/IrrigationController.h"
#include "../ota/OtaUpdater.h"
#include "../ZoneManager.h"

class MqttClient {
public:
  void connect();
  void loop();
  void setIrrigationController(IrrigationController* ctrl);
  void setOtaUpdater(OtaUpdater* ota);
  void setZoneManager(ZoneManager* zm);
  void publishSoil(int zoneId, float humidityPct);
  void publishAmbient(float tempC, float humidityPct, float lightLux);
  void publishRegister();

private:
  WiFiClient _wifiClient;
  PubSubClient _mqtt{_wifiClient};

  static IrrigationController* _irrigCtrl;
  static OtaUpdater*            _otaUpdater;
  static ZoneManager*           _zoneManager;

  // OTA pendent (processada al loop, no al callback MQTT)
  static bool _otaPending;
  static char _otaUrl[256];
  static char _otaVersion[24];

  // Zone config pendent (processada al loop per evitar recursió MQTT)
  static bool _zoneConfigPending;
  static char _zoneConfigBuf[1024];

  void _reconnect();
  void _performOtaUpdate();
  void _performZoneConfigUpdate();
  void _publishOtaStatus(const char* status, const char* error = nullptr);
  void _publishZoneConfigAck(const char* status);
  static void _onMessage(char* topic, byte* payload, unsigned int length);
};
