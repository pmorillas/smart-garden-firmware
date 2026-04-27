#include "WebUI.h"
#include "webpage.h"
#include "../config.h"
#include <WiFi.h>
#include <Arduino.h>

void WebUI::runAPMode() {
    _apMode = true;
    memset(&_currentCfg, 0, sizeof(_currentCfg));
    _currentCfg.mqttPort = 1883;

    WiFi.mode(WIFI_AP);
    WiFi.softAP(PROVISION_AP_SSID);
    Serial.printf("[WebUI] AP actiu: %s — http://192.168.4.1\n", PROVISION_AP_SSID);

    _dns.start(53, "*", IPAddress(192, 168, 4, 1));
    _installRoutes();
    _server.begin();
    Serial.println("[WebUI] Servidor de configuració iniciat");

    while (!_saveCompleted) {
        _dns.processNextRequest();
        _server.handleClient();
        delay(1);
    }

    delay(2000);
    ESP.restart();
}

void WebUI::begin(const char* hostname, const NetworkConfig& cfg) {
    _apMode     = false;
    _currentCfg = cfg;

    if (MDNS.begin(hostname)) {
        Serial.printf("[WebUI] mDNS: http://%s.local\n", hostname);
    }

    _installRoutes();
    _server.begin();
    Serial.printf("[WebUI] Web UI activa a http://%s\n", WiFi.localIP().toString().c_str());
}

void WebUI::handleClient() {
    _server.handleClient();
}

void WebUI::_installRoutes() {
    _server.on("/", HTTP_GET, [this]() { _handleRoot(); });
    _server.on("/save", HTTP_POST, [this]() { _handleSave(); });

    if (_apMode) {
        // Captive portal: qualsevol ruta desconeguda redirigeix a la pàgina de config
        _server.onNotFound([this]() {
            _server.sendHeader("Location", "http://192.168.4.1/");
            _server.send(302, "text/plain", "");
        });
    } else {
        _server.onNotFound([this]() {
            _server.send(404, "text/plain", "Not found");
        });
    }
}

void WebUI::_handleRoot() {
    _server.send(200, "text/html; charset=utf-8", _buildPage());
}

void WebUI::_handleSave() {
    String ssid   = _server.arg("ssid");
    String pass   = _server.arg("pass");
    String broker = _server.arg("broker");
    int    port   = _server.arg("port").toInt();

    if (ssid.isEmpty() || broker.isEmpty()) {
        _server.send(400, "text/plain", "SSID i broker son obligatoris");
        return;
    }

    NetworkConfig newCfg = {};
    ssid.toCharArray(newCfg.wifiSsid, sizeof(newCfg.wifiSsid));

    if (pass.isEmpty()) {
        memcpy(newCfg.wifiPass, _currentCfg.wifiPass, sizeof(newCfg.wifiPass));
    } else {
        pass.toCharArray(newCfg.wifiPass, sizeof(newCfg.wifiPass));
    }

    broker.toCharArray(newCfg.mqttBroker, sizeof(newCfg.mqttBroker));
    newCfg.mqttPort = (port > 0 && port <= 65535) ? (uint16_t)port : 1883;

    saveNetworkConfig(newCfg);
    _server.send(200, "text/html; charset=utf-8", WEBPAGE_SAVED);
    Serial.printf("[WebUI] Config desada — SSID: %s, Broker: %s:%d\n",
                  newCfg.wifiSsid, newCfg.mqttBroker, newCfg.mqttPort);

    if (_apMode) {
        _saveCompleted = true;
    } else {
        _restartRequested = true;
    }
}

String WebUI::_buildPage() const {
    String html(WEBPAGE_FORM);
    html.replace("%SSID%",   String(_currentCfg.wifiSsid));
    html.replace("%BROKER%", String(_currentCfg.mqttBroker));
    html.replace("%PORT%",   String(_currentCfg.mqttPort > 0 ? _currentCfg.mqttPort : 1883));
    return html;
}
