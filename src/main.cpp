#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "ZoneManager.h"
#include "TankManager.h"
#include "sensors/SoilSensor.h"
#include "sensors/AmbientSensor.h"
#include "sensors/TankSensor.h"
#include "mqtt/MqttClient.h"
#include "irrigation/IrrigationController.h"
#include "schedule/LocalSchedule.h"
#include "ota/OtaUpdater.h"

ZoneManager          zoneManager;
TankManager          tankManager;
AmbientSensor        ambientSensor;
IrrigationController irrigationCtrl;
LocalSchedule        localSchedule;
MqttClient           mqttClient;
OtaUpdater           otaUpdater;

// Sensors de terra i dipòsit (allotjats al heap)
static SoilSensor* soilSensors[MAX_ZONES];
static int         numZones = 0;

static TankSensor* tankSensors[MAX_TANKS];
static int         numTanks = 0;

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

  // Carrega configuració de dipòsits (NVS)
  tankManager.loadFromNVS();
  numTanks = tankManager.tankCount();

  // Crea sensors de dipòsit per cada dipòsit configurat
  for (int i = 0; i < numTanks; i++) {
    tankSensors[i] = new TankSensor(tankManager.tank(i));
    tankSensors[i]->begin();
  }

  connectWifi();

  mqttClient.setIrrigationController(&irrigationCtrl);
  mqttClient.setOtaUpdater(&otaUpdater);
  mqttClient.setZoneManager(&zoneManager);
  mqttClient.setTankManager(&tankManager);
  mqttClient.connect();

  localSchedule.loadFromNVS();
  ambientSensor.begin();
  irrigationCtrl.begin(zoneManager);

  lastPublishMs = millis() - PUBLISH_INTERVAL_MS;

  Serial.printf("[smart-garden] Setup complet — %d zones, %d dipòsits actius\n", numZones, numTanks);
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

    for (int i = 0; i < numTanks; i++) {
      const TankConfig& tc = tankManager.tank(i);
      TankLevel lvl = tankSensors[i]->read();
      Serial.printf("[sensors] Dipòsit %d: raw=%.1f pct=%.1f estat=%s\n",
                    tc.id, lvl.rawValue, lvl.levelPct, lvl.state);
      mqttClient.publishTank(tc.id, lvl.rawValue, lvl.levelPct, lvl.state);
    }
  }
}
