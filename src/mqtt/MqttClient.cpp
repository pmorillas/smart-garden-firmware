#include "MqttClient.h"
#include "../config.h"
#include <ArduinoJson.h>

IrrigationController* MqttClient::_irrigCtrl      = nullptr;
OtaUpdater*           MqttClient::_otaUpdater      = nullptr;
ZoneManager*          MqttClient::_zoneManager     = nullptr;
TankManager*          MqttClient::_tankManager     = nullptr;
void (*MqttClient::_sensorReadCallback)()          = nullptr;
bool                  MqttClient::_otaPending       = false;
char                  MqttClient::_otaUrl[256]      = {};
char                  MqttClient::_otaVersion[24]   = {};
bool                  MqttClient::_zoneConfigPending = false;
char                  MqttClient::_zoneConfigBuf[1024] = {};
bool                  MqttClient::_tankConfigPending = false;
char                  MqttClient::_tankConfigBuf[2048] = {};
bool                  MqttClient::_sensorReadPending = false;
bool                  MqttClient::_pingPending       = false;

void MqttClient::setIrrigationController(IrrigationController* ctrl) { _irrigCtrl  = ctrl; }
void MqttClient::setOtaUpdater(OtaUpdater* ota)                       { _otaUpdater = ota; }
void MqttClient::setZoneManager(ZoneManager* zm)                      { _zoneManager = zm; }
void MqttClient::setTankManager(TankManager* tm)                      { _tankManager = tm; }
void MqttClient::setSensorReadCallback(void (*fn)())                  { _sensorReadCallback = fn; }

void MqttClient::connect(const char* broker, uint16_t port) {
  strncpy(_broker, broker, sizeof(_broker) - 1);
  _port = port;
  _mqtt.setServer(_broker, _port);
  _mqtt.setBufferSize(1024);
  _mqtt.setCallback(_onMessage);
  _reconnect();
}

void MqttClient::loop() {
  if (!_mqtt.connected()) {
    _reconnect();
  }
  _mqtt.loop();

  if (_sensorReadPending) {
    _sensorReadPending = false;
    if (_sensorReadCallback) _sensorReadCallback();
  }

  if (_pingPending) {
    _pingPending = false;
    _publishPong();
  }

  if (_zoneConfigPending) {
    _zoneConfigPending = false;
    _performZoneConfigUpdate();
  }

  if (_tankConfigPending) {
    _tankConfigPending = false;
    _performTankConfigUpdate();
  }

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

void MqttClient::publishTank(int tankId, float rawValue, float levelPct, const char* state) {
  JsonDocument doc;
  doc["raw_value"] = rawValue;
  if (levelPct >= 0.0f) doc["level_pct"] = levelPct;
  doc["state"]     = state;
  doc["mac"]       = WiFi.macAddress();
  doc["timestamp"] = millis() / 1000;

  char buf[160];
  serializeJson(doc, buf);
  char topic[48];
  snprintf(topic, sizeof(topic), "smartgarden/sensors/tank/%d", tankId);
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

void MqttClient::_publishPong() {
  JsonDocument doc;
  doc["mac"] = WiFi.macAddress();
  doc["ts"]  = millis() / 1000;

  char buf[128];
  serializeJson(doc, buf);
  char topic[80];
  snprintf(topic, sizeof(topic), "smartgarden/pong/%s", WiFi.macAddress().c_str());
  _mqtt.publish(topic, buf);
  Serial.println("[MQTT] Pong enviat");
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

void MqttClient::_publishZoneConfigAck(const char* status) {
  JsonDocument doc;
  doc["config"] = "zones";
  doc["status"] = status;

  char buf[128];
  serializeJson(doc, buf);

  char topic[80];
  snprintf(topic, sizeof(topic), "smartgarden/devices/ack/%s", WiFi.macAddress().c_str());
  _mqtt.publish(topic, buf);
  _mqtt.loop();  // força l'enviament abans de continuar
  Serial.printf("[ZoneCfg] ACK publicat: %s\n", status);
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

void MqttClient::_performZoneConfigUpdate() {
  Serial.println("[ZoneCfg] Aplicant nova configuració de zones...");

  if (_irrigCtrl) {
    _irrigCtrl->stopAll();
  }

  if (_zoneManager && _zoneManager->saveToNVS(_zoneConfigBuf)) {
    _publishZoneConfigAck("stored");
    Serial.println("[ZoneCfg] Reiniciant en 1s...");
    delay(1000);
    ESP.restart();
  } else {
    _publishZoneConfigAck("failed");
    Serial.println("[ZoneCfg] Error desant config, continuant amb config actual");
  }
}

void MqttClient::_performTankConfigUpdate() {
  Serial.println("[TankCfg] Aplicant nova configuració de dipòsits...");
  if (_tankManager && _tankManager->saveToNVS(_tankConfigBuf)) {
    Serial.println("[TankCfg] Reiniciant en 1s...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("[TankCfg] Error desant config");
  }
}

void MqttClient::_reconnect() {
  Serial.printf("[MQTT] Connectant a %s:%d...\n", _broker, _port);
  unsigned long backoff = 1000;

  String clientId         = String("esp32-") + WiFi.macAddress();
  String otaTopic         = String("smartgarden/ota/") + WiFi.macAddress();
  String zoneCfgTopic     = String("smartgarden/config/zones/") + WiFi.macAddress();
  String tankCfgTopic     = String("smartgarden/config/tanks/") + WiFi.macAddress();
  String sensorReqTopic   = String("smartgarden/sensors/request/") + WiFi.macAddress();
  String pingTopic        = String("smartgarden/ping/") + WiFi.macAddress();

  while (!_mqtt.connected()) {
    if (_mqtt.connect(clientId.c_str())) {
      Serial.printf("[MQTT] Connectat com a %s\n", clientId.c_str());
      _mqtt.subscribe("smartgarden/control/#");
      _mqtt.subscribe("smartgarden/config/push");
      _mqtt.subscribe(otaTopic.c_str());
      _mqtt.subscribe(zoneCfgTopic.c_str());
      _mqtt.subscribe(tankCfgTopic.c_str());
      _mqtt.subscribe(sensorReqTopic.c_str());
      _mqtt.subscribe(pingTopic.c_str());
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

  // Petició de lectura de sensors (mode pull)
  if (topicStr.startsWith("smartgarden/sensors/request/")) {
    _sensorReadPending = true;
    Serial.println("[MQTT] Petició de lectura rebuda");
    return;
  }

  // Ping
  if (topicStr.startsWith("smartgarden/ping/")) {
    _pingPending = true;
    Serial.println("[MQTT] Ping rebut");
    return;
  }

  // Zone config update
  if (topicStr.startsWith("smartgarden/config/zones/")) {
    size_t copied = length < sizeof(_zoneConfigBuf) - 1 ? length : sizeof(_zoneConfigBuf) - 1;
    memcpy(_zoneConfigBuf, payload, copied);
    _zoneConfigBuf[copied] = '\0';
    _zoneConfigPending = true;
    Serial.println("[ZoneCfg] Config rebuda, processant al loop...");
    return;
  }

  // Tank config update
  if (topicStr.startsWith("smartgarden/config/tanks/")) {
    size_t copied = length < sizeof(_tankConfigBuf) - 1 ? length : sizeof(_tankConfigBuf) - 1;
    memcpy(_tankConfigBuf, payload, copied);
    _tankConfigBuf[copied] = '\0';
    _tankConfigPending = true;
    Serial.println("[TankCfg] Config rebuda, processant al loop...");
    return;
  }

  // OTA update
  if (topicStr.startsWith("smartgarden/ota/")) {
    const char* version = doc["version"] | "";
    const char* url     = doc["url"]     | "";
    if (strlen(url) == 0 || strlen(version) == 0) return;

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

  const char* action = doc["action"] | "";
  if (strcmp(action, "on") == 0) {
    unsigned long durationMs = (unsigned long)(doc["duration_seconds"] | 60) * 1000UL;
    _irrigCtrl->startZone(zoneId, durationMs);
  } else if (strcmp(action, "off") == 0) {
    _irrigCtrl->stopZone(zoneId);
  }
}
