/*
 * Smart Garden — Firmware ESP32
 *
 * Flux principal:
 *   setup() → connecta WiFi → connecta MQTT → configura sensors i relés
 *   loop()  → cada PUBLISH_INTERVAL_MS llegeix sensors i publica via MQTT
 *           → comprova ordres pendents de control
 *           → mode offline si no hi ha connexió
 */

#include <Arduino.h>
#include "config.h"
#include "sensors/SoilSensor.h"
#include "sensors/AmbientSensor.h"
#include "mqtt/MqttClient.h"
#include "irrigation/IrrigationController.h"
#include "schedule/LocalSchedule.h"

SoilSensor soilZone1(SOIL_ZONE1_PIN_A, SOIL_ZONE1_PIN_B);
SoilSensor soilZone2(SOIL_ZONE2_PIN_A, SOIL_ZONE2_PIN_B);
AmbientSensor ambientSensor(DHT22_PIN);
IrrigationController irrigationCtrl;
LocalSchedule localSchedule;
MqttClient mqttClient;

unsigned long lastPublishMs = 0;

void setup() {
  Serial.begin(115200);

  // TODO: connectar WiFi
  // TODO: mqttClient.connect()
  // TODO: localSchedule.loadFromNVS()

  soilZone1.begin();
  soilZone2.begin();
  ambientSensor.begin();
  irrigationCtrl.begin();

  Serial.println("[smart-garden] Setup complet");
}

void loop() {
  // TODO: mqttClient.loop() — manté connexió i processa missatges entrants

  if (millis() - lastPublishMs >= PUBLISH_INTERVAL_MS) {
    lastPublishMs = millis();

    float humZone1 = soilZone1.readHumidityPct();
    float humZone2 = soilZone2.readHumidityPct();
    float temp     = ambientSensor.readTemperature();
    float humAmb   = ambientSensor.readHumidity();

    // TODO: mqttClient.publishSoil(1, humZone1)
    // TODO: mqttClient.publishSoil(2, humZone2)
    // TODO: mqttClient.publishAmbient(temp, humAmb)
  }

  // TODO: si offline → localSchedule.tick() per executar programes guardats
}
