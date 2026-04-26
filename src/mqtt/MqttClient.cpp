#include "MqttClient.h"
#include "../config.h"
#include <ArduinoJson.h>

IrrigationController* MqttClient::_irrigCtrl  = nullptr;
OtaUpdater*           MqttClient::_otaUpdater  = nullptr;
bool                  MqttClient::_otaPending   = false;
char                  MqttClient::_otaUrl[256]  = {};
char                  MqttClient::_otaVersion[24] = {};

void MqttClient::setIrrigationController(IrrigationController* ctrl) {
  _irrigCtrl = ctrl;
}

void MqttClient::setOtaUpdater(OtaUpdater* ota) {
  _otaUpdater = ota;
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

  if (_otaPending) {
    _otaPending = false;
    _performOtaUpdate();
  }
}

void MqttClient::publishRegister() {
  JsonDocument doc;
  doc["mac"]      = WiFi.macAddress();
  doc["ip"]       = WiFi.localIP().toString();
  doc["firmware"] = FIRMWARE_VERSION;

  char buf[192];
  serializeJson(doc, buf);
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

void MqttClient::_publishOtaStatus(const char* status, const char* error) {
  JsonDocument doc;
  doc["mac"]     = WiFi.macAddress();
  doc["status"]  = status;
  doc["version"] = _otaVersion;
  if (error) doc["error"] = error;

  char buf[256];
  serializeJson(doc, buf);
  _mqtt.publish("smartgarden/devices/ota_status", buf);
  Serial.printf("[OTA] Status publicat: %s\n", status);
}

void MqttClient::_performOtaUpdate() {
  if (_otaUpdater == nullptr) return;

  Serial.printf("[OTA] Actualitzant a v%s des de %s\n", _otaVersion, _otaUrl);
  _publishOtaStatus("downloading");

  bool ok = _otaUpdater->update(_otaUrl);

  if (ok) {
    _publishOtaStatus("success");
    delay(500);
    ESP.restart();
  } else {
    _publishOtaStatus("failed", "Error durant la descàrrega o el flash");
    Serial.println("[OTA] Actualització fallida, continuant operació normal");
  }
}

void MqttClient::_reconnect() {
  Serial.printf("[MQTT] Connectant a %s:%d...\n", MQTT_BROKER, MQTT_PORT);
  unsigned long backoff = 1000;

  String clientId = String("esp32-") + WiFi.macAddress();
  String otaTopic = String("smartgarden/ota/") + WiFi.macAddress();

  while (!_mqtt.connected()) {
    if (_mqtt.connect(clientId.c_str())) {
      Serial.printf("[MQTT] Connectat com a %s\n", clientId.c_str());
      _mqtt.subscribe("smartgarden/control/#");
      _mqtt.subscribe("smartgarden/config/push");
      _mqtt.subscribe(otaTopic.c_str());
      publishRegister();
    } else {
      Serial.printf("[MQTT] Error %d, reintentant en %lums\n", _mqtt.state(), backoff);
      delay(backoff);
      backoff = min(backoff * 2, (unsigned long)MQTT_RECONNECT_MAX_MS);
    }
  }
}

void MqttClient::_onMessage(char* topic, byte* payload, unsigned int length) {
  JsonDocument doc;
  if (deserializeJson(doc, payload, length) != DeserializationError::Ok) {
    Serial.println("[MQTT] JSON invàlid");
    return;
  }

  String topicStr(topic);

  // OTA update
  if (topicStr.startsWith("smartgarden/ota/")) {
    const char* version = doc["version"] | "";
    const char* url     = doc["url"]     | "";
    if (strlen(url) == 0 || strlen(version) == 0) return;

    // No tornem a actualitzar si ja tenim la mateixa versió
    if (strcmp(version, FIRMWARE_VERSION) == 0) {
      Serial.printf("[OTA] Versió %s ja instal·lada, ignorant\n", version);
      return;
    }

    strncpy(_otaUrl,     url,     sizeof(_otaUrl)     - 1);
    strncpy(_otaVersion, version, sizeof(_otaVersion) - 1);
    _otaUrl[sizeof(_otaUrl) - 1]         = '\0';
    _otaVersion[sizeof(_otaVersion) - 1] = '\0';
    _otaPending = true;
    Serial.printf("[OTA] Actualització pendent: v%s\n", version);
    return;
  }

  // Control de reg
  if (_irrigCtrl == nullptr) return;
  if (!topicStr.startsWith("smartgarden/control/")) return;
  int zoneId = topicStr.substring(20).toInt();
  if (zoneId < 1 || zoneId > 2) return;

  const char* action = doc["action"] | "";
  if (strcmp(action, "on") == 0) {
    unsigned long durationMs = (unsigned long)(doc["duration_seconds"] | 60) * 1000UL;
    _irrigCtrl->startZone(zoneId, durationMs);
    Serial.printf("[control] Zona %d ON durant %lu s\n", zoneId, durationMs / 1000);
  } else if (strcmp(action, "off") == 0) {
    _irrigCtrl->stopZone(zoneId);
    Serial.printf("[control] Zona %d OFF\n", zoneId);
  }
}
