#include "MqttClient.h"
#include "../config.h"
#include <ArduinoJson.h>

IrrigationController* MqttClient::_irrigCtrl = nullptr;

void MqttClient::setIrrigationController(IrrigationController* ctrl) {
  _irrigCtrl = ctrl;
}

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

void MqttClient::publishRegister() {
  JsonDocument doc;
  doc["mac"]      = WiFi.macAddress();
  doc["ip"]       = WiFi.localIP().toString();
  doc["firmware"] = FIRMWARE_VERSION;

  char buf[192];
  serializeJson(doc, buf);
  // retained=true: el backend rep el registre fins i tot si es connecta després
  _mqtt.publish("smartgarden/devices/register", buf, true);
  Serial.printf("[MQTT] Registre publicat: MAC=%s\n", WiFi.macAddress().c_str());
}

void MqttClient::publishSoil(int zoneId, float humidityPct) {
  JsonDocument doc;
  doc["zone_id"]   = zoneId;
  doc["values"][0] = humidityPct;
  doc["unit"]      = "percent";
  doc["mac"]       = WiFi.macAddress();
  doc["timestamp"] = millis() / 1000;

  char buf[160];
  serializeJson(doc, buf);
  char topic[48];
  snprintf(topic, sizeof(topic), "smartgarden/sensors/soil/%d", zoneId);
  _mqtt.publish(topic, buf);
}

void MqttClient::publishAmbient(float tempC, float humidityPct, float lightLux) {
  JsonDocument doc;
  if (!isnan(tempC))       doc["temp"]      = tempC;
  if (!isnan(humidityPct)) doc["humidity"]  = humidityPct;
  if (!isnan(lightLux))    doc["light_lux"] = lightLux;
  doc["unit_temp"] = "celsius";
  doc["mac"]       = WiFi.macAddress();
  doc["timestamp"] = millis() / 1000;

  char buf[192];
  serializeJson(doc, buf);
  _mqtt.publish("smartgarden/sensors/ambient", buf);
}

void MqttClient::_reconnect() {
  Serial.printf("[MQTT] Connectant a %s:%d...\n", MQTT_BROKER, MQTT_PORT);
  unsigned long backoff = 1000;

  // Client ID únic basat en la MAC — evita que múltiples ESP32 es desconnectin entre ells
  String clientId = String("esp32-") + WiFi.macAddress();

  while (!_mqtt.connected()) {
    if (_mqtt.connect(clientId.c_str())) {
      Serial.printf("[MQTT] Connectat com a %s\n", clientId.c_str());
      _mqtt.subscribe("smartgarden/control/#");
      _mqtt.subscribe("smartgarden/config/push");
      publishRegister();
    } else {
      Serial.printf("[MQTT] Error %d, reintentant en %lums\n", _mqtt.state(), backoff);
      delay(backoff);
      backoff = min(backoff * 2, (unsigned long)MQTT_RECONNECT_MAX_MS);
    }
  }
}

void MqttClient::_onMessage(char* topic, byte* payload, unsigned int length) {
  if (_irrigCtrl == nullptr) return;

  JsonDocument doc;
  if (deserializeJson(doc, payload, length) != DeserializationError::Ok) {
    Serial.println("[MQTT] JSON invàlid al missatge de control");
    return;
  }

  const char* action = doc["action"] | "";

  String topicStr(topic);
  if (!topicStr.startsWith("smartgarden/control/")) return;
  int zoneId = topicStr.substring(20).toInt();
  if (zoneId < 1 || zoneId > 2) return;

  if (strcmp(action, "on") == 0) {
    unsigned long durationMs = (unsigned long)(doc["duration_seconds"] | 60) * 1000UL;
    _irrigCtrl->startZone(zoneId, durationMs);
    Serial.printf("[control] Zona %d ON durant %lu s\n", zoneId, durationMs / 1000);
  } else if (strcmp(action, "off") == 0) {
    _irrigCtrl->stopZone(zoneId);
    Serial.printf("[control] Zona %d OFF\n", zoneId);
  }
}
