#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "ZoneManager.h"
#include "sensors/SoilSensor.h"
#include "sensors/AmbientSensor.h"
#include "mqtt/MqttClient.h"
#include "irrigation/IrrigationController.h"
#include "schedule/LocalSchedule.h"
#include "ota/OtaUpdater.h"

ZoneManager          zoneManager;
AmbientSensor        ambientSensor;
IrrigationController irrigationCtrl;
LocalSchedule        localSchedule;
MqttClient           mqttClient;
OtaUpdater           otaUpdater;

// Sensors de terra dinàmics (allotjats al heap)
static SoilSensor* soilSensors[MAX_ZONES];
static int         numZones = 0;

unsigned long lastPublishMs = 0;

void connectWifi() {
  Serial.printf("[WiFi] Connectant a %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] Connectat! IP: %s\n", WiFi.localIP().toString().c_str());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[smart-garden] Iniciant...");

  // Carrega configuració de zones (NVS → fallback a config.h)
  zoneManager.loadFromNVS();
  numZones = zoneManager.zoneCount();

  // Crea sensors de terra per cada zona configurada
  for (int i = 0; i < numZones; i++) {
    const ZoneConfig& z = zoneManager.zone(i);
    soilSensors[i] = new SoilSensor(z.soilPinA, z.soilPinB);
    soilSensors[i]->begin();
  }

  connectWifi();

  mqttClient.setIrrigationController(&irrigationCtrl);
  mqttClient.setOtaUpdater(&otaUpdater);
  mqttClient.setZoneManager(&zoneManager);
  mqttClient.connect();

  localSchedule.loadFromNVS();
  ambientSensor.begin();
  irrigationCtrl.begin(zoneManager);

  lastPublishMs = millis() - PUBLISH_INTERVAL_MS;

  Serial.printf("[smart-garden] Setup complet — %d zones actives\n", numZones);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connexió perduda, reconnectant...");
    connectWifi();
  }

  mqttClient.loop();
  irrigationCtrl.loop();

  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    for (int i = 0; i < numZones; i++) {
      const ZoneConfig& z = zoneManager.zone(i);
      float hum = soilSensors[i]->readHumidityPct();
      Serial.printf("[sensors] Zona %d: %.1f%%\n", z.id, hum);
      mqttClient.publishSoil(z.id, hum);
    }

    float temp   = ambientSensor.readTemperature();
    float humAmb = ambientSensor.readHumidity();
    float light  = ambientSensor.readLightLux();
    Serial.printf("[sensors] Temp: %.1fC | HumAmb: %.1f%% | Llum: %.0flux\n", temp, humAmb, light);
    mqttClient.publishAmbient(temp, humAmb, light);
  }
}
