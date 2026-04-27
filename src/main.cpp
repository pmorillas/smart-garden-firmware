#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "NetworkConfig.h"
#include "webui/WebUI.h"
#include "ZoneManager.h"
#include "TankManager.h"
#include "sensors/SoilSensor.h"
#include "sensors/AmbientSensor.h"
#include "sensors/TankSensor.h"
#include "mqtt/MqttClient.h"
#include "irrigation/IrrigationController.h"
#include "schedule/LocalSchedule.h"
#include "ota/OtaUpdater.h"

static NetworkConfig         netCfg;
static WebUI                 webUI;
static ZoneManager           zoneManager;
static TankManager           tankManager;
static AmbientSensor         ambientSensor;
static IrrigationController  irrigationCtrl;
static LocalSchedule         localSchedule;
static MqttClient            mqttClient;
static OtaUpdater            otaUpdater;

static SoilSensor* soilSensors[MAX_ZONES];
static int         numZones = 0;

static TankSensor* tankSensors[MAX_TANKS];
static int         numTanks = 0;

// Timestamp de l'última lectura publicada (per al failsafe)
static unsigned long lastSensorReadMs = 0;

// Llegeix tots els sensors i publica via MQTT.
// Cridat pel callback pull del backend i pel failsafe.
static void publishAllSensors() {
  lastSensorReadMs = millis();

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

// Intenta connectar al WiFi amb netCfg. Retorna false si passa el timeout de 30s.
static bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(netCfg.wifiSsid, netCfg.wifiPass);
  Serial.printf("[WiFi] Connectant a %s", netCfg.wifiSsid);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 30000UL) {
      Serial.println("\n[WiFi] Timeout");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n[WiFi] Connectat! IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[smart-garden] Iniciant...");

  memset(&netCfg, 0, sizeof(netCfg));
  bool hasConfig = loadNetworkConfig(netCfg);

  if (!hasConfig || !connectWifi()) {
    Serial.println("[boot] Entrant en mode configuració AP...");
    webUI.runAPMode();  // bloqueja fins que l'usuari desa config i reinicia
    return;             // mai s'arriba aquí
  }

  // Carrega configuració de zones (NVS → fallback a config.h)
  zoneManager.loadFromNVS();
  numZones = zoneManager.zoneCount();

  for (int i = 0; i < numZones; i++) {
    const ZoneConfig& z = zoneManager.zone(i);
    soilSensors[i] = new SoilSensor(z.soilPinA, z.soilPinB);
    soilSensors[i]->begin();
  }

  tankManager.loadFromNVS();
  numTanks = tankManager.tankCount();

  for (int i = 0; i < numTanks; i++) {
    tankSensors[i] = new TankSensor(tankManager.tank(i));
    tankSensors[i]->begin();
  }

  webUI.begin("smartgarden", netCfg);

  mqttClient.setIrrigationController(&irrigationCtrl);
  mqttClient.setOtaUpdater(&otaUpdater);
  mqttClient.setZoneManager(&zoneManager);
  mqttClient.setTankManager(&tankManager);
  mqttClient.setSensorReadCallback(publishAllSensors);
  mqttClient.connect(netCfg.mqttBroker, netCfg.mqttPort);

  localSchedule.loadFromNVS();
  ambientSensor.begin();
  irrigationCtrl.begin(zoneManager);

  Serial.printf("[smart-garden] Setup complet — %d zones, %d dipòsits actius\n", numZones, numTanks);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] Connexió perduda, reconnectant...");
    connectWifi();
  }

  webUI.handleClient();

  if (webUI.needsRestart()) {
    Serial.println("[WebUI] Nova config desada, reiniciant...");
    delay(2000);
    ESP.restart();
  }

  mqttClient.loop();
  irrigationCtrl.loop();

  // Failsafe: si el backend no envia cap petició en SENSOR_FAILSAFE_MS, publica igualment
  if (millis() - lastSensorReadMs >= SENSOR_FAILSAFE_MS) {
    Serial.println("[sensors] Failsafe: publicant sense petició del backend");
    publishAllSensors();
  }
}
