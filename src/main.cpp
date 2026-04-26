#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "sensors/SoilSensor.h"
#include "sensors/AmbientSensor.h"
#include "mqtt/MqttClient.h"
#include "irrigation/IrrigationController.h"
#include "schedule/LocalSchedule.h"

SoilSensor soilZone1(SOIL_ZONE1_PIN_A, SOIL_ZONE1_PIN_B);
SoilSensor soilZone2(SOIL_ZONE2_PIN_A, SOIL_ZONE2_PIN_B);
AmbientSensor ambientSensor;
IrrigationController irrigationCtrl;
LocalSchedule localSchedule;
MqttClient mqttClient;

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

  connectWifi();
  mqttClient.setIrrigationController(&irrigationCtrl);
  mqttClient.connect();
  localSchedule.loadFromNVS();

  soilZone1.begin();
  soilZone2.begin();
  ambientSensor.begin();
  irrigationCtrl.begin();

  // Publica immediatament al arrancar sense esperar 5 minuts
  lastPublishMs = millis() - PUBLISH_INTERVAL_MS;

  Serial.println("[smart-garden] Setup complet");
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

    float humZone1 = soilZone1.readHumidityPct();
    float humZone2 = soilZone2.readHumidityPct();
    float temp     = ambientSensor.readTemperature();
    float humAmb   = ambientSensor.readHumidity();
    float light    = ambientSensor.readLightLux();

    Serial.printf("[sensors] Zona1: %.1f%% | Zona2: %.1f%% | Temp: %.1fC | HumAmb: %.1f%% | Llum: %.0flux\n",
                  humZone1, humZone2, temp, humAmb, light);

    mqttClient.publishSoil(1, humZone1);
    mqttClient.publishSoil(2, humZone2);
    mqttClient.publishAmbient(temp, humAmb, light);
  }
}
