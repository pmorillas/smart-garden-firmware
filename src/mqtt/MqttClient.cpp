#include "MqttClient.h"
#include "../config.h"
#include <ArduinoJson.h>

void MqttClient::connect() {
  _mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  _mqtt.setCallback(_onMessage);
  _reconnect();
}

void MqttClient::loop() {
  if (!_mqtt.connected()) {
    _reconnect();
  }
  _mqtt.loop();
}

void MqttClient::publishSoil(int zoneId, float humidityPct) {
  JsonDocument doc;
  doc["zone_id"]   = zoneId;
  doc["values"][0] = humidityPct;
  doc["unit"]      = "percent";
  doc["timestamp"] = millis() / 1000;   // TODO: usar NTP

  char buf[128];
  serializeJson(doc, buf);
  char topic[48];
  snprintf(topic, sizeof(topic), "smartgarden/sensors/soil/%d", zoneId);
  _mqtt.publish(topic, buf);
}

void MqttClient::publishAmbient(float tempC, float humidityPct) {
  JsonDocument doc;
  doc["temp"]       = tempC;
  doc["humidity"]   = humidityPct;
  doc["unit_temp"]  = "celsius";
  doc["timestamp"]  = millis() / 1000;

  char buf[128];
  serializeJson(doc, buf);
  _mqtt.publish("smartgarden/sensors/ambient", buf);
}

void MqttClient::_reconnect() {
  unsigned long backoff = 1000;
  while (!_mqtt.connected()) {
    if (_mqtt.connect(MQTT_CLIENT_ID)) {
      _mqtt.subscribe("smartgarden/control/#");
      _mqtt.subscribe("smartgarden/config/push");
    } else {
      delay(backoff);
      backoff = min(backoff * 2, (unsigned long)MQTT_RECONNECT_MAX_MS);
    }
  }
}

void MqttClient::_onMessage(char* topic, byte* payload, unsigned int length) {
  // TODO: processar ordres de control i config rebudes del backend
}
