#pragma once
#include <PubSubClient.h>
#include <WiFiClient.h>

class MqttClient {
public:
  void connect();
  void loop();
  void publishSoil(int zoneId, float humidityPct);
  void publishAmbient(float tempC, float humidityPct);

private:
  WiFiClient _wifiClient;
  PubSubClient _mqtt{_wifiClient};
  void _reconnect();
  static void _onMessage(char* topic, byte* payload, unsigned int length);
};
