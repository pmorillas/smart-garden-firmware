#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include "../irrigation/IrrigationController.h"
#include "../ota/OtaUpdater.h"

class MqttClient {
public:
  void connect();
  void loop();
  void setIrrigationController(IrrigationController* ctrl);
  void setOtaUpdater(OtaUpdater* ota);
  void publishSoil(int zoneId, float humidityPct);
  void publishAmbient(float tempC, float humidityPct, float lightLux);
  void publishRegister();

private:
  WiFiClient _wifiClient;
  PubSubClient _mqtt{_wifiClient};
  static IrrigationController* _irrigCtrl;
  static OtaUpdater*            _otaUpdater;

  // Petició OTA pendent (processada al loop, no al callback MQTT)
  static bool        _otaPending;
  static char        _otaUrl[256];
  static char        _otaVersion[24];

  void _reconnect();
  void _performOtaUpdate();
  void _publishOtaStatus(const char* status, const char* error = nullptr);
  static void _onMessage(char* topic, byte* payload, unsigned int length);
};
