#pragma once
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include "../NetworkConfig.h"

class WebUI {
public:
    // Primera arrencada: crea AP, bloqueja fins que l'usuari desa config, restart
    void runAPMode();

    // Mode normal: inicia WebServer + mDNS (WiFi ja connectat)
    void begin(const char* hostname, const NetworkConfig& cfg);

    // Crida a loop() per atendre peticions HTTP
    void handleClient();

    // Retorna true si l'usuari ha desat nova config (cal restart extern)
    bool needsRestart() const { return _restartRequested; }

private:
    WebServer      _server{80};
    DNSServer      _dns;
    NetworkConfig  _currentCfg{};
    bool           _apMode          = false;
    bool           _saveCompleted   = false;  // per sortir del bucle AP
    bool           _restartRequested = false; // per mode normal

    void _installRoutes();
    void _handleRoot();
    void _handleSave();

    String _buildPage() const;
};
