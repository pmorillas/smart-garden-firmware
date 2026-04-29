#include "MqttClient.h"
#include "../config.h"
#include <ArduinoJson.h>

IrrigationController* MqttClient::_irrigCtrl         = nullptr;
OtaUpdater*           MqttClient::_otaUpdater         = nullptr;
ZoneManager*          MqttClient::_zoneManager        = nullptr;
TankManager*          MqttClient::_tankManager        = nullptr;
PeripheralRegistry*   MqttClient::_peripheralRegistry = nullptr;
void (*MqttClient::_sensorReadCallback)()             = nullptr;
bool                  MqttClient::_otaPending          = false;
char                  MqttClient::_otaUrl[256]         = {};
char                  MqttClient::_otaVersion[24]      = {};
bool                  MqttClient::_hwCfgPending        = false;
char                  MqttClient::_hwCfgBuf[3072]      = {};
bool                  MqttClient::_sensorReadPending   = false;
bool                  MqttClient::_pingPending          = false;

void MqttClient::setIrrigationController(IrrigationController* ctrl) { _irrigCtrl         = ctrl; }
void MqttClient::setOtaUpdater(OtaUpdater* ota)                       { _otaUpdater         = ota; }
void MqttClient::setZoneManager(ZoneManager* zm)                       { _zoneManager        = zm; }
void MqttClient::setTankManager(TankManager* tm)                       { _tankManager        = tm; }
void MqttClient::setPeripheralRegistry(PeripheralRegistry* pr)         { _peripheralRegistry = pr; }
void MqttClient::setSensorReadCallback(void (*fn)())                   { _sensorReadCallback = fn; }

void MqttClient::connect(const char* broker, uint16_t port) {
  strncpy(_broker, broker, sizeof(_broker) - 1);
  _port = port;
  _mqtt.setServer(_broker, _port);
  _mqtt.setBufferSize(3072);
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

  if (_hwCfgPending) {
    _hwCfgPending = false;
    _performHardwareConfigUpdate();
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

void MqttClient::publishSoil(int zoneId, const int* rawValues, int count) {
  JsonDocument doc;
  doc["zone_id"] = zoneId;
  JsonArray arr  = doc["raw_values"].to<JsonArray>();
  for (int i = 0; i < count; i++) arr.add(rawValues[i]);
  doc["mac"]       = WiFi.macAddress();
  doc["timestamp"] = millis() / 1000;

  char buf[256];
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

void MqttClient::_publishHardwareConfigAck(const char* status) {
  JsonDocument doc;
  doc["config"] = "hardware";
  doc["status"] = status;

  char buf[128];
  serializeJson(doc, buf);
  char topic[80];
  snprintf(topic, sizeof(topic), "smartgarden/devices/ack/%s", WiFi.macAddress().c_str());
  _mqtt.publish(topic, buf);
  _mqtt.loop();
  Serial.printf("[HwCfg] ACK publicat: %s\n", status);
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

void MqttClient::_performHardwareConfigUpdate() {
  Serial.println("[HwCfg] Aplicant nova configuració de hardware...");

  if (_irrigCtrl) _irrigCtrl->stopAll();

  JsonDocument doc;
  if (deserializeJson(doc, _hwCfgBuf) != DeserializationError::Ok) {
    Serial.println("[HwCfg] Error parsejant JSON");
    _publishHardwareConfigAck("failed");
    return;
  }

  // Re-serialize each sub-array for individual NVS storage.
  // PeripheralRegistry expects a top-level JSON array.
  // ZoneManager expects {"zones":[...]} and TankManager expects {"tanks":[...]}.
  char perifBuf[2048];
  char zonesBuf[768];
  char tanksBuf[512];

  serializeJson(doc["peripherals"], perifBuf, sizeof(perifBuf));

  {
    JsonDocument wrapper;
    wrapper["zones"] = doc["zones"];
    serializeJson(wrapper, zonesBuf, sizeof(zonesBuf));
  }
  {
    JsonDocument wrapper;
    wrapper["tanks"] = doc["tanks"];
    serializeJson(wrapper, tanksBuf, sizeof(tanksBuf));
  }

  bool ok = true;
  if (_peripheralRegistry) ok &= _peripheralRegistry->saveToNVS(perifBuf);
  if (_zoneManager)        ok &= _zoneManager->saveToNVS(zonesBuf);
  if (_tankManager)        ok &= _tankManager->saveToNVS(tanksBuf);

  _publishHardwareConfigAck(ok ? "stored" : "failed");

  if (ok) {
    Serial.println("[HwCfg] Reiniciant en 1s...");
    delay(1000);
    ESP.restart();
  } else {
    Serial.println("[HwCfg] Error desant config, continuant amb config actual");
  }
}

void MqttClient::_reconnect() {
  Serial.printf("[MQTT] Connectant a %s:%d...\n", _broker, _port);
  unsigned long backoff = 1000;

  String clientId       = String("esp32-") + WiFi.macAddress();
  String otaTopic       = String("smartgarden/ota/") + WiFi.macAddress();
  String hwCfgTopic     = String("smartgarden/config/hardware/") + WiFi.macAddress();
  String sensorReqTopic = String("smartgarden/sensors/request/") + WiFi.macAddress();
  String pingTopic      = String("smartgarden/ping/") + WiFi.macAddress();

  while (!_mqtt.connected()) {
    if (_mqtt.connect(clientId.c_str())) {
      Serial.printf("[MQTT] Connectat com a %s\n", clientId.c_str());
      _mqtt.subscribe("smartgarden/control/#");
      _mqtt.subscribe("smartgarden/config/push");
      _mqtt.subscribe(otaTopic.c_str());
      _mqtt.subscribe(hwCfgTopic.c_str());
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
  String topicStr(topic);

  // Sensor read request (pull mode)
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

  // Hardware config update — copy raw payload, parse later in loop()
  if (topicStr.startsWith("smartgarden/config/hardware/")) {
    size_t copied = length < sizeof(_hwCfgBuf) - 1 ? length : sizeof(_hwCfgBuf) - 1;
    memcpy(_hwCfgBuf, payload, copied);
    _hwCfgBuf[copied] = '\0';
    _hwCfgPending = true;
    Serial.println("[HwCfg] Config rebuda, processant al loop...");
    return;
  }

  // All remaining topics need JSON
  JsonDocument doc;
  if (deserializeJson(doc, payload, length) != DeserializationError::Ok) {
    Serial.println("[MQTT] JSON invàlid");
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

  // Irrigation control
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
