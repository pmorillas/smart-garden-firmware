#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFi.h>
#include "../irrigation/IrrigationController.h"

class MqttClient {
public:
  void connect();
  void loop();
  void setIrrigationController(IrrigationController* ctrl);
  void publishSoil(int zoneId, float humidityPct);
  void publishAmbient(float tempC, float humidityPct, float lightLux);
  void publishRegister();

private:
  WiFiClient _wifiClient;
  PubSubClient _mqtt{_wifiClient};
  static IrrigationController* _irrigCtrl;
  void _reconnect();
  static void _onMessage(char* topic, byte* payload, unsigned int length);
};
